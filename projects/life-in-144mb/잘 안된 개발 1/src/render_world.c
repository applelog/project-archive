#include "render_world.h"
#include "font_data.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Font s_korean_font;
static bool s_font_loaded = false;

void RenderWorld_InitFont(void) {
    if (s_font_loaded) return;

    int* codepoints = (int*)malloc(sizeof(int) * 12000);
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

    s_korean_font = LoadFontFromMemory(".ttf", g_korean_font_ttf, KOREAN_FONT_TTF_SIZE, 16, codepoints, cp_count);
    SetTextureFilter(s_korean_font.texture, TEXTURE_FILTER_POINT);
    free(codepoints);
    s_font_loaded = true;
}

Font RenderWorld_GetKoreanFont(void) {
    RenderWorld_InitFont();
    return s_korean_font;
}

void RenderWorld_DrawPixelText(const char* text, int x, int y, int size, Color color) {
    if (!text || text[0] == '\0') return;
    RenderWorld_InitFont();
    DrawTextEx(s_korean_font, text, (Vector2){ (float)x, (float)y }, (float)size, 1.0f, color);
}

static uint8_t s_maps[MAP_COUNT][MAX_MAP_H][MAX_MAP_W];
static int s_map_widths[MAP_COUNT];
static int s_map_heights[MAP_COUNT];
static bool s_maps_initialized = false;

