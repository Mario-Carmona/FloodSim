"""
Meteorological data extractor for the SAIH Júcar basin.

This module retrieves station metadata from the official HTML portal and 
extracts hourly precipitation data by downloading and parsing PDF reports.
"""

import csv
import io
import json
import math
import os
import re
from dataclasses import dataclass
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Tuple

import pdfplumber
import requests
from loguru import logger
from pyproj import CRS

from generators.layers.rainfall.extraction.base import BaseFetcher
from generators.layers.rainfall.types import StationMetadata, StationRainData
from utils.types import Position


@dataclass
class PdfLine:
    """
    Represents a parsed line of precipitation data from a PDF report.

    Attributes:
        station_name (str): The name of the station as it appears in the PDF.
        value (float): The recorded rainfall value (e.g., mm/h).
    """
    station_name: str
    value: float


class SAIHJucarFetcher(BaseFetcher):
    """
    Data fetcher for the 'Confederación Hidrográfica del Júcar' (SAIH Júcar).

    This class combines HTML scraping for station discovery with PDF parsing 
    for historical rainfall data retrieval. It uses auxiliary files to standardise 
    coordinates and map PDF names to station IDs.

    Attributes:
        BASE_DIR (str): The directory containing this script.
        BASE_URL (str): The base URL for the SAIH Júcar web portal.
        CSV_PATH (str): Path to the locally validated CSV with station coordinates.
        MAPPING_PATH (str): Path to the JSON mapping file for station names.
        SAIH_JUCAR_CRS (CRS): The expected Coordinate Reference System (EPSG:25830).
        session (requests.Session): Active HTTP session for connection pooling.
        pdf_line_regex (re.Pattern): Regular expression to extract values from PDF text.
        csv_stations (Dict[str, StationMetadata]): Stations loaded from the local CSV.
        mapping (Dict[str, List[str]]): Loaded JSON name mapping.
    """
    
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    BASE_URL = "https://saih.chj.es"
    CSV_PATH = os.path.join(BASE_DIR, "jucar_stations.csv")
    MAPPING_PATH = os.path.join(BASE_DIR, "station_pdf_mapping.json")

    # Base Coordinate Reference System for SAIH Jucar (UTM Zone 30N, ETRS89)
    SAIH_JUCAR_CRS = CRS.from_epsg(25830)

    def __init__(self) -> None:
        """
        Initializes the fetcher, configures the HTTP session, and pre-loads 
        local auxiliary data (CSV and JSON mapping).
        """
        self.session = requests.Session() # Optimization: Reuse TCP connection
        
        # Regex to parse PDF lines: "STATION NAME 12.5"
        self.pdf_line_regex = re.compile(r"^(.+)\s(\d+\.\d+)$")
        
        logger.debug("Initializing SAIHJucarFetcher and loading local assets.")
        self.csv_stations: Dict[str, StationMetadata] = self._load_csv_stations()
        self.mapping: Dict[str, List[str]] = self._load_mapping()

    def _load_csv_stations(self) -> Dict[str, StationMetadata]:
        """
        Loads the locally validated station coordinates from the CSV file.

        Reads the static 'jucar_stations.csv' file and converts each row into a 
        `StationMetadata` object containing its geographical coordinates and EPSG code.

        Returns:
            Dict[str, StationMetadata]: A dictionary mapping the unique `station_id` 
                to its corresponding `StationMetadata` instance.
        """

        stations: Dict[str, StationMetadata] = {}
        
        if not os.path.exists(self.CSV_PATH):
            logger.warning(f"Local CSV stations file not found at: {self.CSV_PATH}")
            return stations

        logger.debug(f"Loading local stations from CSV: {self.CSV_PATH}")
        with open(self.CSV_PATH, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                s = StationMetadata(
                    station_id=row['station_id'],
                    position=Position(
                        x=float(row['x']),
                        y=float(row['y']),
                        crs=CRS.from_epsg(int(row['epsg']))
                    ),
                    location=row['location'],
                    province=row['province']
                )
                stations[s.station_id] = s
                
        logger.debug(f"Loaded {len(stations)} stations from CSV.")
        return stations

    def _load_mapping(self) -> Dict[str, List[str]]:
        """
        Loads the JSON mapping configuration for PDF station names.

        The PDF reports often contain truncated or alternative names for the stations.
        This method loads a JSON dictionary that maps the official station ID to a 
        list of possible names found in the PDFs.

        Returns:
            Dict[str, List[str]]: A dictionary where keys are the official `station_id`s 
                and values are lists of string aliases. Returns an empty dict if the 
                file does not exist.
        """
        if not os.path.exists(self.MAPPING_PATH):
            logger.warning(f"JSON mapping file not found at: {self.MAPPING_PATH}")
            return {}

        logger.debug(f"Loading PDF name mappings from: {self.MAPPING_PATH}")
        with open(self.MAPPING_PATH, 'r', encoding='utf-8') as f:
            mapping_data = json.load(f)
            return mapping_data

    def _parse_web_stations(self, html: str) -> Dict[str, StationMetadata]:
        """
        Extracts station data from the embedded JavaScript in the SAIH HTML portal.

        The official webpage loads station coordinates dynamically via a JavaScript
        variable `let estaciones = [...]`. This method uses a regular expression
        to isolate that JSON array and parses it into `StationMetadata` objects.

        Args:
            html (str): The raw HTML content of the target webpage.

        Returns:
            Dict[str, StationMetadata]: A dictionary mapping extracted station IDs 
                to their metadata. Malformed or unparseable entries are silently skipped.
        """
        match = re.search(r'let\s+estaciones\s*=\s*(\[.*?\]);', html, re.DOTALL)
        found: Dict[str, StationMetadata] = {}

        if match:
            logger.debug("JavaScript station array found in HTML. Parsing JSON...")
            try:
                data = json.loads(match.group(1))
                for item in data:
                    try:
                        s = StationMetadata(
                            station_id=item["fldTNombre"],
                            position=Position(
                                x=float(item["fldNCoordGPSLat"]),
                                y=float(item["fldNCoordGPSLon"]),
                                crs=self.SAIH_JUCAR_CRS
                            ),
                            location=item.get("fldTPoblacion", ""),
                            province=item.get("fldTProvincia", "")
                        )
                        found[s.station_id] = s
                    except (ValueError, KeyError):
                        # Skip entries that don't have the expected format or valid coordinates
                        continue
                logger.debug(f"Successfully parsed {len(found)} stations from web HTML.")
            except json.JSONDecodeError as e:
                logger.error(f"Failed to decode embedded JSON from web: {e}")
        else:
            logger.warning("Could not locate the 'estaciones' JavaScript array in the provided HTML.")

        return found

    def _merge_stations(
        self, 
        old: Dict[str, StationMetadata], 
        new: Dict[str, StationMetadata]
    ) -> Dict[str, StationMetadata]:
        """
        Merges dynamically fetched web stations with the local CSV stations.

        This method acts as a conflict resolver: it prioritizes the locally stored 
        coordinates (which might have been manually corrected) over the newly 
        fetched web coordinates. Only entirely new stations from the web are added.

        Args:
            old (Dict[str, StationMetadata]): The locally validated stations.
            new (Dict[str, StationMetadata]): The stations newly scraped from the web.

        Returns:
            Dict[str, StationMetadata]: A unified dictionary containing all stations.
        """
        merged = old.copy()

        for sid, s_new in new.items():
            if sid not in merged:
                merged[sid] = s_new

        if len(merged) > len(old):
            logger.info(f"Merged {len(merged) - len(old)} new stations from the web into the local dataset.")
        else:
            logger.debug("No new stations found on the web to merge.")

        return merged

    def _save_csv_stations(self, stations: Dict[str, StationMetadata]) -> None:
        """
        Saves the unified list of stations back to the local CSV file.

        This ensures that any newly discovered stations from the web portal 
        are permanently stored locally, reducing the need to rely purely on 
        web scraping in future executions and allowing manual coordinate corrections.

        Args:
            stations (Dict[str, StationMetadata]): The complete dictionary of 
                stations to save.
        """
        logger.debug(f"Saving {len(stations)} stations to local CSV: {self.CSV_PATH}")
        try:
            with open(self.CSV_PATH, 'w', newline='', encoding='utf-8') as f:
                # We define the headers
                fieldnames = ['station_id', 'x', 'y', 'epsg', 'location', 'province']
                writer = csv.DictWriter(f, fieldnames=fieldnames)

                writer.writeheader()
                for s in stations.values():
                    writer.writerow({
                        'station_id': s.station_id,
                        'x': s.position.x,
                        'y': s.position.y,
                        'epsg': s.position.crs.to_epsg(),
                        'location': s.location,
                        'province': s.province,
                    })
            logger.success("Local CSV stations file updated successfully.")
        except Exception as e:
            logger.error(f"Failed to save stations to CSV: {e}")

    def fetch_stations(self) -> List[StationMetadata]:
        """
        Retrieves, merges, and returns all available meteorological stations.

        This method orchestrates the station discovery process by:
        1. Fetching the dynamic station list from the SAIH Júcar web portal.
        2. Merging the newly discovered web stations with the locally validated ones.
        3. Saving the updated list back to the local CSV if new stations were found.

        Returns:
            List[StationMetadata]: A complete list of all active stations, ready
                for spatial filtering and data extraction.
        """
        logger.info(f"[{self.__class__.__name__}] Fetching stations from SAIH Júcar...")
        
        web_stations: Dict[str, StationMetadata] = {}
        try:
            # Request the main rain gauge page
            res = self.session.get(f"{self.BASE_URL}/mapa-lluvias")
            res.raise_for_status()
            web_stations = self._parse_web_stations(res.text)
        except requests.RequestException as e:
            logger.error(f"Error fetching web stations from SAIH Júcar portal: {e}")
           
        # Combine local CSV data with fresh web data
        merged = self._merge_stations(self.csv_stations, web_stations)
        
        # If the merged dictionary has more entries than the local one, save it
        if len(merged) > len(self.csv_stations):
            logger.info("New stations detected. Updating local CSV file...")
            self._save_csv_stations(merged)
        else:
            logger.debug("No new stations to save.")
            
        logger.info(f"[{self.__class__.__name__}] Total stations ready: {len(merged)}")
        return list(merged.values())

    def _process_date(
        self, 
        date: datetime, 
        stations: List[StationMetadata]
    ) -> Tuple[List[StationRainData], int]:
        """
        Attempts to download and parse a precipitation PDF report for a specific date.

        The SAIH Júcar portal stores accumulation reports that can span multiple days 
        (from 1 to 10 days). This method attempts to find the most comprehensive 
        report available for the given date by testing different period suffixes.

        Args:
            date (datetime): The target date for which to find a report.
            stations (List[StationMetadata]): The list of relevant stations.

        Returns:
            Tuple[List[StationRainData], int]: A tuple containing:
                1. A list of extracted rainfall data objects.
                2. The period (in days) covered by the found report (0 if none found).
        """
        # Try periods 1 to 10 days (Jucar naming convention)
        for period in range(1, 11):
            # Filename format: YYYYMMDDPLU[period if > 1]
            filename = f"{date.strftime('%Y%m%d')}PLU" + ("" if period == 1 else str(period))
            url = f"{self.BASE_URL}/docs/{filename}.pdf"

            try:
                res = self.session.get(url, timeout=10)
                if res.status_code == 200:
                    logger.debug(f"Successfully downloaded report: {filename}.pdf")
                    data = self._parse_pdf(res.content, date, period, stations)
                    return data, period
                elif res.status_code == 404:
                    continue  # Try next period
            except Exception as e:
                logger.warning(f"Failed processing {filename}.pdf: {e}")
        
        return [], 0

    def _parse_pdf(
        self, 
        content: bytes, 
        report_date: datetime, 
        period: int, 
        stations: List[StationMetadata]
    ) -> List[StationRainData]:
        """
        Parses the raw bytes of a downloaded PDF to extract precipitation values.

        Args:
            content (bytes): The raw binary content of the PDF file.
            report_date (datetime): The reference date of the report.
            period (int): The number of days the report covers.
            stations (List[StationMetadata]): The active stations to look for.

        Returns:
            List[StationRainData]: A list containing the parsed rainfall data.
        """
        extracted_lines = []
        with pdfplumber.open(io.BytesIO(content)) as pdf:
            for page in pdf.pages:
                text = page.extract_text()
                if not text: 
                    continue
                for raw_line in text.split('\n'):
                    match = self.pdf_line_regex.match(raw_line.strip())
                    if match:
                        extracted_lines.append(PdfLine(match.group(1).strip().upper(), float(match.group(2))))

        results = []
        start_ts = report_date - timedelta(days=period)
        
        for st in stations:
            valid_names = self.mapping.get(st.station_id, [])
            for line in extracted_lines:
                if any(name in line.station_name for name in valid_names):
                    results.append(StationRainData(
                        station=st,
                        mm_per_hour=line.value,
                        start_timestamp=start_ts,
                        end_timestamp=report_date
                    ))
        
        return results

    def fetch_data(
        self, 
        stations: List[StationMetadata], 
        start_date: datetime, 
        end_date: datetime
    ) -> List[StationRainData]:
        """
        Downloads daily PDFs and extracts precipitation data for the given time window.

        This method dynamically traverses the timeline backwards from the end date. 
        It smartly skips days if a multi-day accumulation report is found, avoiding 
        redundant downloads and data duplication.

        Args:
            stations (List[StationMetadata]): The stations to extract data for.
            start_date (datetime): The beginning of the extraction window.
            end_date (datetime): The end of the extraction window.

        Returns:
            List[StationRainData]: A collection of all extracted rainfall measurements.
        """
        data_points = []
        
        # Jucar reports are usually available around 08:00
        curr_date = end_date.replace(hour=8, minute=0, second=0, microsecond=0)
        if end_date > curr_date: 
            curr_date += timedelta(days=1)
        
        stop_date = start_date.replace(hour=8, minute=0, second=0, microsecond=0)
        if start_date < stop_date: 
            stop_date -= timedelta(days=1)

        logger.info(f"[{self.__class__.__name__}] Downloading reports from {stop_date.date()} to {curr_date.date()}")

        while curr_date >= stop_date:
            # Try to find a report for this date (checking periods 1..10)
            points, found_period = self._process_date(curr_date, stations)
            
            if points and found_period > 0:
                logger.debug(f"-> Found {len(points)} points for {curr_date.date()} (Period: {found_period} days)")
                data_points.extend(points)
                
                # OPTIMIZATION: Skip the days covered by this accumulation report
                curr_date -= timedelta(days=found_period)
            else:
                # No data found for this specific date, step back 1 day and retry
                logger.warning(f"-> No data found for {curr_date.date()}")
                curr_date -= timedelta(days=1)
            
        return data_points