#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../src/font_korean.h"
#include "../src/font_data.h"

int main(void) {
    printf("=== Running Task 3: Korean Font Engine Tests ===\n");

    // 1. Check glyph structure and constants
    assert(GLYPH_HEIGHT == 12);
    assert(GLYPH_COUNT > 50);

    // 2. Strict Memory Footprint Validation (< 35 KB)
    size_t font_table_size = sizeof(GAME_GLYPHS);
    printf("[TEST] Total glyph count: %d\n", GLYPH_COUNT);
    printf("[TEST] GAME_GLYPHS array memory footprint: %zu bytes (%.2f KB)\n", 
           font_table_size, (double)font_table_size / 1024.0);
    assert(font_table_size < 35 * 1024); // Must be strictly under 35KB

    // 3. Verify GAME_GLYPHS is strictly sorted ascending (required for binary search)
    for (int i = 0; i < GLYPH_COUNT - 1; i++) {
        assert(GAME_GLYPHS[i].codepoint < GAME_GLYPHS[i + 1].codepoint);
    }

    // 4. Verify critical Korean & UI characters exist
    uint32_t required_cps[] = {
        0x0020, // ' '
        0x0021, // '!'
        0x003F, // '?'
        0x005A, // 'Z'
        0x0058, // 'X'
        0x003A, // ':'
        0x002F, // '/'
        0xC5C4, // '엄'
        0xB9C8, // '마'
        0xC5B4, // '어'
        0xB528, // '딨'
        0xAC00, // '가'
        0xC9C0, // '지'
        0xD558, // '하'
        0xCCA0, // '철'
        0xAE30, // '기'
        0xC5B5, // '억'
    };
    int req_count = (int)(sizeof(required_cps) / sizeof(required_cps[0]));

    for (int r = 0; r < req_count; r++) {
        uint32_t target = required_cps[r];
        int found = 0;
        for (int i = 0; i < GLYPH_COUNT; i++) {
            if (GAME_GLYPHS[i].codepoint == target) {
                found = 1;
                if (target != 0x0020) {
                    int non_zero_rows = 0;
                    for (int row = 0; row < GLYPH_HEIGHT; row++) {
                        if (GAME_GLYPHS[i].bitmap[row] != 0) non_zero_rows++;
                    }
                    assert(non_zero_rows > 0);
                }
                break;
            }
        }
        assert(found && "Required character codepoint missing from GAME_GLYPHS!");
    }

    // 5. Test FontKorean_Init()
    FontKorean_Init();

    // 6. Test FontKorean_MeasureTextWidth()
    assert(FontKorean_MeasureTextWidth(NULL, 1) == 0);
    assert(FontKorean_MeasureTextWidth("", 1) == 0);
    assert(FontKorean_MeasureTextWidth("Test", 0) == 0);
    assert(FontKorean_MeasureTextWidth("Test", -1) == 0);

    int w_single = FontKorean_MeasureTextWidth("엄", 1);
    assert(w_single > 0);
    assert(FontKorean_MeasureTextWidth("엄", 2) == w_single * 2);

    int w_str = FontKorean_MeasureTextWidth("엄마, 어딨어?", 1);
    assert(w_str > 30);

    // 7. Verify FontKorean_DrawText with hidden Raylib window
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(100, 100, "FontTest");
    
    BeginDrawing();
    ClearBackground(BLACK);
    FontKorean_DrawText("엄마, 어딨어?", 10, 10, 1, WHITE);
    FontKorean_DrawText("Z / SPACE : 조사 및 대화\nX : 기억 수첩", 10, 30, 1, YELLOW);
    FontKorean_DrawText("12345", 10, 60, 2, RED);
    FontKorean_DrawText(NULL, 0, 0, 1, WHITE);
    FontKorean_DrawText("", 0, 0, 1, WHITE);
    EndDrawing();

    CloseWindow();

    printf("[PASS] All Korean font engine tests passed successfully!\n");
    return 0;
}