static void InitMapLayouts(void) {
    if (s_maps_initialized) return;

    for (int m = 0; m < MAP_COUNT; m++) {
        for (int y = 0; y < MAX_MAP_H; y++) {
            for (int x = 0; x < MAX_MAP_W; x++) {
                s_maps[m][y][x] = TILE_VOID;
            }
        }
    }

    // =========================================================================
    // 1. MAP_OVERWORLD: 야외 메인 월드 (56 x 44)
    // =========================================================================
    s_map_widths[MAP_OVERWORLD]  = 56;
    s_map_heights[MAP_OVERWORLD] = 44;
    for (int y = 0; y < 44; y++) {
        for (int x = 0; x < 56; x++) {
            s_maps[MAP_OVERWORLD][y][x] = TILE_GRASS;
        }
    }

    // [우리 집 외관] (Top-Left: x 4..14, y 3..9)
    for (int y = 3; y <= 9; y++) {
        for (int x = 4; x <= 14; x++) {
            if (y == 3 || y == 9 || x == 4 || x == 14) s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_BRICK;
            else s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_BRICK;
        }
    }
    s_maps[MAP_OVERWORLD][9][8] = TILE_DOOR_HOME;
    s_maps[MAP_OVERWORLD][5][6] = TILE_WINDOW;
    s_maps[MAP_OVERWORLD][5][12] = TILE_WINDOW;

    // [남의 집 1 (잠김)] (x 18..26, y 3..9)
    for (int y = 3; y <= 9; y++) {
        for (int x = 18; x <= 26; x++) {
            s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_CONCRETE;
        }
    }
    s_maps[MAP_OVERWORLD][9][22] = TILE_DOOR_LOCKED;

    // [남의 집 2 (잠김)] (x 4..12, y 18..24)
    for (int y = 18; y <= 24; y++) {
        for (int x = 4; x <= 12; x++) {
            s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_BRICK;
        }
    }
    s_maps[MAP_OVERWORLD][24][8] = TILE_DOOR_LOCKED;

    // [그린 문방구 건물 외관] (x 16..26, y 16..22)
    for (int y = 16; y <= 22; y++) {
        for (int x = 16; x <= 26; x++) {
            s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_CONCRETE;
        }
    }
    s_maps[MAP_OVERWORLD][22][20] = TILE_DOOR_SHOP;
    s_maps[MAP_OVERWORLD][23][18] = TILE_ARCADE_MACHINE;
    s_maps[MAP_OVERWORLD][23][23] = TILE_VENDING_MACHINE;
    s_maps[MAP_OVERWORLD][24][16] = TILE_PYEONGSANG;

    // [초등학교 교문 & 본관 외관] (Top-Right: x 34..52, y 2..16)
    for (int y = 2; y <= 16; y++) {
        s_maps[MAP_OVERWORLD][y][34] = TILE_FENCE_IRON;
        s_maps[MAP_OVERWORLD][y][52] = TILE_FENCE_IRON;
    }
    for (int x = 34; x <= 52; x++) s_maps[MAP_OVERWORLD][2][x] = TILE_FENCE_IRON;
    s_maps[MAP_OVERWORLD][16][42] = TILE_CROSSWALK;
    s_maps[MAP_OVERWORLD][15][42] = TILE_PATH_STONE;

    for (int y = 4; y <= 10; y++) {
        for (int x = 36; x <= 50; x++) {
            s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_BRICK;
        }
    }
    s_maps[MAP_OVERWORLD][10][44] = TILE_DOOR_SCHOOL;

    s_maps[MAP_OVERWORLD][14][36] = TILE_SOCCER_GOAL;
    s_maps[MAP_OVERWORLD][14][50] = TILE_SOCCER_GOAL;

    // [포장마차 외관] (x 28, y 28)
    s_maps[MAP_OVERWORLD][28][28] = TILE_DOOR_POCHA;

    // [아파트 단지 외관] (Bottom-Right: x 34..52, y 24..38)
    for (int y = 24; y <= 38; y++) {
        for (int x = 34; x <= 52; x++) {
            s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_CONCRETE;
        }
    }
    s_maps[MAP_OVERWORLD][38][42] = TILE_DOOR_APT;
    s_maps[MAP_OVERWORLD][36][38] = TILE_MAILBOX;

    // [회사 빌딩 외관] (Bottom-Left: x 4..16, y 30..40)
    for (int y = 30; y <= 40; y++) {
        for (int x = 4; x <= 16; x++) {
            s_maps[MAP_OVERWORLD][y][x] = TILE_WALL_CONCRETE;
        }
    }
    s_maps[MAP_OVERWORLD][40][10] = TILE_DOOR_OFFICE;

    // 도로 & 가로수
    for (int y = 10; y < 44; y++) s_maps[MAP_OVERWORLD][y][8] = TILE_PATH_STONE;
    for (int x = 8; x < 54; x++)  s_maps[MAP_OVERWORLD][16][x] = TILE_PATH_STONE;
    for (int x = 8; x < 54; x++)  s_maps[MAP_OVERWORLD][28][x] = TILE_PATH_ASPHALT;
    s_maps[MAP_OVERWORLD][28][20] = TILE_CROSSWALK;

    s_maps[MAP_OVERWORLD][12][7]  = TILE_POLE;
    s_maps[MAP_OVERWORLD][26][7]  = TILE_POLE;
    s_maps[MAP_OVERWORLD][16][30] = TILE_TREE_CHERRY;
    s_maps[MAP_OVERWORLD][24][30] = TILE_TREE_BIG;
    s_maps[MAP_OVERWORLD][30][22] = TILE_STREETLAMP;

    // =========================================================================
    // 2. MAP_MY_HOME: 우리 집 실내 (16 x 14)
    // =========================================================================
    s_map_widths[MAP_MY_HOME]  = 16;
    s_map_heights[MAP_MY_HOME] = 14;
    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 16; x++) {
            if (y == 0 || y == 13 || x == 0 || x == 15) s_maps[MAP_MY_HOME][y][x] = TILE_WALL_INTERIOR;
            else s_maps[MAP_MY_HOME][y][x] = TILE_FLOOR_WOOD;
        }
    }
    s_maps[MAP_MY_HOME][13][8] = TILE_DOORMAT;
    s_maps[MAP_MY_HOME][1][2]  = TILE_TV;
    s_maps[MAP_MY_HOME][1][6]  = TILE_WINDOW;
    s_maps[MAP_MY_HOME][1][10] = TILE_BOOKSHELF;
    s_maps[MAP_MY_HOME][1][13] = TILE_FRIDGE;
    s_maps[MAP_MY_HOME][3][13] = TILE_SINK;
    s_maps[MAP_MY_HOME][5][3]  = TILE_BED;
    s_maps[MAP_MY_HOME][6][10] = TILE_TABLE;

    // =========================================================================
    // 3. MAP_STATIONERY_STORE: 문방구 실내 (14 x 12)
    // =========================================================================
    s_map_widths[MAP_STATIONERY_STORE]  = 14;
    s_map_heights[MAP_STATIONERY_STORE] = 12;
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 14; x++) {
            if (y == 0 || y == 11 || x == 0 || x == 13) s_maps[MAP_STATIONERY_STORE][y][x] = TILE_WALL_INTERIOR;
            else s_maps[MAP_STATIONERY_STORE][y][x] = TILE_FLOOR_TILE;
        }
    }
    s_maps[MAP_STATIONERY_STORE][11][7] = TILE_DOORMAT;
    s_maps[MAP_STATIONERY_STORE][1][2]  = TILE_STATIONERY_SHELF;
    s_maps[MAP_STATIONERY_STORE][1][5]  = TILE_STATIONERY_SHELF;
    s_maps[MAP_STATIONERY_STORE][1][8]  = TILE_STATIONERY_SHELF;
    s_maps[MAP_STATIONERY_STORE][1][11] = TILE_STATIONERY_SHELF;
    s_maps[MAP_STATIONERY_STORE][5][2]  = TILE_ARCADE_MACHINE;
    s_maps[MAP_STATIONERY_STORE][5][11] = TILE_TABLE;

    // =========================================================================
    // 4. MAP_CLASSROOM: 초등학교 교실 실내 (18 x 14)
    // =========================================================================
    s_map_widths[MAP_CLASSROOM]  = 18;
    s_map_heights[MAP_CLASSROOM] = 14;
    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 18; x++) {
            if (y == 0 || y == 13 || x == 0 || x == 17) s_maps[MAP_CLASSROOM][y][x] = TILE_WALL_INTERIOR;
            else s_maps[MAP_CLASSROOM][y][x] = TILE_FLOOR_WOOD;
        }
    }
    s_maps[MAP_CLASSROOM][13][9] = TILE_DOORMAT;
    s_maps[MAP_CLASSROOM][1][6]  = TILE_BLACKBOARD;
    s_maps[MAP_CLASSROOM][1][7]  = TILE_BLACKBOARD;
    s_maps[MAP_CLASSROOM][1][8]  = TILE_BLACKBOARD;
    s_maps[MAP_CLASSROOM][1][9]  = TILE_BLACKBOARD;
    s_maps[MAP_CLASSROOM][1][10] = TILE_BLACKBOARD;
    s_maps[MAP_CLASSROOM][1][11] = TILE_BLACKBOARD;
    s_maps[MAP_CLASSROOM][3][9]  = TILE_DESK;
    s_maps[MAP_CLASSROOM][6][5]  = TILE_DESK; s_maps[MAP_CLASSROOM][6][8]  = TILE_DESK; s_maps[MAP_CLASSROOM][6][12] = TILE_DESK;
    s_maps[MAP_CLASSROOM][9][5]  = TILE_DESK; s_maps[MAP_CLASSROOM][9][8]  = TILE_DESK; s_maps[MAP_CLASSROOM][9][12] = TILE_DESK;
    s_maps[MAP_CLASSROOM][1][2]  = TILE_BOOKSHELF;
    s_maps[MAP_CLASSROOM][1][15] = TILE_WINDOW;

    // =========================================================================
    // 5. MAP_POJANGMACHA_INTERIOR: 포장마차 실내 (12 x 10)
    // =========================================================================
    s_map_widths[MAP_POJANGMACHA_INTERIOR]  = 12;
    s_map_heights[MAP_POJANGMACHA_INTERIOR] = 10;
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 12; x++) {
            if (y == 0 || y == 9 || x == 0 || x == 11) s_maps[MAP_POJANGMACHA_INTERIOR][y][x] = TILE_WALL_WOOD;
            else s_maps[MAP_POJANGMACHA_INTERIOR][y][x] = TILE_FLOOR_CARPET;
        }
    }
    s_maps[MAP_POJANGMACHA_INTERIOR][9][6] = TILE_DOORMAT;
    s_maps[MAP_POJANGMACHA_INTERIOR][1][3] = TILE_POJANGMACHA_POT;
    s_maps[MAP_POJANGMACHA_INTERIOR][1][4] = TILE_POJANGMACHA_POT;
    s_maps[MAP_POJANGMACHA_INTERIOR][4][6] = TILE_TABLE;

    // =========================================================================
    // 6. MAP_APARTMENT_STAIRS: 아파트 계단실 (12 x 26)
    // =========================================================================
    s_map_widths[MAP_APARTMENT_STAIRS]  = 12;
    s_map_heights[MAP_APARTMENT_STAIRS] = 26;
    for (int y = 0; y < 26; y++) {
        for (int x = 0; x < 12; x++) {
            if (x == 0 || x == 11 || y == 0 || y == 25) s_maps[MAP_APARTMENT_STAIRS][y][x] = TILE_WALL_CONCRETE;
            else s_maps[MAP_APARTMENT_STAIRS][y][x] = TILE_FLOOR_TILE;
        }
    }
    s_maps[MAP_APARTMENT_STAIRS][25][6] = TILE_DOORMAT;
    s_maps[MAP_APARTMENT_STAIRS][23][2] = TILE_MAILBOX;
    s_maps[MAP_APARTMENT_STAIRS][20][6] = TILE_STAIRS_UP;
    s_maps[MAP_APARTMENT_STAIRS][19][6] = TILE_STAIRS_UP;
    s_maps[MAP_APARTMENT_STAIRS][18][6] = TILE_STAIRS_UP;
    s_maps[MAP_APARTMENT_STAIRS][13][6] = TILE_STAIRS_UP;
    s_maps[MAP_APARTMENT_STAIRS][12][6] = TILE_STAIRS_UP;
    s_maps[MAP_APARTMENT_STAIRS][11][6] = TILE_STAIRS_UP;
    s_maps[MAP_APARTMENT_STAIRS][3][6]  = TILE_DOORBELL;

    // =========================================================================
    // 7. MAP_APARTMENT_302: 우리 집 아파트 302호 실내 (18 x 14)
    // =========================================================================
    s_map_widths[MAP_APARTMENT_302]  = 18;
    s_map_heights[MAP_APARTMENT_302] = 14;
    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 18; x++) {
            if (y == 0 || y == 13 || x == 0 || x == 17) s_maps[MAP_APARTMENT_302][y][x] = TILE_WALL_INTERIOR;
            else s_maps[MAP_APARTMENT_302][y][x] = TILE_FLOOR_WOOD;
        }
    }
    s_maps[MAP_APARTMENT_302][13][9] = TILE_DOORMAT;
    s_maps[MAP_APARTMENT_302][1][3]  = TILE_TV;
    s_maps[MAP_APARTMENT_302][1][8]  = TILE_WINDOW;
    s_maps[MAP_APARTMENT_302][1][14] = TILE_FRIDGE;
    s_maps[MAP_APARTMENT_302][4][14] = TILE_SINK;
    s_maps[MAP_APARTMENT_302][5][4]  = TILE_BED;
    s_maps[MAP_APARTMENT_302][6][11] = TILE_TABLE;

    // =========================================================================
    // 8. MAP_OFFICE_INTERIOR: 회사 사무실 실내 (18 x 14)
    // =========================================================================
    s_map_widths[MAP_OFFICE_INTERIOR]  = 18;
    s_map_heights[MAP_OFFICE_INTERIOR] = 14;
    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 18; x++) {
            if (y == 0 || y == 13 || x == 0 || x == 17) s_maps[MAP_OFFICE_INTERIOR][y][x] = TILE_WALL_CONCRETE;
            else s_maps[MAP_OFFICE_INTERIOR][y][x] = TILE_FLOOR_TILE;
        }
    }
    s_maps[MAP_OFFICE_INTERIOR][13][9] = TILE_DOORMAT;
    s_maps[MAP_OFFICE_INTERIOR][1][3]  = TILE_WINDOW;
    s_maps[MAP_OFFICE_INTERIOR][1][9]  = TILE_WINDOW;
    s_maps[MAP_OFFICE_INTERIOR][1][15] = TILE_VENDING_MACHINE;
    s_maps[MAP_OFFICE_INTERIOR][5][5]  = TILE_DESK;
    s_maps[MAP_OFFICE_INTERIOR][5][12] = TILE_DESK;
    s_maps[MAP_OFFICE_INTERIOR][8][9]  = TILE_TABLE;

    // =========================================================================
    // 9. MAP_SENIOR_HALL: 경로당 실내 (16 x 12)
    // =========================================================================
    s_map_widths[MAP_SENIOR_HALL]  = 16;
    s_map_heights[MAP_SENIOR_HALL] = 12;
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 16; x++) {
            if (y == 0 || y == 11 || x == 0 || x == 15) s_maps[MAP_SENIOR_HALL][y][x] = TILE_WALL_WOOD;
            else s_maps[MAP_SENIOR_HALL][y][x] = TILE_FLOOR_WOOD;
        }
    }
    s_maps[MAP_SENIOR_HALL][11][8] = TILE_DOORMAT;
    s_maps[MAP_SENIOR_HALL][1][3]  = TILE_TV;
    s_maps[MAP_SENIOR_HALL][1][8]  = TILE_WINDOW;
    s_maps[MAP_SENIOR_HALL][5][8]  = TILE_PYEONGSANG;

    s_maps_initialized = true;
}

