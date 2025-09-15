#!/usr/bin/env python3
"""
Configuration management for AnimHost ML Framework
Handles loading and validation of training configurations
"""

import json
import logging
from pathlib import Path

from config.model_configs import StarkeModelConfig


logger = logging.getLogger(__name__)


class ConfigManager:
    """Manages configuration loading and validation"""
    
    # Static config directory - computed once at module load
    config_dir = Path(__file__).parent.parent
    
    @staticmethod
    def load_config(config_name: str = 'starke_model_config.json') -> StarkeModelConfig:
        """
        Load and validate configuration
        
        Args:
            config_name: Name of the config file to load
            
        Returns:
            StarkeModelConfig: Loaded and validated configuration
            
        Raises:
            FileNotFoundError: If config file doesn't exist
            ValueError: If config is invalid
            json.JSONDecodeError: If config file is malformed
        """
        config_path = ConfigManager.config_dir / config_name
        logger.info(f"Loading configuration from: {config_path}")
        
        # Check config file exists
        if not config_path.exists():
            error_msg = f"Configuration file not found: {config_path}"
            logger.error(error_msg)
            raise FileNotFoundError(error_msg)
        
        # Load JSON config
        try:
            with open(config_path, 'r') as f:
                config_data = json.load(f)
            logger.info("Configuration file loaded successfully")
        except json.JSONDecodeError as e:
            error_msg = f"Invalid JSON in config file {config_path}: {e}"
            logger.error(error_msg)
            raise json.JSONDecodeError(error_msg, e.doc, e.pos)
        
        # Create config object
        try:
            config = StarkeModelConfig(**config_data)
            logger.info("StarkeModelConfig created successfully")
        except TypeError as e:
            error_msg = f"Invalid config structure: {e}"
            logger.error(error_msg)
            raise ValueError(error_msg)
        
        # Validate config
        validation_error = config.validate()
        if validation_error:
            logger.error(f"Configuration validation failed: {validation_error}")
            raise ValueError(validation_error)
        
        logger.info("Configuration validation successful")
        return config