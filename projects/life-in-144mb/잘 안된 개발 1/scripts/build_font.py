import os
import sys
from fontTools import subset

def main():
    input_ttf = "neodgm.ttf"
    output_ttf = "neodgm_subset.ttf"
    output_c_header = "src/font_data.h"

    # All KS X 1001 2350 Hangul syllables + ASCII + symbols + Hangul Jamo
    # ASCII (32..126)
    chars = set(chr(i) for i in range(32, 127))
    # Korean Jamo (0x3131..0x3163)
    for i in range(0x3131, 0x3164):
        chars.add(chr(i))
    # Common Hangul Syllables (0xAC00..0xD7A3) - full 11172 syllables or all used chars
    for i in range(0xAC00, 0xD7A4):
        chars.add(chr(i))
    # Special symbols
    chars.update(["♥", "▶", "◀", "▲", "▼", "★", "·", "…", "“", "”", "‘", "’", "○", "●", "◎", "※", "→", "←"])

    text_unicodes = sorted(list(set(ord(c) for c in chars)))

    print(f"[FONT] Subsetting {input_ttf} with {len(text_unicodes)} glyphs...")
    options = subset.Options()
    options.desubroutinize = True
    options.layout_closure = False
    options.name_IDs = ['*']
    options.name_legacy = False
    options.name_languages = ['*']
    options.drop_tables = ['GSUB', 'GPOS', 'GDEF', 'vhea', 'vmtx', 'meta', 'BASE', 'JSTF', 'DSIG', 'EBDT', 'EBLC', 'EBSC', 'CBDT', 'CBLC', 'sbix', 'COLR', 'CPAL']

    font = subset.load_font(input_ttf, options)
    subsetter = subset.Subsetter(options=options)
    subsetter.populate(unicodes=text_unicodes)
    subsetter.subset(font)
    subset.save_font(font, output_ttf, options)

    size = os.path.getsize(output_ttf)
    print(f"[FONT] Subset TTF size: {size:,} bytes")

    # Read binary and export to C header
    with open(output_ttf, "rb") as f:
        data = f.read()

    with open(output_c_header, "w") as f:
        f.write("// Auto-generated embedded Korean retro pixel font (NeoDunggeunmo subset)\n")
        f.write("#ifndef FONT_DATA_H\n#define FONT_DATA_H\n\n")
        f.write(f"#define KOREAN_FONT_TTF_SIZE {len(data)}\n")
        f.write("static const unsigned char g_korean_font_ttf[] = {\n")
        for i, b in enumerate(data):
            if i % 16 == 0:
                f.write("    ")
            f.write(f"0x{b:02x}, ")
            if i % 16 == 15:
                f.write("\n")
        if len(data) % 16 != 0:
            f.write("\n")
        f.write("};\n\n#endif // FONT_DATA_H\n")

    print(f"[FONT] Generated {output_c_header} successfully!")

if __name__ == "__main__":
    main()