void RenderWorld_Init(void) {
    InitMapLayouts();
    RenderWorld_InitFont();
}

void RenderWorld_GetMapDimensions(MapId map_id, int* out_w, int* out_h) {
    InitMapLayouts();
    if (map_id < 0 || map_id >= MAP_COUNT) map_id = MAP_OVERWORLD;
    if (out_w) *out_w = s_map_widths[map_id];
    if (out_h) *out_h = s_map_heights[map_id];
}

TileType RenderWorld_GetTile(MapId map_id, int x, int y) {
    InitMapLayouts();
    if (map_id < 0 || map_id >= MAP_COUNT) return TILE_VOID;
    if (x < 0 || x >= s_map_widths[map_id] || y < 0 || y >= s_map_heights[map_id]) return TILE_VOID;
    return (TileType)s_maps[map_id][y][x];
}

void RenderWorld_DrawMap(MapId map_id, const MasterPalette* pal, float saturation, LifeAct act, Camera2D camera, float sensor_light) {
    (void)act;
    InitMapLayouts();
    if (map_id < 0 || map_id >= MAP_COUNT) map_id = MAP_OVERWORLD;

    int w = s_map_widths[map_id];
    int h = s_map_heights[map_id];

    int start_x = (int)((camera.target.x - camera.offset.x) / TILE_SIZE) - 1;
    int end_x   = (int)((camera.target.x + camera.offset.x) / TILE_SIZE) + 2;
    int start_y = (int)((camera.target.y - camera.offset.y) / TILE_SIZE) - 1;
    int end_y   = (int)((camera.target.y + camera.offset.y) / TILE_SIZE) + 2;

    if (start_x < 0) start_x = 0;
    if (end_x >= w) end_x = w - 1;
    if (start_y < 0) start_y = 0;
    if (end_y >= h) end_y = h - 1;

    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            TileType t = (TileType)s_maps[map_id][y][x];
            int px = x * TILE_SIZE;
            int py = y * TILE_SIZE;

            Color base_color = pal->ground;
            switch (t) {
                case TILE_VOID:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, (Color){ 10, 10, 14, 255 });
                    break;
                case TILE_GRASS:
                    base_color = pal->ground;
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(base_color, saturation));
                    if ((x + y) % 3 == 0) {
                        DrawRectangle(px + 4, py + 6, 2, 4, Palette_ApplySaturation(pal->foliage_dark, saturation));
                        DrawRectangle(px + 14, py + 12, 2, 4, Palette_ApplySaturation(pal->foliage_light, saturation));
                    }
                    break;
                case TILE_TALL_GRASS:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 2, py + 4, 6, 16, Palette_ApplySaturation(pal->foliage_dark, saturation));
                    DrawRectangle(px + 10, py + 2, 6, 18, Palette_ApplySaturation(pal->foliage_dark, saturation));
                    break;
                case TILE_PATH_STONE:
                    base_color = pal->path;
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(base_color, saturation));
                    DrawRectangleLines(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->shadow, saturation * 0.35f));
                    break;
                case TILE_PATH_DIRT:
                    base_color = (Color){ 200, 165, 120, 255 };
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(base_color, saturation));
                    break;
                case TILE_PATH_ASPHALT:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 60, 62, 68, 255 }, saturation));
                    if ((x % 4 == 0) && (y % 2 == 0)) {
                        DrawRectangle(px + 8, py + 10, 8, 3, Palette_ApplySaturation((Color){ 245, 220, 80, 255 }, saturation));
                    }
                    break;
                case TILE_CROSSWALK:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 60, 62, 68, 255 }, saturation));
                    DrawRectangle(px + 2, py + 2, TILE_SIZE - 4, 5, Palette_ApplySaturation((Color){ 250, 250, 250, 255 }, saturation));
                    DrawRectangle(px + 2, py + 10, TILE_SIZE - 4, 5, Palette_ApplySaturation((Color){ 250, 250, 250, 255 }, saturation));
                    DrawRectangle(px + 2, py + 18, TILE_SIZE - 4, 5, Palette_ApplySaturation((Color){ 250, 250, 250, 255 }, saturation));
                    break;
                case TILE_WATER:
                    base_color = pal->sky;
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(base_color, saturation));
                    if ((x + (int)(GetTime() * 3.0f)) % 4 == 0) {
                        DrawRectangle(px + 4, py + 8, 12, 2, Palette_ApplySaturation(pal->light, saturation));
                    }
                    break;
                case TILE_WATER_SHORE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px, py + 14, TILE_SIZE, 10, Palette_ApplySaturation(pal->sky, saturation));
                    break;
                case TILE_WALL_BRICK:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 178, 64, 48, 255 }, saturation));
                    DrawRectangle(px, py, TILE_SIZE, 4, Palette_ApplySaturation((Color){ 120, 40, 30, 255 }, saturation));
                    DrawLine(px, py + 8, px + TILE_SIZE, py + 8, Palette_ApplySaturation((Color){ 220, 200, 190, 200 }, saturation));
                    DrawLine(px, py + 16, px + TILE_SIZE, py + 16, Palette_ApplySaturation((Color){ 220, 200, 190, 200 }, saturation));
                    break;
                case TILE_WALL_CONCRETE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 160, 165, 172, 255 }, saturation));
                    DrawRectangleLines(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 110, 115, 120, 255 }, saturation));
                    break;
                case TILE_WALL_WOOD:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 140, 95, 60, 255 }, saturation));
                    DrawLine(px, py + 12, px + TILE_SIZE, py + 12, Palette_ApplySaturation((Color){ 90, 55, 30, 255 }, saturation));
                    break;
                case TILE_WALL_INTERIOR:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 230, 225, 215, 255 }, saturation));
                    DrawRectangle(px, py + TILE_SIZE - 4, TILE_SIZE, 4, Palette_ApplySaturation((Color){ 180, 130, 85, 255 }, saturation));
                    break;
                case TILE_FLOOR_WOOD:
                    base_color = pal->path;
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(base_color, saturation));
                    DrawLine(px, py + 12, px + TILE_SIZE, py + 12, Palette_ApplySaturation(pal->shadow, saturation * 0.3f));
                    break;
                case TILE_FLOOR_TILE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 220, 225, 230, 255 }, saturation));
                    DrawRectangleLines(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 180, 185, 190, 255 }, saturation));
                    break;
                case TILE_FLOOR_CARPET:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 200, 110, 80, 255 }, saturation));
                    break;
                case TILE_WINDOW:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 160, 165, 172, 255 }, saturation));
                    DrawRectangle(px + 3, py + 3, TILE_SIZE - 6, TILE_SIZE - 6, Palette_ApplySaturation((Color){ 130, 205, 240, 255 }, saturation));
                    DrawLine(px + 4, py + 18, px + 18, py + 4, Palette_ApplySaturation((Color){ 255, 255, 255, 220 }, saturation));
                    break;
                case TILE_DOOR_HOME:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 140, 70, 40, 255 }, saturation));
                    DrawRectangle(px + 3, py + 3, TILE_SIZE - 6, TILE_SIZE - 3, Palette_ApplySaturation((Color){ 220, 160, 100, 255 }, saturation));
                    DrawCircle(px + TILE_SIZE - 7, py + TILE_SIZE / 2, 2, Palette_ApplySaturation(pal->light, saturation));
                    RenderWorld_DrawPixelText("우리집", px + 1, py + 18, 6, (Color){ 255, 255, 255, 255 });
                    break;
                case TILE_DOOR_LOCKED:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 90, 95, 100, 255 }, saturation));
                    DrawRectangle(px + 4, py + 4, TILE_SIZE - 8, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 60, 65, 70, 255 }, saturation));
                    DrawRectangle(px + 10, py + 8, 4, 6, Palette_ApplySaturation((Color){ 220, 50, 50, 255 }, saturation));
                    RenderWorld_DrawPixelText("이웃집", px + 1, py + 18, 6, (Color){ 200, 200, 200, 255 });
                    break;
                case TILE_DOOR_SHOP:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 45, 140, 75, 255 }, saturation));
                    DrawRectangle(px + 3, py + 3, TILE_SIZE - 6, TILE_SIZE - 3, Palette_ApplySaturation((Color){ 130, 220, 160, 255 }, saturation));
                    RenderWorld_DrawPixelText("문방구", px + 1, py + 18, 6, (Color){ 255, 255, 255, 255 });
                    break;
                case TILE_DOOR_SCHOOL:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 160, 100, 50, 255 }, saturation));
                    DrawRectangle(px + 3, py + 3, TILE_SIZE - 6, TILE_SIZE - 3, Palette_ApplySaturation((Color){ 220, 180, 130, 255 }, saturation));
                    RenderWorld_DrawPixelText("1-2반", px + 3, py + 18, 6, (Color){ 255, 255, 255, 255 });
                    break;
                case TILE_DOOR_POCHA:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 240, 107, 50, 255 }, saturation));
                    DrawRectangleLines(px + 1, py + 1, TILE_SIZE - 2, TILE_SIZE - 2, Palette_ApplySaturation((Color){ 180, 50, 20, 255 }, saturation));
                    RenderWorld_DrawPixelText("포차", px + 3, py + 6, 8, (Color){ 255, 255, 255, 255 });
                    break;
                case TILE_DOOR_APT:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 80, 90, 105, 255 }, saturation));
                    DrawRectangle(px + 3, py + 3, TILE_SIZE - 6, TILE_SIZE - 3, Palette_ApplySaturation((Color){ 140, 180, 220, 255 }, saturation));
                    RenderWorld_DrawPixelText("101동", px + 1, py + 18, 6, (Color){ 255, 255, 255, 255 });
                    break;
                case TILE_DOOR_OFFICE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 50, 60, 75, 255 }, saturation));
                    DrawRectangle(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 2, Palette_ApplySaturation((Color){ 160, 200, 240, 255 }, saturation));
                    RenderWorld_DrawPixelText("오피스", px + 1, py + 18, 6, (Color){ 255, 255, 255, 255 });
                    break;
                case TILE_DOORMAT:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 2, py + 8, TILE_SIZE - 4, 12, Palette_ApplySaturation((Color){ 200, 60, 50, 255 }, saturation));
                    RenderWorld_DrawPixelText("▼", px + 8, py + 10, 8, (Color){ 255, 255, 255, 255 });
                    break;
                case TILE_TREE_BIG:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 8, py + 12, 8, 12, Palette_ApplySaturation((Color){ 110, 65, 35, 255 }, saturation));
                    DrawCircle(px + 12, py + 8, 12, Palette_ApplySaturation(pal->foliage_dark, saturation));
                    DrawCircle(px + 12, py + 5, 9, Palette_ApplySaturation(pal->foliage_light, saturation));
                    break;
                case TILE_TREE_CHERRY:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 8, py + 12, 8, 12, Palette_ApplySaturation((Color){ 110, 65, 35, 255 }, saturation));
                    DrawCircle(px + 12, py + 8, 12, Palette_ApplySaturation((Color){ 245, 160, 185, 255 }, saturation));
                    break;
                case TILE_TREE_PINE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 9, py + 14, 6, 10, Palette_ApplySaturation((Color){ 90, 50, 30, 255 }, saturation));
                    DrawTriangle((Vector2){ px + 2, py + 16 }, (Vector2){ px + 22, py + 16 }, (Vector2){ px + 12, py + 8 }, Palette_ApplySaturation((Color){ 35, 90, 50, 255 }, saturation));
                    break;
                case TILE_FLOWER_GARDEN:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawCircle(px + 6, py + 6, 3, Palette_ApplySaturation((Color){ 255, 100, 120, 255 }, saturation));
                    DrawCircle(px + 18, py + 7, 3, Palette_ApplySaturation((Color){ 255, 220, 80, 255 }, saturation));
                    break;
                case TILE_FENCE_WOOD:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 1, py + 6, TILE_SIZE - 2, 3, Palette_ApplySaturation((Color){ 180, 130, 85, 255 }, saturation));
                    DrawRectangle(px + 1, py + 14, TILE_SIZE - 2, 3, Palette_ApplySaturation((Color){ 180, 130, 85, 255 }, saturation));
                    break;
                case TILE_FENCE_IRON:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 1, py + 5, TILE_SIZE - 2, 2, Palette_ApplySaturation((Color){ 50, 60, 70, 255 }, saturation));
                    for (int fx = 3; fx < TILE_SIZE; fx += 5) {
                        DrawRectangle(px + fx, py + 1, 2, 20, Palette_ApplySaturation((Color){ 40, 50, 60, 255 }, saturation));
                    }
                    break;
                case TILE_BED:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, Palette_ApplySaturation(pal->accent, saturation));
                    DrawRectangle(px + 4, py + 4, TILE_SIZE - 8, 6, Palette_ApplySaturation(pal->light, saturation));
                    break;
                case TILE_TABLE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 3, py + 4, TILE_SIZE - 6, 8, Palette_ApplySaturation((Color){ 160, 110, 65, 255 }, saturation));
                    break;
                case TILE_TV:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 30, 32, 36, 255 }, saturation));
                    DrawRectangle(px + 4, py + 4, TILE_SIZE - 8, TILE_SIZE - 10, Palette_ApplySaturation((Color){ 84, 201, 206, 255 }, saturation));
                    break;
                case TILE_FRIDGE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 3, py + 2, TILE_SIZE - 6, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 240, 242, 245, 255 }, saturation));
                    DrawLine(px + 3, py + 10, px + TILE_SIZE - 3, py + 10, Palette_ApplySaturation((Color){ 180, 185, 190, 255 }, saturation));
                    break;
                case TILE_BOOKSHELF:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 140, 90, 50, 255 }, saturation));
                    DrawRectangle(px + 4, py + 5, 4, 6, Palette_ApplySaturation((Color){ 220, 60, 50, 255 }, saturation));
                    DrawRectangle(px + 9, py + 5, 4, 6, Palette_ApplySaturation((Color){ 60, 140, 220, 255 }, saturation));
                    break;
                case TILE_SINK:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 200, 205, 210, 255 }, saturation));
                    break;
                case TILE_BENCH:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 2, py + 8, TILE_SIZE - 4, 8, Palette_ApplySaturation(pal->structure_roof, saturation));
                    break;
                case TILE_PYEONGSANG:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 1, py + 2, TILE_SIZE - 2, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 205, 133, 63, 255 }, saturation));
                    DrawRectangleLines(px + 1, py + 2, TILE_SIZE - 2, TILE_SIZE - 4, Palette_ApplySaturation(pal->shadow, saturation));
                    break;
                case TILE_STATIONERY_SHELF:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 230, 170, 80, 255 }, saturation));
                    break;
                case TILE_ARCADE_MACHINE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 3, py + 2, TILE_SIZE - 6, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 45, 50, 60, 255 }, saturation));
                    DrawRectangle(px + 5, py + 4, TILE_SIZE - 10, 8, Palette_ApplySaturation((Color){ 84, 201, 206, 255 }, saturation));
                    break;
                case TILE_VENDING_MACHINE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 3, py + 2, TILE_SIZE - 6, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 220, 40, 40, 255 }, saturation));
                    break;
                case TILE_POJANGMACHA_POT:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 180, 185, 190, 255 }, saturation));
                    DrawRectangle(px + 3, py + 3, TILE_SIZE - 6, TILE_SIZE - 6, Palette_ApplySaturation((Color){ 220, 70, 40, 255 }, saturation));
                    break;
                case TILE_GAZEBO:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawTriangle((Vector2){ px, py + 8 }, (Vector2){ px + TILE_SIZE, py + 8 }, (Vector2){ px + TILE_SIZE / 2, py }, Palette_ApplySaturation((Color){ 60, 70, 85, 255 }, saturation));
                    break;
                case TILE_POLE:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawRectangle(px + 10, py + 2, 4, TILE_SIZE - 2, Palette_ApplySaturation((Color){ 130, 135, 140, 255 }, saturation));
                    break;
                case TILE_STREETLAMP:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->ground, saturation));
                    DrawCircle(px + 12, py + 4, 5, Palette_ApplySaturation((Color){ 255, 235, 120, 255 }, saturation));
                    break;
                case TILE_BLACKBOARD:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 34, 75, 48, 255 }, saturation));
                    DrawRectangleLines(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 139, 90, 43, 255 }, saturation));
                    break;
                case TILE_DESK:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation(pal->path, saturation));
                    DrawRectangle(px + 3, py + 4, TILE_SIZE - 6, 8, Palette_ApplySaturation((Color){ 195, 142, 85, 255 }, saturation));
                    break;
                case TILE_SOCCER_GOAL:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 200, 165, 120, 255 }, saturation));
                    DrawRectangleLines(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, Palette_ApplySaturation((Color){ 255, 255, 255, 255 }, saturation));
                    break;
                case TILE_STAIRS_UP:
                case TILE_STAIRS_DOWN:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 140, 145, 150, 255 }, saturation));
                    DrawRectangle(px, py + 4, TILE_SIZE, 4, Palette_ApplySaturation((Color){ 110, 115, 120, 255 }, saturation));
                    DrawRectangle(px, py + 12, TILE_SIZE, 4, Palette_ApplySaturation((Color){ 90, 95, 100, 255 }, saturation));
                    DrawRectangle(px, py + 20, TILE_SIZE, 4, Palette_ApplySaturation((Color){ 70, 75, 80, 255 }, saturation));
                    break;
                case TILE_MAILBOX:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 160, 165, 172, 255 }, saturation));
                    DrawRectangle(px + 2, py + 4, TILE_SIZE - 4, TILE_SIZE - 8, Palette_ApplySaturation((Color){ 80, 85, 95, 255 }, saturation));
                    break;
                case TILE_DOORBELL:
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, Palette_ApplySaturation((Color){ 55, 60, 70, 255 }, saturation));
                    DrawRectangle(px + 6, py + 6, 12, 12, Palette_ApplySaturation((Color){ 25, 25, 30, 255 }, saturation));
                    DrawCircle(px + 12, py + 10, 2, Palette_ApplySaturation((Color){ 84, 201, 206, 255 }, saturation));
                    RenderWorld_DrawPixelText("302호", px + 1, py + 18, 6, (Color){ 255, 255, 255, 255 });
                    break;
                default:
                    break;
            }
        }
    }

    if (map_id == MAP_APARTMENT_STAIRS && sensor_light > 0.05f) {
        int lamp_x = 6 * TILE_SIZE + 12;
        int lamp_y = 6 * TILE_SIZE + 12;
        DrawCircleGradient((Vector2){ (float)lamp_x, (float)lamp_y }, 90.0f * sensor_light, (Color){ 255, 235, 140, (unsigned char)(160 * sensor_light) }, (Color){ 255, 235, 140, 0 });
    }
}

