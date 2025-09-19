"""Texture management system for DirectX application with AI-powered texture regeneration."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import time
from datetime import datetime
from pathlib import Path
from typing import Dict, Optional

from PIL import Image
from image_gen import describe_image, generate_image_from_text


class TextureManager:
    """Manages texture analysis, backup, and AI-powered regeneration for DirectX applications."""
    
    def __init__(self, assets_path: Path, api_key: str, exe_path: Path, force_reanalyze: bool = False):
        self.assets_path = Path(assets_path)
        self.api_key = api_key
        self.exe_path = Path(exe_path)
        self.force_reanalyze = force_reanalyze
        self.texture_data: Dict[str, Dict] = {}
        self.backup_root = Path("texture_backups")  # Move backups outside assets folder
        self.supported_extensions = {'.png', '.jpg', '.jpeg'}
        self.directx_process = None
        
    def find_texture_files(self) -> list[Path]:
        """Scan assets folder for texture files (PNG/JPG)."""
        texture_files = []
        for file_path in self.assets_path.rglob("*"):
            if file_path.is_file() and file_path.suffix.lower() in self.supported_extensions:
                texture_files.append(file_path)
        return texture_files
    
    def get_image_metadata(self, image_path: Path) -> Dict:
        """Extract image metadata using PIL."""
        try:
            with Image.open(image_path) as img:
                return {
                    'width': img.width,
                    'height': img.height,
                    'channels': len(img.getbands()),
                    'mode': img.mode,
                    'format': img.format,
                    'size_bytes': image_path.stat().st_size
                }
        except Exception as e:
            print(f"Error reading metadata for {image_path}: {e}")
            return {'width': 0, 'height': 0, 'channels': 0, 'mode': 'unknown', 'format': 'unknown', 'size_bytes': 0}
    
    def analyze_texture(self, texture_path: Path) -> Dict:
        """Analyze a single texture file using Gemini."""
        print(f"Analyzing texture: {texture_path.name}")
        
        # Get image metadata
        metadata = self.get_image_metadata(texture_path)
        
        # Get AI description
        try:
            description = describe_image(str(texture_path), api_key=self.api_key, is_texture=True)
        except Exception as e:
            print(f"Failed to analyze {texture_path}: {e}")
            description = {
                'summary': f"Texture file: {texture_path.name}",
                'caption': "Unknown texture",
                'description': f"Could not analyze texture {texture_path.name}",
                'sentiment': 'neutral'
            }
        
        return {
            'path': str(texture_path),
            'relative_path': str(texture_path.relative_to(self.assets_path)),
            'metadata': metadata,
            'ai_description': description,
            'last_analyzed': datetime.now().isoformat(),
        }
    
    def analyze_all_textures(self) -> None:
        """Analyze all texture files in the assets folder."""
        analysis_file = self.assets_path / "texture_analysis.json"
        
        # Check if analysis file already exists and we're not forcing re-analysis
        if analysis_file.exists() and not self.force_reanalyze:
            print(f"Found existing analysis file: {analysis_file}")
            try:
                with open(analysis_file, 'r', encoding='utf-8') as f:
                    self.texture_data = json.load(f)
                print(f"Loaded analysis for {len(self.texture_data)} textures from existing file")
                print("Use --force-reanalyze to regenerate the analysis")
                return
            except (json.JSONDecodeError, IOError) as e:
                print(f"Error reading existing analysis file: {e}")
                print("Proceeding with fresh analysis...")
        
        if self.force_reanalyze and analysis_file.exists():
            print("Forcing re-analysis of textures...")
        
        print("Starting texture analysis...")
        texture_files = self.find_texture_files()
        print(f"Found {len(texture_files)} texture files")
        
        for texture_file in texture_files:
            analysis = self.analyze_texture(texture_file)
            relative_path = str(texture_file.relative_to(self.assets_path))
            self.texture_data[relative_path] = analysis
        
        # Save analysis results
        with open(analysis_file, 'w', encoding='utf-8') as f:
            json.dump(self.texture_data, f, indent=2, ensure_ascii=False)
        
        print(f"Analysis complete. Results saved to {analysis_file}")
    
    def create_backup(self, texture_path: Path) -> Path:
        """Create a timestamped backup of a texture file."""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_dir = self.backup_root / timestamp
        backup_dir.mkdir(parents=True, exist_ok=True)
        
        relative_path = texture_path.relative_to(self.assets_path)
        backup_path = backup_dir / relative_path
        backup_path.parent.mkdir(parents=True, exist_ok=True)
        
        shutil.copy2(texture_path, backup_path)
        return backup_path
    
    def regenerate_texture(self, texture_info: Dict) -> bool:
        """Regenerate a single texture using Gemini."""
        try:
            original_path = Path(texture_info['path'])
            metadata = texture_info['metadata']
            description = texture_info['ai_description']
            
            print(f"Regenerating: {original_path.name}")
            
            # Create backup
            backup_path = self.create_backup(original_path)
            print(f"Backed up to: {backup_path}")
            
            # Generate new texture using the AI description
            prompt = f"{description['description']} {description['caption']}"
            
            temp_output = original_path.parent / f"temp_{original_path.name}"
            
            generate_image_from_text(
                prompt=prompt,
                width=metadata['width'],
                height=metadata['height'],
                output_path=str(temp_output),
                api_key=self.api_key,
                is_texture=True
            )
            
            # Replace original with generated texture
            if temp_output.exists():
                shutil.move(temp_output, original_path)
                print(f"✓ Regenerated: {original_path.name}")
                return True
            else:
                print(f"✗ Failed to generate: {original_path.name}")
                return False
                
        except Exception as e:
            print(f"Error regenerating {texture_info.get('path', 'unknown')}: {e}")
            return False
    
    def regenerate_all_textures(self) -> None:
        """Regenerate all analyzed textures."""
        print("Starting texture regeneration...")
        
        success_count = 0
        total_count = len(self.texture_data)
        
        for relative_path, texture_info in self.texture_data.items():
            if self.regenerate_texture(texture_info):
                success_count += 1
        
        print(f"Regeneration complete: {success_count}/{total_count} textures regenerated")
        
        # Create update flag file
        self.create_update_flag()
    
    def create_update_flag(self) -> None:
        """Create UpdateTexture.txt flag file to signal DirectX application."""
        flag_file = self.assets_path / "UpdateTexture.txt"
        timestamp = datetime.now().isoformat()
        with open(flag_file, 'w') as f:
            f.write(f"Textures updated at: {timestamp}\n")
        print(f"Created update flag: {flag_file}")
    
    def launch_directx_app(self) -> None:
        """Launch the DirectX application."""
        try:
            print(f"Launching DirectX application: {self.exe_path}")
            self.directx_process = subprocess.Popen([str(self.exe_path)], cwd=self.exe_path.parent)
            print("DirectX application launched successfully")
        except Exception as e:
            print(f"Failed to launch DirectX application: {e}")
    
    def is_directx_app_running(self) -> bool:
        """Check if the DirectX application is still running."""
        if self.directx_process is None:
            return False
        return self.directx_process.poll() is None
    
    def run(self, regeneration_interval: Optional[float] = None) -> None:
        """Main execution flow."""
        print("=== DirectX Texture Manager ===")
        
        # Step 1: Analyze all textures
        self.analyze_all_textures()
        
        # Step 2: Launch DirectX application
        self.launch_directx_app()
        
        # Step 3: Simple periodic regeneration loop if requested
        if regeneration_interval and regeneration_interval > 0:
            print(f"Starting periodic texture regeneration (interval: {regeneration_interval}s)")
            print("The script will regenerate textures while the DirectX app is running")
            print("Press Ctrl+C to stop")
            
            try:
                while self.is_directx_app_running():
                    print(f"\n--- Starting scheduled texture regeneration ---")
                    self.regenerate_all_textures()
                    
                    # Wait for the interval, checking app status periodically
                    elapsed = 0
                    while elapsed < regeneration_interval and self.is_directx_app_running():
                        time.sleep(1)
                        elapsed += 1
                
                print("\nDirectX application has closed. Stopping texture regeneration.")
                
            except KeyboardInterrupt:
                print("\nShutting down...")
        
        print("Texture manager finished")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="DirectX Texture Manager with AI-powered regeneration",
    )
    parser.add_argument(
        "--assets-path",
        type=Path,
        default=Path("assets"),
        help="Path to the assets folder containing textures",
    )
    parser.add_argument(
        "--exe-path", 
        type=Path,
        default=Path("DirectX12Triangle.exe"),
        help="Path to the DirectX executable",
    )
    parser.add_argument(
        "--regeneration-interval",
        type=float,
        default=None,
        help="Interval in seconds for periodic texture regeneration (0 to disable)",
    )
    parser.add_argument(
        "--force-reanalyze",
        action="store_true",
        help="Force re-analysis of textures even if texture_analysis.json exists",
    )
    parser.add_argument(
        "--api-key",
        dest="api_key",
        default=os.getenv("GOOGLE_API_KEY"),
        help="Google Gemini API key (defaults to GOOGLE_API_KEY env var)",
    )

    args = parser.parse_args()

    if not args.api_key:
        parser.error("A Google Gemini API key is required. Set --api-key or the GOOGLE_API_KEY env var.")

    if not args.assets_path.exists():
        parser.error(f"Assets path does not exist: {args.assets_path}")
    
    if not args.exe_path.exists():
        parser.error(f"DirectX executable not found: {args.exe_path}")

    try:
        manager = TextureManager(args.assets_path, args.api_key, args.exe_path, args.force_reanalyze)
        manager.run(args.regeneration_interval)
    except Exception as exc:
        parser.exit(status=1, message=f"Error: {exc}\n")


if __name__ == "__main__":
    main()
