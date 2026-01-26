import os
import re
import json
import csv
import io
import math
import requests
import pdfplumber
from datetime import datetime, timedelta
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass

from sim_data_scripts.rain_on_grid.misc.types import StationMetadata, StationRainData
from sim_data_scripts.rain_on_grid.extraction.base import BaseFetcher

@dataclass
class PdfLine:
    station_name: str
    value: float

class SAIHJucarFetcher(BaseFetcher):
    """
    Fetcher for 'Confederación Hidrográfica del Júcar'.
    Parses HTML for station coordinates and PDFs for rainfall data.
    """
    
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    BASE_URL = "https://saih.chj.es"
    CSV_PATH = os.path.join(BASE_DIR, "jucar_stations.csv")
    MAPPING_PATH = os.path.join(BASE_DIR, "station_pdf_mapping.json")

    def __init__(self):
        self.session = requests.Session() # Optimization: Reuse TCP connection
        
        # Regex to parse PDF lines: "STATION NAME 12.5"
        self.line_regex = re.compile(r"^(.+)\s(\d+\.\d+)$")

    def fetch_stations(self) -> List[StationMetadata]:
        """Loads stations from CSV, checks web for updates, and merges."""
        existing_stations = self._load_csv_stations()
        
        print(f"[{self.__class__.__name__}] Checking for station updates...")
        try:
            resp = self.session.get(f"{self.BASE_URL}/mapa-lluvias", timeout=10)
            if resp.status_code == 200:
                web_stations = self._parse_web_stations(resp.text)
                merged = self._merge_stations(existing_stations, web_stations)
                
                # Update CSV if new stations found
                if len(merged) > len(existing_stations):
                    self._save_csv_stations(list(merged.values()))
                return list(merged.values())
        except Exception as e:
            print(f"[{self.__class__.__name__}] Web update failed ({e}). Using cached CSV.")
            
        return list(existing_stations.values())

    def fetch_data(self, stations: List[StationMetadata], start: datetime, end: datetime) -> List[StationRainData]:
        """
        Downloads daily PDFs and extracts data.
        Smartly skips days if a multi-day accumulation report is found.
        """
        pdf_map = self._load_mapping()
        data_points = []
        
        # Jucar reports are usually available around 08:00
        curr_date = end.replace(hour=8, minute=0, second=0, microsecond=0)
        if end > curr_date: curr_date += timedelta(days=1)
        
        stop_date = start.replace(hour=8, minute=0, second=0, microsecond=0)
        if start < stop_date: stop_date -= timedelta(days=1)

        print(f"[{self.__class__.__name__}] Downloading reports from {stop_date} to {curr_date}")

        while curr_date >= stop_date:
            # Try to find a report for this date (checking periods 1..10)
            points, found_period = self._process_date(curr_date, stations, pdf_map)
            
            if points and found_period > 0:
                print(f"   -> Found {len(points)} points for {curr_date.date()} (Period: {found_period} days)")
                data_points.extend(points)
                
                # OPTIMIZATION: Skip the days covered by this accumulation report
                curr_date -= timedelta(days=found_period)
            else:
                # No data found for this specific date, step back 1 day and retry
                print(f"   -> No data for {curr_date.date()}")
                curr_date -= timedelta(days=1)
            
        return data_points

        # --- Internal Helpers ---

    def _process_date(self, date: datetime, stations: List[StationMetadata], pdf_map: Dict) -> Tuple[List[StationRainData], int]:
        """
        Attempts to download and parse a PDF for a specific date.
        
        Returns:
            Tuple containing:
            1. List of extracted StationRainData.
            2. The period (int) of the valid report found (e.g., 1, 3, 7). Returns 0 if none found.
        """
        # Try periods 1 to 10 days (Jucar naming convention)
        for period in range(1, 11):
            # Filename format: YYYYMMDDPLU[period if > 1]
            fname = f"{date.strftime('%Y%m%d')}PLU" + ("" if period == 1 else str(period))
            url = f"{self.BASE_URL}/docs/{fname}.pdf"

            try:
                resp = self.session.get(url, timeout=10)
                if resp.status_code == 200:
                    data = self._parse_pdf(resp.content, date, period, stations, pdf_map)
                    return data, period
                elif resp.status_code == 404:
                    continue # Try next period
            except Exception as e:
                print(f"[Warn] Failed processing {fname}: {e}")
        
        return [], 0

    def _parse_pdf(self, content: bytes, report_date: datetime, period: int, 
                   stations: List[StationMetadata], mapping: Dict) -> List[StationRainData]:
        
        extracted_lines = []
        with pdfplumber.open(io.BytesIO(content)) as pdf:
            for page in pdf.pages:
                text = page.extract_text()
                if not text: continue
                for raw_line in text.split('\n'):
                    match = self.line_regex.match(raw_line.strip())
                    if match:
                        extracted_lines.append(PdfLine(match.group(1).strip().upper(), float(match.group(2))))

        results = []
        start_ts = report_date - timedelta(days=period)
        
        for st in stations:
            valid_names = mapping.get(st.station_id, [])
            for line in extracted_lines:
                if any(name in line.station_name for name in valid_names):
                    results.append(StationRainData(
                        station=st,
                        mm_per_hour=line.value,
                        start_timestamp=start_ts,
                        end_timestamp=report_date
                    ))
        
        return results

    def _load_csv_stations(self) -> Dict[str, StationMetadata]:
        if not os.path.exists(self.CSV_PATH): return {}
        try:
            stations = {}
            with open(self.CSV_PATH, 'r', encoding='utf-8') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    try:
                        row['x'] = float(row['x'])
                        row['y'] = float(row['y'])
                        
                        stations[row['station_id']] = StationMetadata(**row)
                    except ValueError:
                        continue
            return stations
        except Exception as e:
            print(f"[Error] Loading CSV stations: {e}")
            return {}

    def _save_csv_stations(self, stations: List[StationMetadata]):
        with open(self.CSV_PATH, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=['station_id', 'x', 'y', 'location', 'province'])
            writer.writeheader()
            for s in sorted(stations, key=lambda x: x.station_id):
                writer.writerow(s.__dict__)

    def _load_mapping(self) -> Dict[str, List[str]]:
        if os.path.exists(self.MAPPING_PATH):
            with open(self.MAPPING_PATH, 'r', encoding='utf-8') as f:
                return json.load(f)
        return {}

    def _parse_web_stations(self, html: str) -> Dict[str, StationMetadata]:
        """Extracts JS array `let estaciones = [...]` from HTML."""
        match = re.search(r'let\s+estaciones\s*=\s*(\[.*?\]);', html, re.DOTALL)
        found = {}
        if match:
            data = json.loads(match.group(1))
            for item in data:
                try:
                    s = StationMetadata(
                        station_id=item["fldTNombre"],
                        x=float(item["fldNCoordGPSLat"]),
                        y=float(item["fldNCoordGPSLon"]),
                        location=item.get("fldTPoblacion", ""),
                        province=item.get("fldTProvincia", "")
                    )
                    found[s.station_id] = s
                except (ValueError, KeyError):
                    continue
        return found

    def _merge_stations(self, old: Dict, new: Dict) -> Dict:
        """Merges new web data into old CSV data, prioritizing CSV coordinates on conflict."""
        merged = old.copy()
        for sid, s_new in new.items():
            if sid not in merged:
                merged[sid] = s_new
        return merged