// =========================================================================
// Top-Down Expressive Human Sprite Renderer
// =========================================================================
static void DrawHumanCharacter(float px, float py, Direction dir, float age, float sat, 
                               Color hair_color, Color shirt_color, Color pants_color, 
                               int hair_style, bool has_glasses, bool has_cane, bool is_moving, float anim_time) {
    float scale = 1.0f;
    if (age < 12.0f) scale = 0.75f + (age / 12.0f) * 0.25f;
    else if (age > 60.0f) scale = 1.0f - ((age - 60.0f) / 40.0f) * 0.12f;

    // Shadow
    DrawEllipse((int)px, (int)(py + 8 * scale), (int)(7 * scale), (int)(3 * scale), Palette_ApplySaturation((Color){ 0, 0, 0, 90 }, sat));

    int leg_left = 0, leg_right = 0, arm_swing = 0;
    if (is_moving) {
        int step = ((int)(anim_time * 8.0f)) % 4;
        if (step == 1) { leg_left = 2; arm_swing = -2; }
        else if (step == 3) { leg_right = 2; arm_swing = 2; }
    }

    // Legs & Shoes
    Color pants = Palette_ApplySaturation(pants_color, sat);
    Color shoes = Palette_ApplySaturation((Color){ 40, 40, 45, 255 }, sat);
    DrawRectangle((int)(px - 4 * scale), (int)(py + 2 * scale + leg_left), (int)(3 * scale), (int)(6 * scale), pants);
    DrawRectangle((int)(px + 1 * scale), (int)(py + 2 * scale + leg_right), (int)(3 * scale), (int)(6 * scale), pants);
    DrawRectangle((int)(px - 5 * scale), (int)(py + 7 * scale + leg_left), (int)(4 * scale), (int)(2 * scale), shoes);
    DrawRectangle((int)(px + 1 * scale), (int)(py + 7 * scale + leg_right), (int)(4 * scale), (int)(2 * scale), shoes);

    // Torso / Clothes
    Color shirt = Palette_ApplySaturation(shirt_color, sat);
    DrawRectangle((int)(px - 6 * scale), (int)(py - 5 * scale), (int)(12 * scale), (int)(8 * scale), shirt);
    DrawRectangle((int)(px - 2 * scale), (int)(py - 5 * scale), (int)(4 * scale), (int)(2 * scale), Palette_ApplySaturation((Color){ 245, 245, 245, 255 }, sat)); // Collar

    // Hands / Arms
    Color skin = Palette_ApplySaturation((Color){ 255, 218, 185, 255 }, sat);
    DrawRectangle((int)(px - 7 * scale), (int)(py - 4 * scale + arm_swing), (int)(2 * scale), (int)(6 * scale), skin);
    DrawRectangle((int)(px + 5 * scale), (int)(py - 4 * scale - arm_swing), (int)(2 * scale), (int)(6 * scale), skin);

    // Head / Face
    DrawCircle((int)px, (int)(py - 9 * scale), (int)(6 * scale), skin);

    // Hairstyle
    Color hair = Palette_ApplySaturation(hair_color, sat);
    if (hair_style == 0) { // Short neat hair
        DrawCircle((int)px, (int)(py - 12 * scale), (int)(5 * scale), hair);
        DrawRectangle((int)(px - 5 * scale), (int)(py - 13 * scale), (int)(10 * scale), (int)(3 * scale), hair);
    } else if (hair_style == 1) { // Perm / Curly hair
        DrawCircle((int)px, (int)(py - 13 * scale), (int)(6 * scale), hair);
        DrawCircle((int)(px - 5 * scale), (int)(py - 10 * scale), (int)(3 * scale), hair);
        DrawCircle((int)(px + 5 * scale), (int)(py - 10 * scale), (int)(3 * scale), hair);
    } else if (hair_style == 2) { // Cap / Hat
        DrawRectangle((int)(px - 6 * scale), (int)(py - 14 * scale), (int)(12 * scale), (int)(4 * scale), hair);
        DrawRectangle((int)(px - 4 * scale), (int)(py - 16 * scale), (int)(8 * scale), (int)(3 * scale), hair);
    } else { // Long hair / Ribbon
        DrawCircle((int)px, (int)(py - 13 * scale), (int)(6 * scale), hair);
        DrawRectangle((int)(px - 6 * scale), (int)(py - 11 * scale), (int)(3 * scale), (int)(7 * scale), hair);
        DrawRectangle((int)(px + 3 * scale), (int)(py - 11 * scale), (int)(3 * scale), (int)(7 * scale), hair);
    }

    // Eyes & Face Details
    Color eye = Palette_ApplySaturation((Color){ 20, 20, 20, 255 }, sat);
    if (dir == DIR_DOWN) {
        DrawRectangle((int)(px - 3 * scale), (int)(py - 10 * scale), 2, 2, eye);
        DrawRectangle((int)(px + 1 * scale), (int)(py - 10 * scale), 2, 2, eye);
        // Soft blush
        DrawCircle((int)(px - 3 * scale), (int)(py - 7 * scale), 1, Palette_ApplySaturation((Color){ 255, 140, 150, 180 }, sat));
        DrawCircle((int)(px + 3 * scale), (int)(py - 7 * scale), 1, Palette_ApplySaturation((Color){ 255, 140, 150, 180 }, sat));
        if (has_glasses) {
            DrawRectangleLines((int)(px - 4 * scale), (int)(py - 11 * scale), 4, 3, eye);
            DrawRectangleLines((int)(px + 0 * scale), (int)(py - 11 * scale), 4, 3, eye);
            DrawLine((int)px, (int)(py - 10 * scale), (int)px, (int)(py - 10 * scale), eye);
        }
    } else if (dir == DIR_UP) {
        DrawCircle((int)px, (int)(py - 10 * scale), (int)(5 * scale), hair);
    } else if (dir == DIR_LEFT) {
        DrawRectangle((int)(px - 4 * scale), (int)(py - 10 * scale), 2, 2, eye);
    } else if (dir == DIR_RIGHT) {
        DrawRectangle((int)(px + 2 * scale), (int)(py - 10 * scale), 2, 2, eye);
    }

    if (has_cane) {
        Color cane = Palette_ApplySaturation((Color){ 139, 69, 19, 255 }, sat);
        DrawRectangle((int)(px + 6 * scale), (int)(py - 2 * scale), 2, (int)(10 * scale), cane);
        DrawRectangle((int)(px + 5 * scale), (int)(py - 3 * scale), 3, 2, cane);
    }
}

