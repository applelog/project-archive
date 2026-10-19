#include "font_korean.h"
#include "font_data.h"
#include <stdlib.h>
#include <string.h>

static Font s_font;
static bool s_loaded = false;

void FontKorean_Init(void) {
    if (s_loaded) return;
    
    // Extract unique codepoints
    int* codepoints = (int*)malloc(sizeof(int) * 15000);
    int cp_count = 0;
    
    for (int i = 32; i <= 126; i++) codepoints[cp_count++] = i;
    for (int i = 0x3131; i <= 0x3163; i++) codepoints[cp_count++] = i;
    for (int i = 0xAC00; i <= 0xD7A3; i++) codepoints[cp_count++] = i;
    codepoints[cp_count++] = 0x2665; // ♥
    codepoints[cp_count++] = 0x25B6; // ▶
    codepoints[cp_count++] = 0x25C0; // ◀
    codepoints[cp_count++] = 0x25B2; // ▲
    codepoints[cp_count++] = 0x25BC; // ▼
    codepoints[cp_count++] = 0x2605; // ★
    codepoints[cp_count++] = 0x300A; // 《
    codepoints[cp_count++] = 0x300B; // 》
    codepoints[cp_count++] = 0x2026; // …
    
    s_font = LoadFontFromMemory(".ttf", g_korean_font_ttf, KOREAN_FONT_TTF_SIZE, 16, codepoints, cp_count);
    SetTextureFilter(s_font.texture, TEXTURE_FILTER_POINT);
    free(codepoints);
    s_loaded = true;
}

Font FontKorean_GetFont(void) {
    if (!s_loaded) FontKorean_Init();
    return s_font;
}

void FontKorean_DrawText(const char* text, int x, int y, int size, Color color) {
    if (!text || text[0] == '\0') return;
    if (!s_loaded) FontKorean_Init();
    DrawTextEx(s_font, text, (Vector2){ (float)x, (float)y }, (float)size, 1.0f, color);
}

int FontKorean_MeasureTextWidth(const char* text, int size) {
    if (!text || text[0] == '\0') return 0;
    if (!s_loaded) FontKorean_Init();
    Vector2 dim = MeasureTextEx(s_font, text, (float)size, 1.0f);
    return (int)dim.x;
}

void FontKorean_Close(void) {
    if (s_loaded) {
        UnloadFont(s_font);
        s_loaded = false;
    }
}
