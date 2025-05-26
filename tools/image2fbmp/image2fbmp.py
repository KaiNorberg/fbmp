#!/usr/bin/env python3

import sys
import struct
import argparse
from pathlib import Path
from PIL import Image

def image2fbmp(inputPath, outputPath=None):
    inputPath = Path(inputPath)
    
    if not inputPath.exists():
        raise FileNotFoundError(f"image2fbmp: file not found: {inputPath}")
    
    if outputPath is None:
        outputPath = inputPath.with_suffix('.fbmp')
    else:
        outputPath = Path(outputPath)
    
    try:
        with Image.open(inputPath) as img:
            if img.mode != 'RGBA':
                img = img.convert('RGBA')
            
            width, height = img.size
            
            rgbaData = img.tobytes('raw', 'RGBA')
            
            bgraData = bytearray()
            for i in range(0, len(rgbaData), 4):
                r, g, b, a = rgbaData[i:i+4]
                bgraData.extend([b, g, r, a])
            
            with open(outputPath, 'wb') as f:
                f.write(struct.pack('<I', 0x706D6266))
                f.write(struct.pack('<I', width))
                f.write(struct.pack('<I', height))
                f.write(bgraData)
        
            print(f"image2fbmp: converted '{inputPath}' to '{outputPath}'")
            return str(outputPath)
            
    except Exception as e:
        raise RuntimeError(f"image2fbmp: error converting image: {e}")

def main():
    parser = argparse.ArgumentParser(
        description="Convert image files to custom .fbmp format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python image2fbmp.py input.png
  python image2fbmp.py input.jpg output.fbmp
  python image2fbmp.py --verify output.fbmp
        """
    )
    
    parser.add_argument('input', help='Input image file or .fbmp file (when using --verify)')
    parser.add_argument('output', nargs='?', help='Output .fbmp file (optional)')
    
    args = parser.parse_args()
    
    try:
        image2fbmp(args.input, args.output)
            
    except (FileNotFoundError, RuntimeError) as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()