void RenderWorld_DrawPlayerTopDown(const PlayerState* player, const MasterPalette* pal) {
    Color hair_color = (Color){ 30, 25, 20, 255 };
    if (player->age > 45.0f && player->age <= 65.0f) {
        float t = (player->age - 45.0f) / 20.0f;
        hair_color = Palette_Lerp((Color){ 60, 45, 30, 255 }, (Color){ 160, 160, 160, 255 }, t);
    } else if (player->age > 65.0f) {
        float t = (player->age - 65.0f) / 15.0f;
        hair_color = Palette_Lerp((Color){ 160, 160, 160, 255 }, (Color){ 240, 240, 245, 255 }, t);
    }

    Color shirt = pal->accent;
    if (player->age >= 25.0f && player->age < 60.0f) shirt = pal->structure_roof;

    DrawHumanCharacter(
        player->x, player->y, player->dir, player->age, player->color_saturation,
        hair_color, shirt, pal->shadow,
        (player->age > 65.0f) ? 1 : 0, false, (player->age > 65.0f),
        player->is_moving, player->anim_timer
    );
}

void RenderWorld_DrawNPC(const NPC* npc, const MasterPalette* pal, float saturation, float anim_time) {
    Color hair = (Color){ 45, 35, 30, 255 };
    Color shirt = pal->accent;
    Color pants = pal->shadow;
    int hair_style = 0;
    bool glasses = false;

    if (npc->sprite_id == 0) { // 어머니 (분홍 한복/앞치마 & 뽀글이 파마)
        shirt = (Color){ 245, 140, 160, 255 };
        pants = (Color){ 180, 80, 100, 255 };
        hair = (Color){ 50, 40, 40, 255 };
        hair_style = 1;
    } else if (npc->sprite_id == 1) { // 아버지 (양복 셔츠 & 넥타이)
        shirt = (Color){ 70, 90, 130, 255 };
        pants = (Color){ 35, 40, 50, 255 };
        hair = (Color){ 40, 35, 30, 255 };
        hair_style = 0;
    } else if (npc->sprite_id == 2) { // 절친 (푸른 야구점퍼 & 캡모자)
        shirt = (Color){ 50, 140, 230, 255 };
        pants = (Color){ 40, 45, 60, 255 };
        hair = (Color){ 30, 25, 20, 255 };
        hair_style = 2;
    } else if (npc->sprite_id == 3) { // 수아 (첫사랑 - 화사한 붉은 카디건 & 긴머리 리본)
        shirt = (Color){ 255, 110, 140, 255 };
        pants = (Color){ 220, 180, 190, 255 };
        hair = (Color){ 90, 45, 25, 255 };
        hair_style = 3;
    } else if (npc->sprite_id == 4) { // 지은 (후배 - 노란 니트 & 단발머리)
        shirt = (Color){ 250, 200, 80, 255 };
        pants = (Color){ 60, 65, 80, 255 };
        hair = (Color){ 60, 45, 35, 255 };
        hair_style = 3;
    } else if (npc->sprite_id == 5) { // 혜진 (동기 - 지적 보라색 정장 자켓 & 안경)
        shirt = (Color){ 160, 110, 220, 255 };
        pants = (Color){ 40, 40, 50, 255 };
        hair = (Color){ 35, 30, 40, 255 };
        hair_style = 0;
        glasses = true;
    } else if (npc->sprite_id == 6) { // 어르신 / 문방구 할머니 (백발 & 조끼)
        shirt = (Color){ 130, 140, 160, 255 };
        pants = (Color){ 60, 65, 70, 255 };
        hair = (Color){ 230, 230, 240, 255 };
        hair_style = 1;
        glasses = true;
    }

    DrawHumanCharacter(
        npc->x, npc->y, npc->dir, 30.0f, saturation,
        hair, shirt, pants, hair_style, glasses, false, false, anim_time
    );

    // Floating '!' Bubble
    if (!npc->talked_in_current_act) {
        float bounce = sinf(GetTime() * 4.0f) * 3.0f;
        int bx = (int)npc->x;
        int by = (int)(npc->y - 24 + bounce);
        DrawRectangle(bx - 6, by - 8, 12, 12, (Color){ 255, 255, 255, 230 });
        DrawRectangleLines(bx - 6, by - 8, 12, 12, (Color){ 20, 20, 20, 255 });
        RenderWorld_DrawPixelText("!", bx - 2, by - 7, 10, (Color){ 240, 60, 40, 255 });

        int name_w = (int)strlen(npc->name) * 8 + 8;
        DrawRectangle(bx - name_w / 2, by - 20, name_w, 13, (Color){ 20, 20, 24, 190 });
        RenderWorld_DrawPixelText(npc->name, bx - name_w / 2 + 4, by - 19, 10, (Color){ 255, 255, 255, 255 });
    }
}

