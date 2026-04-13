import time
import random
from typing import Any, Dict, Optional

import requests
from loguru import logger


def fetch_with_retries(
    url: str, 
    params: Dict[str, Any], 
    headers: Optional[Dict[str, str]] = None, 
    max_retries: int = 3, 
    timeout: int = 60
) -> Optional[bytes]:
    """Fetches data from an HTTP endpoint using exponential backoff for retries.

    This function attempts to make an HTTP GET request to the specified URL.
    It handles transient network errors and server-side errors (5xx) by
    retrying the request with an exponential backoff strategy. It also
    detects silent errors commonly returned by GeoServer/MapServer implementations.

    Args:
        url (str): The endpoint URL to fetch data from.
        params (Dict[str, Any]): The query parameters to include in the request.
        headers (Optional[Dict[str, str]], optional): HTTP headers to include. Defaults to None.
        max_retries (int, optional): The maximum number of retry attempts. Defaults to 3.
        timeout (int, optional): The timeout for the request in seconds. Defaults to 60.

    Returns:
        bytes: The raw byte content of the successful response.

    Raises:
        Exception: If all retry attempts fail.
        Exception: If the server returns an 'ExceptionReport' within a 200 OK response.
        requests.exceptions.HTTPError: If a non-recoverable HTTP error (e.g., 4xx) occurs.
        requests.exceptions.RequestException: For any other unhandled request-related errors.
    """
    for attempt in range(max_retries):
        try:
            response = requests.get(url, params=params, headers=headers, timeout=timeout)
            response.raise_for_status()
            
            # Validate silent errors (common in GeoServer/MapServer WFS/WCS responses)
            if b'ExceptionReport' in response.content:
                error_snippet = response.text[:200]
                logger.error(f"Server returned a silent exception: {error_snippet}")
                raise Exception(f"Exception detected in response: {error_snippet}")
                
            return response.content

        except requests.exceptions.HTTPError as e:
            # Safely get the status code from the exception object
            status_code = e.response.status_code if e.response is not None else 0

            if status_code in (500, 502, 503, 504):
                logger.warning(
                    f"Server error {status_code} at {url}. Retry {attempt + 1}/{max_retries}..."
                )
            else:
                logger.error(f"Unrecoverable HTTP error: {e}")
                raise

        except requests.exceptions.ConnectionError as e:
            logger.warning(f"Connection failed: {e}. Retry {attempt + 1}/{max_retries}...")

        except requests.exceptions.RequestException as e:
            logger.error(f"Unexpected request error: {e}")
            raise

        # Exponential backoff with jitter (only reached if a retryable exception was caught)
        if attempt < max_retries - 1:
            sleep_time = (2 ** attempt) + random.uniform(0, 1)
            time.sleep(sleep_time)
            
    # If the loop finishes without returning or raising a non-retryable exception
    logger.error(f"Exceeded limit of {max_retries} retries for URL: {url}")
    raise Exception(f"Failed to fetch {url} after {max_retries} attempts.")