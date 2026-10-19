#!/usr/bin/env python3
import os
import sys

LIMIT_BYTES = 1474560

def check_size(filepath):
    if not os.path.exists(filepath):
        print(f"[ERROR] Target file not found: {filepath}")
        sys.exit(1)
    
    size = os.path.getsize(filepath)
    rem = LIMIT_BYTES - size
    ratio = (size / LIMIT_BYTES) * 100
    
    print("=" * 60)
    print(" 💾 1.44MB GAME SIZE AUDIT (2P GAME ARCADE)")
    print("=" * 60)
    print(f" Target File  : {filepath}")
    print(f" File Size    : {size:,} bytes")
    print(f" Disk Limit   : {LIMIT_BYTES:,} bytes (1.44MB HD Floppy)")
    print(f" Usage Ratio  : {ratio:.2f}%")
    print(f" Free Space   : {rem:,} bytes")
    print("=" * 60)
    
    if size > LIMIT_BYTES:
        print(f"❌ [FAILED] EXCEEDED 1.44MB LIMIT BY {abs(rem):,} BYTES!")
        sys.exit(1)
    else:
        print("✅ [PASSED] PERFECTLY WITHIN 1.44MB LIMIT!")
        sys.exit(0)

if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "bin/game"
    check_size(target)