// =========================================================================
// DrawPixelFace: 픽셀 아트 얼굴 — lean_angle 만큼 기울어져 화면 안쪽을 향해 내려다봄
// =========================================================================

// Rotation helpers — use static vars instead of macros to avoid compound-literal comma issue
static int   s_ox, s_oy;
static float s_cs, s_sn;

static inline void RC(float rx, float ry, int r, Color col) {
    DrawCircle(s_ox + (int)(s_cs*rx - s_sn*ry), s_oy + (int)(s_sn*rx + s_cs*ry), r, col);
}
static inline void RL(float x0, float y0, float x1, float y1, Color col) {
    DrawLine(s_ox + (int)(s_cs*x0 - s_sn*y0), s_oy + (int)(s_sn*x0 + s_cs*y0),
             s_ox + (int)(s_cs*x1 - s_sn*y1), s_oy + (int)(s_sn*x1 + s_cs*y1), col);
}
static inline void RR(float rx, float ry, int w, int h, Color col) {
    DrawRectangle(s_ox + (int)(s_cs*rx - s_sn*ry) - w/2,
                  s_oy + (int)(s_sn*rx + s_cs*ry) - h/2, w, h, col);
}

static void DrawPixelFace(int ox, int oy, float lean_angle,
                          Color skin, Color hair_col, Color clothes_col,
                          bool has_glasses, bool curly_hair, bool grey_hair,
                          bool has_hat, bool open_mouth, float phase) {

    float t = GetTime();
    oy += (int)(sinf(t * 3.8f + phase) * 2.5f);

    s_ox = ox; s_oy = oy;
    s_cs = cosf(lean_angle);
    s_sn = sinf(lean_angle);

    Color dark   = (Color){ 25, 20, 18, 255 };
    Color blush  = (Color){ 255, 140, 128, 120 };
    Color white  = (Color){ 250, 248, 240, 255 };
    Color tongue = (Color){ 228, 98, 108, 255 };
    Color wrinkle= (Color){ 168, 132, 112, 140 };
    Color nose_c = (Color){ (unsigned char)(skin.r>22?skin.r-22:0),
                            (unsigned char)(skin.g>20?skin.g-20:0),
                            (unsigned char)(skin.b>17?skin.b-17:0), 255 };

    // ── 옷 + 목 + 얼굴 베이스 ──
    RC(0, 38, 32, clothes_col);
    RR(0, 22, 13, 18, skin);
    RC(0,  0, 28, skin);

    // ── 머리카락 ──
    if (has_hat) {
        Color hat1 = (Color){ 78, 72, 65, 255 };
        Color hat2 = (Color){ 65, 60, 55, 255 };
        RR(2, -28, 56, 10, hat1);
        RR(0, -36, 38, 14, hat2);
        RR(0, -42, 28,  8, hat1);
    } else if (curly_hair) {
        RC( 0, -24, 24, hair_col);
        RC(-20, -18, 14, hair_col);
        RC( 20, -18, 14, hair_col);
        RC(-25,  -6, 11, hair_col);
        RC( 25,  -6, 11, hair_col);
        RC(-14, -28, 10, hair_col);
        RC( 14, -28, 10, hair_col);
        if (grey_hair) {
            Color hl1 = (Color){ 225, 225, 230, 200 };
            Color hl2 = (Color){ 215, 215, 220, 180 };
            RC(-6, -30, 5, hl1);
            RC( 8, -28, 4, hl2);
        }
    } else {
        RC( 0, -22, 26, hair_col);
        RC(-22, -12, 13, hair_col);
        RC( 22, -12, 13, hair_col);
        RR( 0, -22, 50, 12, hair_col);
    }

    // ── 눈 (^ ^ 웃는 눈호) ──
    RL(-14,-6, -8,-12, dark); RL(-8,-12, -2,-6, dark);
    RL(  2,-6,  8,-12, dark); RL( 8,-12, 14,-6, dark);
    Color shine = (Color){ 255, 255, 255, 180 };
    RC(-7, -8, 2, shine);
    RC( 7, -8, 2, shine);

    // ── 안경 (사각 프레임) ──
    if (has_glasses) {
        Color gf = (Color){ 38, 32, 25, 255 };
        RL(-18,-14, -4,-14, gf); RL(-18,-4, -4,-4, gf);
        RL(-18,-14,-18, -4, gf); RL( -4,-14,-4, -4, gf);
        RL(  4,-14, 18,-14, gf); RL(  4, -4,18, -4, gf);
        RL(  4,-14,  4, -4, gf); RL( 18,-14,18, -4, gf);
        RL( -4, -9,  4,  -9, gf); // 브릿지
    }

    // ── 코 ──
    RC( 0, 5, 4, nose_c);
    RL(-4, 5, -6, 8, nose_c);
    RL( 4, 5,  6, 8, nose_c);

    // ── 볼 홍조 ──
    RC(-17, 8, 8, blush);
    RC( 17, 8, 8, blush);

    // ── 입 ──
    if (open_mouth) {
        int mw = 17 + (int)(sinf(t * 5.5f + phase) * 2.0f);
        int mh = 12 + (int)(sinf(t * 4.8f + phase + 1.0f) * 2.0f);
        Color mouth_dark = (Color){ 55, 18, 22, 255 };
        Color teeth2 = (Color){ 242, 240, 232, 255 };
        RC(0, 15, mw/2, mouth_dark);
        RR(0, 12, mw-2, 5, white);
        RR(0, 16+mh/2, mw-4, 4, teeth2);
        RC(0, 18, 5, tongue);
    } else {
        Color smile = (Color){ 175, 80, 80, 255 };
        RL(-10, 13, 0, 17, smile);
        RL(  0, 17, 10, 13, smile);
    }

    // ── 주름 (노인/어르신) ──
    if (grey_hair || has_hat) {
        RL(-22,-2, -25, 3, wrinkle); RL( 22,-2,  25, 3, wrinkle);
        RL(-18, 8, -20,14, wrinkle); RL( 18, 8,  20,14, wrinkle);
    }
}

