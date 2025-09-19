#!/usr/bin/env python3
"""Test script to demonstrate texture regeneration with a short interval."""

import os
import subprocess
import sys
from pathlib import Path


def main():
    """Run texture regeneration test with 10-second intervals."""
    # Check if API key is set
    api_key = os.getenv("GOOGLE_API_KEY")
    if not api_key:
        print("ERROR: GOOGLE_API_KEY environment variable is required")
        print("Set it with: $env:GOOGLE_API_KEY = 'your-api-key-here'")
        sys.exit(1)
    
    print("=== Simplified Texture Regeneration Test ===")
    print("This will regenerate textures every 10 seconds while DirectX app is running")
    print("The script will stop when you close the DirectX application")
    print("Or press Ctrl+C to stop manually")
    print()
    
    # Path to the Python executable in venv
    python_exe = Path(".venv/Scripts/python.exe")
    if not python_exe.exists():
        print(f"ERROR: Python executable not found at {python_exe}")
        sys.exit(1)
    
    # Command to run
    cmd = [
        str(python_exe),
        "main.py",
        "--assets-path", "assets",
        "--exe-path", "DirectX12Triangle.exe",
        "--regeneration-interval", "10"
    ]
    
    try:
        # Run the main texture manager
        subprocess.run(cmd, check=True)
    except KeyboardInterrupt:
        print("\nTest stopped by user")
    except subprocess.CalledProcessError as e:
        print(f"Error running texture manager: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()