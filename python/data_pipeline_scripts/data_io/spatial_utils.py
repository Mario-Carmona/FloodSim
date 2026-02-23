"""
Spatial utility functions for the flood simulator pipeline.

This module provides helper functions to validate spatial constraints,
such as ensuring that source raster layers completely cover the required
target execution area.
"""

from rasterio.transform import array_bounds
from rasterio.warp import transform_bounds
from loguru import logger

from misc.types import SpatialContext


def validate_spatial_coverage(src_ctx: SpatialContext, tgt_ctx: SpatialContext) -> None:
    """
    Verifies that the source layer completely covers the target layer's spatial extent.

    Args:
        src_ctx (SpatialContext): The spatial context of the source data layer.
        tgt_ctx (SpatialContext): The spatial context of the target/master layer.

    Raises:
        ValueError: If the source layer does not fully encompass the target area.
    """
    logger.debug("Validating spatial coverage between source and target contexts.")
    
    tgt_bounds = array_bounds(tgt_ctx.height, tgt_ctx.width, tgt_ctx.transform)
    src_bounds = array_bounds(src_ctx.height, src_ctx.width, src_ctx.transform)

    tgt_bounds_in_src_crs = transform_bounds(tgt_ctx.crs, src_ctx.crs, *tgt_bounds)
    
    tol = 1e-5
    # Check if target boundaries exceed source boundaries
    if (src_bounds[0] > tgt_bounds_in_src_crs[0] + tol or  
        src_bounds[1] > tgt_bounds_in_src_crs[1] + tol or  
        src_bounds[2] < tgt_bounds_in_src_crs[2] - tol or  
        src_bounds[3] < tgt_bounds_in_src_crs[3] - tol):   
        
        logger.error("Spatial coverage validation failed. Source layer is too small.")
        raise ValueError(
            "Spatial Error: The input layer does not completely cover the "
            "extent of the master layer. Operation aborted."
        )