void RenderWorld_DrawInfant1stPerson(float blink_alpha, float clarity, int milestone, float baby_hand_y, float mobile_angle) {
    int W = SCREEN_WIDTH;
    int H = SCREEN_HEIGHT;

    // 1. 따뜻한 연한 크림색 천장/벽 배경 (한국 아파트 실내)
    ClearBackground((Color){ 205, 215, 210, 255 });

    // 방 모서리 그림자
    DrawRectangleGradientV(0, 0, W, H/4, (Color){ 140, 150, 145, 80 }, (Color){ 0, 0, 0, 0 });

    // 형광등 (천장 중앙)
    DrawRectangle(W/2 - 55, 0, 110, 8, (Color){ 255, 255, 250, 255 });
    DrawRectangleGradientV(W/2 - 55, 8, 110, 30, (Color){ 255, 250, 230, 180 }, (Color){ 0, 0, 0, 0 });

    // 모빌 (천장에서 줄로 매달림)
    int mmx = W/2;
    DrawLine(mmx, 0, mmx, 55, (Color){ 160, 162, 168, 255 });
    DrawLine(mmx, 55, mmx + (int)(cosf(mobile_angle)*50), 55 + (int)(sinf(mobile_angle)*14), (Color){ 160, 162, 168, 255 });
    DrawLine(mmx, 55, mmx - (int)(cosf(mobile_angle)*40), 55 - (int)(sinf(mobile_angle)*12), (Color){ 160, 162, 168, 255 });
    DrawCircle(mmx + (int)(cosf(mobile_angle)*50), 55 + (int)(sinf(mobile_angle)*14) + 14, 9, (Color){ 255, 210, 80, 255 });
    DrawCircle(mmx - (int)(cosf(mobile_angle)*40), 55 - (int)(sinf(mobile_angle)*12) + 12, 7, (Color){ 120, 200, 255, 255 });

    // =========================================================================
    // ★ 사진처럼: 얼굴들이 4방향 가장자리에서 중앙을 향해 lean_angle로 기울어져 내려다봄
    // =========================================================================

    Color skin_k  = (Color){ 255, 218, 185, 255 };
    Color skin_e  = (Color){ 248, 208, 172, 255 };
    Color hair_bk = (Color){ 30, 25, 20, 255 };
    Color hair_gy = (Color){ 155, 150, 145, 255 };

    // ── 상단 (위에서 아래로 내려봄) ──
    // 정중앙 상단: 이모/누나 (뽀글이 파마 + 안경 + 체크 셔츠)
    DrawPixelFace(W/2, -5, 0.0f,
        skin_k, hair_bk, (Color){ 62, 78, 120, 255 },
        true, true, false, false, true, 0.0f);

    // 좌상단: 어머니 (뽀글이, 분홍 셔츠, 크게 웃음)
    DrawPixelFace(W/2 - 148, 15, 0.28f,
        skin_k, hair_bk, (Color){ 220, 100, 130, 255 },
        false, true, false, false, true, 1.3f);

    // 우상단: 할머니 (흰머리 뽀글이, 황갈색 한복, 주름)
    DrawPixelFace(W/2 + 148, 15, -0.28f,
        skin_e, hair_gy, (Color){ 215, 138, 55, 255 },
        false, true, true, false, true, 2.6f);

    // ── 좌측 (오른쪽으로 기울어져 내려봄) ──
    // 좌중단: 아버지 (단발, 남색 셔츠, 활짝)
    DrawPixelFace(10, H/2 - 50, (float)(PI/2) - 0.18f,
        skin_k, hair_bk, (Color){ 48, 80, 148, 255 },
        false, false, false, false, true, 3.9f);

    // 좌하단: 삼촌 (민트 셔츠, 웃음)
    DrawPixelFace(25, H/2 + 75, (float)(PI/2) + 0.22f,
        skin_k, (Color){ 45, 38, 32, 255 }, (Color){ 65, 185, 170, 255 },
        false, false, false, false, true, 5.1f);

    // ── 우측 (왼쪽으로 기울어져 내려봄) ──
    // 우중단: 큰아버지 (회색머리, 스트라이프 셔츠, 안경)
    DrawPixelFace(W - 10, H/2 - 50, -(float)(PI/2) + 0.18f,
        skin_e, hair_gy, (Color){ 155, 200, 238, 255 },
        true, false, true, false, true, 0.7f);

    // 우하단: 고모 (뽀글이, 옥색 블라우스)
    DrawPixelFace(W - 25, H/2 + 75, -(float)(PI/2) - 0.22f,
        skin_k, hair_bk, (Color){ 72, 188, 174, 255 },
        false, true, false, false, true, 2.0f);

    // ── 하단 중앙: 할아버지 (가장 크게! 헌팅캡 + 뿔테 안경 + 체크 셔츠) ──
    DrawPixelFace(W/2, H + 8, (float)PI,
        skin_e, hair_gy, (Color){ 218, 108, 48, 255 },
        true, false, true, true, true, 0.4f);

    // ── 하트 파티클 ──
    float hp = sinf(GetTime() * 3.2f) * 5.0f;
    RenderWorld_DrawPixelText("♥", W/2 - 20, H/2 - 58 + (int)hp, 18, (Color){ 255, 100, 130, 210 });
    RenderWorld_DrawPixelText("♥", W/2 + 26, H/2 - 78 - (int)hp, 14, (Color){ 255, 180, 80, 200 });
    RenderWorld_DrawPixelText("♥", W/2 - 48, H/2 - 45 - (int)hp, 12, (Color){ 255, 130, 170, 190 });

    // ── Milestone 1: 아빠가 손가락을 내밀어줌 ──
    if (milestone >= 1) {
        int fx = W/2 - 58;
        int fy = H/2 - 12;
        DrawCircle(fx, fy, 12, (Color){ 255, 218, 185, 255 });
        DrawRectangle(fx - 7, fy, 14, 48, (Color){ 255, 218, 185, 255 });
        DrawCircle(fx - 8, fy - 6, 5, (Color){ 255, 218, 185, 255 });
        DrawCircle(fx,     fy - 11, 5, (Color){ 255, 218, 185, 255 });
        DrawCircle(fx + 8, fy - 7,  5, (Color){ 255, 218, 185, 255 });
    }

    // ── 아기 통통한 손 ──
    int hx = W/2 + 32;
    int hy = (int)baby_hand_y;
    DrawCircle(hx, hy, 15, (Color){ 255, 225, 198, 255 });
    DrawRectangle(hx - 8, hy, 16, 55, (Color){ 255, 225, 198, 255 });
    DrawCircle(hx - 9, hy -  7, 5, (Color){ 255, 225, 198, 255 });
    DrawCircle(hx - 4, hy - 12, 5, (Color){ 255, 225, 198, 255 });
    DrawCircle(hx + 2, hy - 13, 5, (Color){ 255, 225, 198, 255 });
    DrawCircle(hx + 8, hy -  9, 5, (Color){ 255, 225, 198, 255 });
    DrawLine(hx - 8, hy + 10, hx + 8, hy + 10, (Color){ 240, 200, 170, 155 });
    DrawLine(hx - 7, hy + 22, hx + 7, hy + 22, (Color){ 240, 200, 170, 125 });

    // ── 시야 흐림 (아기 눈) ──
    if (clarity < 0.95f) {
        DrawRectangle(0, 0, W, H, (Color){ 208, 210, 215, (unsigned char)(145 * (1.0f - clarity)) });
    }

    // ── 눈꺼풀 ──
    if (blink_alpha > 0.01f) {
        int lid = (int)((H / 2) * blink_alpha);
        DrawRectangle(0, 0, W, lid, (Color){ 8, 8, 10, 255 });
        DrawRectangle(0, H - lid, W, lid, (Color){ 8, 8, 10, 255 });
    }

    // ── 안내 텍스트 ──
    if (milestone == 0) {
        RenderWorld_DrawPixelText("[ SPACE: 눈 깜빡이기 — 가족들이 보여요 ]",
            W/2 - 155, H - 30, 14, (Color){ 40, 42, 48, 230 });
    } else if (milestone == 1) {
        RenderWorld_DrawPixelText("[ SPACE: 아빠 손가락을 꼭 쥐기 ]",
            W/2 - 120, H - 30, 14, (Color){ 220, 80, 60, 255 });
    } else if (milestone == 2) {
        RenderWorld_DrawPixelText("[ 방향키: 가족의 응원 속에 첫 걸음마! ]",
            W/2 - 145, H - 30, 14, (Color){ 20, 120, 200, 255 });
    }
}

