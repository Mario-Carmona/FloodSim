import argparse
import sys
from pathlib import Path
from loguru import logger

from pipeline import DataPipeline


def main() -> None:
    """
    Main entry point for the flood simulator data pipeline.

    Parses command-line arguments to obtain the configuration file path,
    initializes the DataPipeline, and executes it.
    """
    # Loguru configuration: Clean format for console output
    logger.remove()
    logger.add(
        sys.stdout, 
        format="<green>{time:YYYY-MM-DD HH:mm:ss}</green> | <level>{level: <8}</level> | <level>{message}</level>",
        level="INFO"
    )

    parser = argparse.ArgumentParser(
        description="Tool to extract and process data for a specific geographical area."
    )

    parser.add_argument(
        "config_path", 
        type=str,
        help="Path to the .yaml configuration file."
    )

    args = parser.parse_args()
    cfg_path = Path(args.config_path).resolve()

    if not cfg_path.exists():
        logger.error(f"Configuration file not found: {cfg_path}")
        sys.exit(1)

    try:
        logger.info(f"Initializing Data Pipeline using config: {cfg_path.name}")
        pipeline = DataPipeline(cfg_path)
        pipeline.run()
    except Exception as e:
        logger.exception("A critical error occurred during pipeline execution.")
        sys.exit(1)


if __name__ == "__main__":
    main()