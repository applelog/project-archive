import os
import glob
from PIL import Image, ImageDraw, ImageFont

GAME_TEXTS = [
    # UI & System & Controls
    "엄마, 어딨어?", "Z / SPACE : 조사 및 대화", "X : 기억 수첩", "이동 : 방향키 / WASD",
    "지하철 2호선", "열차가 진입하고 있습니다.", "기억의 조각을 발견했다.", "[ R : 다시 시작 ]",
    "튜토리얼", "[ 방향키 / WASD 로 움직여 보세요 ]", "[ SPACE / Z 키로 역무원과 대화하세요 ]",
    "[ 벤치 위의 반짝이는 승차권을 조사하세요 ]", "[ 열차 문 안으로 걸어 들어가 탑승하세요 ]",
    "목적지 선택", "1. 옛 골목 재래시장", "2. 초등학교 앞 놀이터", "3. 추억의 옛집 단독주택",
    "열차가 출발합니다. 이번 역은 ", " 역입니다.", "내리실 문은 오른쪽입니다.",
    "다음 역으로 이동 중...", "기억의 파편", "기억의 조각", "지하철역",
    "1. 지하철 환승 승강장 (중앙역)", "2. 옛 골목 재래시장", "3. 초등학교 앞 놀이터", "4. 추억의 옛집 단독주택",
    "지하철 2호선 노선도 (목적지 선택)", "[ W/S : 선택 | SPACE : 탑승 ]", "(현재역)",
    "열차가 이동 중입니다. 이번 역은 [ ", " ] 역입니다.", "지하철 안내방송", "나 (20대)",
    "[ SPACE / Z : 계속 ]",
    
    # Intro Story Monologues
    "엄마... 어디 계신 거예요?",
    "저를 두고 어디로 가신 건가요...",
    "제가 꼭... 엄마를 찾으러 갈게요.",
    "눈앞이 흐릿하다... 여긴 어디지?",
    "유리에 비친 내 모습... 엄마가 사주신 하얀 원피스 그대로야.",
    "엄마가 예쁘다고 사주셨던 그 옷 그대로야.",
    "아직 늦지 않았어. 엄마를 찾아야 해.",
    "어둠 속에서 아련한 멜로디가 들려온다...",
    "...엄마?",
    
    # Dialogue & Clues across 4 Areas
    "역무원", "채소가게 상인", "문방구 할아버지", "이웃집 할머니",
    "아가씨, 혼자서 어딜 가시려고요? 표는 챙기셨나요?",
    "할머니... 아니, 댁이 어디시길래 이 시간에 혼자...",
    "새댁, 오늘 콩나물 싱싱해! 한 봉지 사가!",
    "어르신, 혹시 길을 잃으셨나요? 가족분들 전화번호 아세요?",
    "젊은 처자가 왜 초등학교 앞을 서성이고 있어?",
    "어머님, 날이 저무는데 얼른 댁으로 들어가셔야죠...",
    "새댁네 둘째 아이가 벌써 걸음마를 뗐다면서?",
    "영순이 엄마... 나야, 옆집 순이 엄마. 나 못 알아보겠어?",
    
    # Items & Memory Shards
    "낡은 지하철 승차권", "손등의 주름... 내가 기억하는 내 손이 아니야.",
    "작은 배냇저고리", "아기 냄새... 내가 이 옷을 밤새 바느질해서 입혔었지.",
    "노란 유치원 가방", "\"엄마! 나 오늘 상 받았어!\" 아이의 목소리가 귓가에 맴돈다.",
    "낡은 가족사진", "사진 속 다정한 어머니의 얼굴... 바로 지금의 나였구나.",
    "기억을 되찾을수록 세상이 온기와 색채를 되찾습니다.",
    
    # Multi-Endings
    "《 진실의 거울 》", "거울 속에 비친 주름진 손과 백발의 어머니.",
    "\"아... 내가 우리 아이들의 엄마였구나.\"",
    "기억을 되찾은 어머니의 미소와 함께 가족들이 눈물을 흘리며 달려옵니다.",
    "《 가족의 품으로 》", "어두워진 밤길, 멀리서 숨을 헐떡이며 달려온 딸의 목소리.",
    "\"엄마! 한참 찾았잖아... 집에 가자.\"", "따스한 손에 이끌려 집으로 돌아갑니다.",
    "《 기억의 암전 》", "눈앞이 점점 어두워지며 모든 소리가 아득해집니다.",
    "눈을 떴을 때, 차가운 병실 침대와 창문만이 덩그러니 보입니다."
]