void RenderWorld_DrawColorBloomWave(float center_x, float center_y, float radius, float max_radius) {
    if (radius <= 0.0f || radius >= max_radius) return;

    DrawCircleLines((int)center_x, (int)center_y, radius, (Color){ 255, 220, 100, 200 });
    DrawCircleLines((int)center_x, (int)center_y, radius + 2.0f, (Color){ 255, 140, 180, 160 });
    DrawCircleLines((int)center_x, (int)center_y, radius + 4.0f, (Color){ 120, 230, 255, 120 });

    for (int i = 0; i < 8; i++) {
        float angle = (float)i * (2.0f * PI / 8.0f) + radius * 0.02f;
        int sx = (int)(center_x + cosf(angle) * radius);
        int sy = (int)(center_y + sinf(angle) * radius);
        if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
            RenderWorld_DrawPixelText("♥", sx - 4, sy - 6, 12, (Color){ 255, 100, 140, 220 });
        }
    }
}

void RenderWorld_DrawWarpFade(float fade_alpha) {
    if (fade_alpha <= 0.001f) return;
    if (fade_alpha > 1.0f) fade_alpha = 1.0f;
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 10, 10, 12, (unsigned char)(255 * fade_alpha) });
}

void RenderWorld_DrawLocationBanner(const char* location_name, float timer) {
    if (timer <= 0.01f || !location_name || strlen(location_name) == 0) return;
    float alpha = (timer > 2.0f) ? (3.0f - timer) : ((timer < 0.5f) ? (timer / 0.5f) : 1.0f);
    if (alpha > 1.0f) alpha = 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;

    int text_w = (int)strlen(location_name) * 10 + 24;
    int bx = SCREEN_WIDTH / 2 - text_w / 2;
    int by = 20;
    DrawRectangle(bx, by, text_w, 26, (Color){ 20, 22, 28, (unsigned char)(220 * alpha) });
    DrawRectangleLines(bx, by, text_w, 26, (Color){ 240, 235, 210, (unsigned char)(240 * alpha) });
    RenderWorld_DrawPixelText(location_name, bx + 12, by + 6, 14, (Color){ 255, 255, 255, (unsigned char)(255 * alpha) });
}

void RenderWorld_DrawDialogueBox(const char* speaker, const char* role_title,
                                 const char* line1, const char* line2, 
                                 const char* choice_a, const char* choice_b, int selected_choice, 
                                 float saturation) {
    (void)saturation;
    int box_x = 20;
    int box_y = SCREEN_HEIGHT - 105;
    int box_w = SCREEN_WIDTH - 40;
    int box_h = 90;

    DrawRectangle(box_x, box_y, box_w, box_h, (Color){ 24, 26, 28, 245 });
    DrawRectangleLines(box_x, box_y, box_w, box_h, (Color){ 240, 235, 220, 255 });
    DrawRectangleLines(box_x + 2, box_y + 2, box_w - 4, box_h - 4, (Color){ 80, 85, 90, 255 });

    if (speaker && strlen(speaker) > 0) {
        char tag_text[64];
        if (role_title && strlen(role_title) > 0) {
            snprintf(tag_text, sizeof(tag_text), "%s [%s]", speaker, role_title);
        } else {
            snprintf(tag_text, sizeof(tag_text), "%s", speaker);
        }
        int tag_w = (int)strlen(tag_text) * 8 + 16;
        DrawRectangle(box_x + 12, box_y - 12, tag_w, 18, (Color){ 240, 107, 50, 255 });
        DrawRectangleLines(box_x + 12, box_y - 12, tag_w, 18, (Color){ 255, 255, 255, 255 });
        RenderWorld_DrawPixelText(tag_text, box_x + 18, box_y - 10, 12, (Color){ 255, 255, 255, 255 });
    }

    if (line1) RenderWorld_DrawPixelText(line1, box_x + 16, box_y + 12, 14, (Color){ 245, 245, 245, 255 });
    if (line2) RenderWorld_DrawPixelText(line2, box_x + 16, box_y + 34, 14, (Color){ 220, 220, 220, 255 });

    if (choice_a && choice_b && strlen(choice_a) > 0) {
        int cy = box_y + 56;
        Color col_a = (selected_choice == 0) ? (Color){ 255, 215, 64, 255 } : (Color){ 160, 160, 160, 255 };
        const char* prefix_a = (selected_choice == 0) ? "▶ " : "  ";
        char buf_a[96];
        snprintf(buf_a, sizeof(buf_a), "%s%s", prefix_a, choice_a);
        RenderWorld_DrawPixelText(buf_a, box_x + 18, cy, 13, col_a);

        Color col_b = (selected_choice == 1) ? (Color){ 255, 215, 64, 255 } : (Color){ 160, 160, 160, 255 };
        const char* prefix_b = (selected_choice == 1) ? "▶ " : "  ";
        char buf_b[96];
        snprintf(buf_b, sizeof(buf_b), "%s%s", prefix_b, choice_b);
        RenderWorld_DrawPixelText(buf_b, box_x + 310, cy, 13, col_b);
    } else {
        RenderWorld_DrawPixelText("▶ [Z / SPACE : 계속]", box_x + box_w - 150, box_y + box_h - 18, 12, (Color){ 180, 180, 180, 200 });
    }
}

void RenderWorld_DrawHUD(const PlayerState* player, float lived_years, float saturation) {
    (void)saturation;
    DrawRectangle(10, 10, 200, 48, (Color){ 20, 20, 24, 210 });
    DrawRectangleLines(10, 10, 200, 48, (Color){ 200, 200, 210, 255 });

    char age_str[64];
    snprintf(age_str, sizeof(age_str), "나이: %.0f세 (인생 %.0f년)", player->age, lived_years);
    RenderWorld_DrawPixelText(age_str, 16, 14, 13, (Color){ 255, 255, 255, 255 });

    const char* map_loc = "우리 동네 골목길";
    if (player->current_map == MAP_MY_HOME) map_loc = "우리 집 안방";
    else if (player->current_map == MAP_STATIONERY_STORE) map_loc = "그린 문방구";
    else if (player->current_map == MAP_CLASSROOM) map_loc = "1학년 2반 교실";
    else if (player->current_map == MAP_POJANGMACHA_INTERIOR) map_loc = "골목 포장마차";
    else if (player->current_map == MAP_APARTMENT_STAIRS) map_loc = "아파트 계단실 (3F)";
    else if (player->current_map == MAP_APARTMENT_302) map_loc = "우리 집 아파트 302호";
    else if (player->current_map == MAP_OFFICE_INTERIOR) map_loc = "회사 사무실";
    else if (player->current_map == MAP_SENIOR_HALL) map_loc = "동네 경로당";

    Color act_color = (player->true_love_awakened) ? (Color){ 255, 140, 180, 255 } : (Color){ 180, 180, 180, 255 };
    RenderWorld_DrawPixelText(map_loc, 16, 30, 11, act_color);

    char bf_str[48];
    snprintf(bf_str, sizeof(bf_str), "절친: %s", player->best_friend_name);
    RenderWorld_DrawPixelText(bf_str, 16, 42, 10, (Color){ 84, 201, 206, 255 });
}
