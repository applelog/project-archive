import sys
import os

MAX_BYTES = 1474560  # 1.44MB

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 check_size.py <binary_path>")
        sys.exit(1)
        
    binary_path = sys.argv[1]
    if not os.path.exists(binary_path):
        print(f"[ERROR] Binary {binary_path} does not exist!")
        sys.exit(1)
        
    size = os.path.getsize(binary_path)
    percent = (size / MAX_BYTES) * 100.0
    
    print("==================================================")
    print(f" TARGET BINARY: {binary_path}")
    print(f" SIZE: {size:,} bytes / {MAX_BYTES:,} bytes ({percent:.2f}%)")
    print("==================================================")
    
    if size > MAX_BYTES:
        overflow = size - MAX_BYTES
        print(f"[FAIL] OVER SIZE LIMIT BY {overflow:,} BYTES!")
        sys.exit(1)
    else:
        remaining = MAX_BYTES - size
        print(f"[PASS] WITHIN 1.44MB LIMIT! ({remaining:,} bytes remaining)")
        sys.exit(0)

if __name__ == "__main__":
    main()
