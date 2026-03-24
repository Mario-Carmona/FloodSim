"""Client for fetching bathymetry data from the EMODnet Web Coverage Service (WCS)."""

from typing import Optional

import numpy as np
from loguru import logger
from pyproj import CRS, Transformer

from utils.types import SpatialContext, StaticRaster
from utils.api_client import fetch_with_retries
from utils.raster_io import load_raster_from_bytes


class EmodnetClient:
    """A client for interacting with the EMODnet bathymetry API.

    Attributes:
        EMODNET_WCS_URL (str): The endpoint URL for the EMODnet WCS service.
        WGS84_CRS (pyproj.CRS): The WGS84 coordinate reference system object.
    """

    EMODNET_WCS_URL = "https://ows.emodnet-bathymetry.eu/wcs" 
    WGS84_CRS = CRS.from_epsg(4326)

    @classmethod
    def fetch_bathymetry(cls, ctx: SpatialContext) -> StaticRaster:
        """Fetches bathymetry data from EMODnet for a given spatial context.

        This method calculates the WGS84 bounding box for the requested spatial
        context, formulates a WCS GetCoverage request, and fetches the raster 
        data. The downloaded TIFF is then loaded directly into a StaticRaster.

        Args:
            ctx (SpatialContext): The spatial context defining the bounds and CRS 
                for the desired bathymetry data.

        Returns:
            StaticRaster: An object containing the bathymetry raster data matrix 
                and its spatial metadata.

        Raises:
            Exception: If the API request fails to return valid content 
                after the maximum number of retries.
        """
        # Check if transformation is needed before initializing the Transformer
        if ctx.crs != cls.WGS84_CRS:
            logger.debug(
                f"Transforming BBox from {ctx.crs.to_epsg() or 'Local'} to 4326 for WCS search."
            )
            
            # Initialize transformer to convert from the context's CRS to WGS84 (EPSG:4326)
            transformer = Transformer.from_crs(ctx.crs, cls.WGS84_CRS, always_xy=True)

            # Define the corners of the original bounding box
            xs = [ctx.bounds.min_x, ctx.bounds.max_x, ctx.bounds.max_x, ctx.bounds.min_x]
            ys = [ctx.bounds.min_y, ctx.bounds.min_y, ctx.bounds.max_y, ctx.bounds.max_y]
            
            # Transform the corner coordinates to WGS84
            lons, lats = transformer.transform(xs, ys)
            
            # Extract the new bounding box limits in WGS84
            wgs84_bbox = [min(lons), min(lats), max(lons), max(lats)]
        else:
            logger.debug("CRS is already WGS84 (EPSG:4326). Skipping BBox transformation.")
            wgs84_bbox = [
                ctx.bounds.min_x, 
                ctx.bounds.min_y, 
                ctx.bounds.max_x, 
                ctx.bounds.max_y
            ]

        # Construct the WCS GetCoverage payload
        params = {
            "service": "WCS",
            "version": "1.0.0",
            "request": "GetCoverage",
            "coverage": "emodnet:mean", 
            "crs": "EPSG:4326",
            "bbox": f"{wgs84_bbox[0]},{wgs84_bbox[1]},{wgs84_bbox[2]},{wgs84_bbox[3]}",
            "format": "image/tiff",
            "interpolation": "bilinear",
            "resx": "0.00208333",  # EMODnet native resolution in degrees (~250m)
            "resy": "0.00208333" 
        }

        logger.info(f"Requesting bathymetry from EMODnet | BBox: {wgs84_bbox}")

        # Fetch the raster data using the resilient retry mechanism
        content = fetch_with_retries(cls.EMODNET_WCS_URL, params=params, timeout=60)

        # Validate that content was successfully retrieved
        if not content:
            error_msg = "Failed to download bathymetry from EMODnet after multiple retries."
            logger.error(error_msg)
            raise Exception(error_msg)

        # Load the raw TIFF byte stream into a StaticRaster, casting values to float32
        return load_raster_from_bytes([content], data_type=np.float32)