def main():
    # 1. Collect all unique characters
    unique_chars = set(" !?.,'\"~:;()[]<>0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ/%+-=_*▶")
    for text in GAME_TEXTS:
        for char in text:
            unique_chars.add(char)
            
    # Also scan source files
    for filepath in glob.glob("src/*.c") + glob.glob("src/*.h"):
        if "font_data.h" in filepath:
            continue
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                for ch in f.read():
                    if ord(ch) >= 32:
                        unique_chars.add(ch)
        except Exception:
            pass

    sorted_chars = sorted(list(unique_chars), key=lambda c: ord(c))
    print(f"[FONT GEN] Total unique characters to rasterize: {len(sorted_chars)}")
    
    # 2. Find Korean Font on macOS
    font_paths = [
        "/System/Library/Fonts/Supplemental/AppleGothic.ttf",
        "/System/Library/Fonts/AppleSDGothicNeo.ttc",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf"
    ]
    font_path = None
    for p in font_paths:
        if os.path.exists(p):
            font_path = p
            break
            
    if not font_path:
        raise RuntimeError("No system Korean font found!")
        
    print(f"[FONT GEN] Using system font: {font_path}")
    font = ImageFont.truetype(font_path, 11)
    
    # 3. Generate font_data.h with 12x12 pixel-perfect bitmasks
    os.makedirs("src", exist_ok=True)
    with open("src/font_data.h", "w", encoding="utf-8") as f:
        f.write("// Pixel-Perfect Korean 1-bit Bitmap Font Subset for '엄마, 어딨어?'\n")
        f.write("#ifndef FONT_DATA_H\n#define FONT_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define GLYPH_COUNT {len(sorted_chars)}\n")
        f.write("#define GLYPH_HEIGHT 12\n\n")
        
        f.write("typedef struct {\n")
        f.write("    uint32_t codepoint;\n")
        f.write("    uint8_t width;\n")
        f.write("    uint16_t bitmap[12];\n")
        f.write("} Glyph;\n\n")
        
        f.write("static const Glyph GAME_GLYPHS[GLYPH_COUNT] = {\n")
        
        for ch in sorted_chars:
            cp = ord(ch)
            
            if ch == " ":
                f.write(f"    {{ 0x{cp:04X}, 4, {{ 0,0,0,0,0,0,0,0,0,0,0,0 }} }}, // ' '\n")
                continue
                
            img = Image.new("1", (16, 16), color=0)
            draw = ImageDraw.Draw(img)
            # Center and draw
            draw.text((1, -2), ch, font=font, fill=1)
            
            # Find bounds
            min_x, max_x = 15, 0
            has_pixels = False
            for y in range(12):
                for x in range(16):
                    if img.getpixel((x, y)):
                        min_x = min(min_x, x)
                        max_x = max(max_x, x)
                        has_pixels = True
                        
            if not has_pixels:
                width = 6 if cp < 128 else 11
            else:
                width = max(max_x + 1, 5) if cp < 128 else 11
                if width > 12: width = 12
                
            # Pack rows into 16-bit bitmasks
            rows = []
            for y in range(12):
                row_val = 0
                for x in range(12):
                    if img.getpixel((x, y)):
                        row_val |= (1 << (15 - x))
                rows.append(f"0x{row_val:04X}")
                
            rows_str = ", ".join(rows)
            # Comment escape
            comment_ch = ch if ch not in ["\\", "\n", "\r"] else f"\\u{cp:04X}"
            f.write(f"    {{ 0x{cp:04X}, {width}, {{ {rows_str} }} }}, // '{comment_ch}'\n")
            
        f.write("};\n\n#endif // FONT_DATA_H\n")
        
    file_size = os.path.getsize("src/font_data.h")
    print(f"[FONT GEN] Successfully generated src/font_data.h ({len(sorted_chars)} glyphs, {file_size:,} bytes, ~{len(sorted_chars)*28/1024:.2f} KB in binary)")

if __name__ == "__main__":
    main()
