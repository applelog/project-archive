#include "render_world.h"
#include "font_korean.h"
#include <math.h>
#include <string.h>

void Render_Init(void) {
    FontKorean_Init();
}

void Render_DrawFake3DShadow(float x, float y, float rx, float ry) {
    Color shadow = (Color){ 12, 16, 14, 110 };
    DrawEllipse((int)x, (int)(y + 8), rx, ry, shadow);
}

void Render_DrawCharacter(float x, float y, Direction dir, int anim_frame, bool is_player, float saturation) {
    // 1. Fake 3D Grounding Drop Shadow
    Render_DrawFake3DShadow(x + 12, y + 20, 9.0f, 4.5f);
    
    // 2. Palettes
    GBCPalette pal = Palette_GetDefault();
    Color skinCol  = Palette_ApplyMonochrome(pal.skin_tone, saturation);
    Color hairCol  = Palette_ApplyMonochrome(pal.bg_dark, saturation);
    Color dressCol = is_player ? Palette_ApplyMonochrome(pal.dress_white, saturation)
                               : Palette_ApplyMonochrome(pal.poke_blue, saturation);
    
    int bob_y = (anim_frame % 2 == 1) ? 1 : 0;
    
    // Hair & Head Outline (Pokemon Gold Trainer Style)
    DrawRectangle((int)x + 3, (int)y - 2 + bob_y, 18, 14, hairCol);
    DrawRectangle((int)x + 5, (int)y + 3 + bob_y, 14, 10, skinCol);
    
    // Direction-aware facial features
    if (dir == DIR_DOWN) {
        // Expressive anime eyes
        DrawRectangle((int)x + 7,  (int)y + 6 + bob_y, 2, 3, hairCol);
        DrawRectangle((int)x + 15, (int)y + 6 + bob_y, 2, 3, hairCol);
        // Catchlights / Eye glint
        DrawRectangle((int)x + 7,  (int)y + 6 + bob_y, 1, 1, WHITE);
        DrawRectangle((int)x + 15, (int)y + 6 + bob_y, 1, 1, WHITE);
        // Soft blush
        DrawRectangle((int)x + 6,  (int)y + 9 + bob_y, 2, 1, Palette_ApplyMonochrome((Color){ 245, 140, 150, 220 }, saturation));
        DrawRectangle((int)x + 16, (int)y + 9 + bob_y, 2, 1, Palette_ApplyMonochrome((Color){ 245, 140, 150, 220 }, saturation));
    } else if (dir == DIR_LEFT) {
        DrawRectangle((int)x + 5,  (int)y + 6 + bob_y, 2, 3, hairCol);
        DrawRectangle((int)x + 5,  (int)y + 6 + bob_y, 1, 1, WHITE);
        DrawRectangle((int)x + 5,  (int)y + 9 + bob_y, 2, 1, Palette_ApplyMonochrome((Color){ 245, 140, 150, 220 }, saturation));
    } else if (dir == DIR_RIGHT) {
        DrawRectangle((int)x + 17, (int)y + 6 + bob_y, 2, 3, hairCol);
        DrawRectangle((int)x + 17, (int)y + 6 + bob_y, 1, 1, WHITE);
        DrawRectangle((int)x + 17, (int)y + 9 + bob_y, 2, 1, Palette_ApplyMonochrome((Color){ 245, 140, 150, 220 }, saturation));
    } else { // DIR_UP
        DrawRectangle((int)x + 4, (int)y + 2 + bob_y, 16, 12, hairCol);
        DrawRectangle((int)x + 8, (int)y + 4 + bob_y, 8, 4, Palette_ApplyMonochrome((Color){ 45, 42, 50, 255 }, saturation)); // Hair tie
    }
    
    // NPC uniform cap & gold badge
    if (!is_player) {
        DrawRectangle((int)x + 3, (int)y - 4 + bob_y, 18, 5, Palette_ApplyMonochrome((Color){ 30, 45, 80, 255 }, saturation));
        DrawRectangle((int)x + 10, (int)y - 3 + bob_y, 4, 3, Palette_ApplyMonochrome(pal.poke_gold, saturation));
    }
    
    // Body / Outfit
    DrawRectangle((int)x + 4, (int)y + 13 + bob_y, 16, 11, dressCol);
    DrawRectangleLines((int)x + 4, (int)y + 13 + bob_y, 16, 11, hairCol);
    
    // Waist Ribbon / Sash
    if (is_player) {
        DrawRectangle((int)x + 4, (int)y + 15 + bob_y, 16, 2, Palette_ApplyMonochrome(pal.poke_gold, saturation));
    }
    
    // Stepping Feet Animation
    if (anim_frame % 2 == 1) {
        DrawRectangle((int)x + 6, (int)y + 23, 4, 3, hairCol);
        DrawRectangle((int)x + 14, (int)y + 22, 4, 3, hairCol);
    } else {
        DrawRectangle((int)x + 6, (int)y + 22, 4, 3, hairCol);
        DrawRectangle((int)x + 14, (int)y + 23, 4, 3, hairCol);
    }
}

void Render_DrawMovingSubway(const SubwayTrain* train, float saturation) {
    if (!train) return;
    GBCPalette pal = Palette_GetDefault();
    Color carBody  = Palette_ApplyMonochrome((Color){ 220, 225, 230, 255 }, saturation);
    Color lineCol  = Palette_ApplyMonochrome(pal.subway_line, saturation);
    Color windowCol= Palette_ApplyMonochrome((Color){ 35, 55, 80, 255 }, saturation);
    Color roofCol  = Palette_ApplyMonochrome((Color){ 165, 172, 182, 255 }, saturation);
    Color underShadow = (Color){ 10, 10, 15, 200 };
    
    int carWidth = 260;
    int carHeight = 78;
    int trackY = 65;
    
    for (int i = 0; i < MAX_TRAIN_CARS; i++) {
        int carX = (int)train->x + i * (carWidth + 12);
        if (carX + carWidth < -120 || carX > SCREEN_WIDTH + 120) continue;
        
        // 1. Heavy Occlusion Shadow under the train
        DrawRectangle(carX - 8, trackY + carHeight - 4, carWidth + 16, 16, underShadow);
        
        // 2. Train Roof Curve & Vent pods
        DrawRectangle(carX, trackY - 6, carWidth, 8, roofCol);
        DrawRectangleLines(carX, trackY - 6, carWidth, 8, pal.bg_dark);
        for (int v = 20; v < carWidth - 20; v += 60) {
            DrawRectangle(carX + v, trackY - 9, 24, 4, Palette_ApplyMonochrome((Color){ 120, 125, 135, 255 }, saturation));
            DrawRectangleLines(carX + v, trackY - 9, 24, 4, pal.bg_dark);
        }
        
        // 3. Train Body
        DrawRectangle(carX, trackY, carWidth, carHeight, carBody);
        DrawRectangleLines(carX, trackY, carWidth, carHeight, pal.bg_dark);
        
        // 4. Line 2 Green Accent Stripe & Metallic Seam
        DrawRectangle(carX, trackY + 40, carWidth, 10, lineCol);
        DrawLine(carX, trackY + 40, carX + carWidth, trackY + 40, WHITE);
        DrawLine(carX, trackY + 50, carX + carWidth, trackY + 50, pal.bg_dark);
        
        // 5. Windows with Warm Interior Passengers
        for (int w = 0; w < 4; w++) {
            int winX = carX + 18 + w * 58;
            DrawRectangle(winX, trackY + 10, 42, 26, windowCol);
            DrawRectangleLines(winX, trackY + 10, 42, 26, pal.bg_dark);
            
            // Warm interior lighting & passenger silhouettes
            DrawRectangle(winX + 2, trackY + 12, 38, 22, Palette_ApplyMonochrome((Color){ 100, 135, 165, 255 }, saturation));
            DrawCircle(winX + 14, trackY + 24, 5, Palette_ApplyMonochrome((Color){ 30, 35, 45, 255 }, saturation));
            DrawCircle(winX + 28, trackY + 24, 5, Palette_ApplyMonochrome((Color){ 30, 35, 45, 255 }, saturation));
            
            // Specular window glass sheen
            DrawLine(winX + 4, trackY + 32, winX + 16, trackY + 14, WHITE);
            DrawLine(winX + 22, trackY + 32, winX + 34, trackY + 14, WHITE);
        }
        
        // 6. Sliding Passenger Doors with Red/Green LED
        int doorOffset = (int)(train->door_open_ratio * 16.0f);
        DrawRectangle(carX + 112 - doorOffset, trackY + 14, 18, 56, carBody);
        DrawRectangleLines(carX + 112 - doorOffset, trackY + 14, 18, 56, pal.bg_dark);
        DrawRectangle(carX + 114 - doorOffset, trackY + 18, 14, 22, windowCol);
        
        DrawRectangle(carX + 130 + doorOffset, trackY + 14, 18, 56, carBody);
        DrawRectangleLines(carX + 130 + doorOffset, trackY + 14, 18, 56, pal.bg_dark);
        DrawRectangle(carX + 132 + doorOffset, trackY + 18, 14, 22, windowCol);
        
        // Safety Warning Stripe on Doors
        DrawRectangle(carX + 112 - doorOffset, trackY + 44, 18, 4, Palette_ApplyMonochrome(pal.poke_gold, saturation));
        DrawRectangle(carX + 130 + doorOffset, trackY + 44, 18, 4, Palette_ApplyMonochrome(pal.poke_gold, saturation));
        
        Color ledCol = (train->door_open_ratio > 0.8f) ? (Color){ 80, 230, 80, 255 } : (Color){ 230, 60, 60, 255 };
        DrawRectangle(carX + 128, trackY + 8, 4, 3, ledCol);
    }
}

void Render_DrawAreaMap(AreaId area, float saturation) {
    GBCPalette pal = Palette_GetDefault();
    
    // =========================================================================
    // 1. AREA_SUBWAY_PLATFORM: 서울 지하철 2호선 승강장
    // =========================================================================
    if (area == AREA_SUBWAY_PLATFORM) {
        Color floorCol   = Palette_ApplyMonochrome((Color){ 82, 90, 94, 255 }, saturation);
        Color brailleTile= Palette_ApplyMonochrome(pal.poke_gold, saturation);
        Color pillarCol  = Palette_ApplyMonochrome((Color){ 52, 60, 64, 255 }, saturation);
        
        ClearBackground(floorCol);
        
        // Rails Area (Top)
        DrawRectangle(0, 0, SCREEN_WIDTH, 140, Palette_ApplyMonochrome((Color){ 25, 28, 32, 255 }, saturation));
        for (int r = 0; r < SCREEN_WIDTH; r += 24) {
            DrawRectangle(r, 68, 10, 72, Palette_ApplyMonochrome(pal.poke_wood, saturation));
        }
        DrawRectangle(0, 85, SCREEN_WIDTH, 6, Palette_ApplyMonochrome((Color){ 140, 145, 150, 255 }, saturation));
        DrawRectangle(0, 120, SCREEN_WIDTH, 6, Palette_ApplyMonochrome((Color){ 140, 145, 150, 255 }, saturation));
        
        // Platform Edge Hazard Stripe (Black/Yellow)
        for (int x = 0; x < SCREEN_WIDTH; x += 16) {
            DrawRectangle(x, 140, 8, 5, brailleTile);
            DrawRectangle(x + 8, 140, 8, 5, pal.bg_dark);
        }
        // Braille Tactile Bumps Line
        DrawRectangle(0, 146, SCREEN_WIDTH, 8, brailleTile);
        for (int bx = 0; bx < SCREEN_WIDTH; bx += 8) {
            DrawCircle(bx + 4, 150, 1.5f, Palette_ApplyMonochrome((Color){ 180, 130, 20, 255 }, saturation));
        }
        
        // Floor Tile Seams Grid
        for (int gx = 0; gx < SCREEN_WIDTH; gx += 32) {
            DrawLine(gx, 154, gx, SCREEN_HEIGHT, Palette_ApplyMonochrome((Color){ 68, 75, 80, 255 }, saturation));
        }
        for (int gy = 154; gy < SCREEN_HEIGHT; gy += 32) {
            DrawLine(0, gy, SCREEN_WIDTH, gy, Palette_ApplyMonochrome((Color){ 68, 75, 80, 255 }, saturation));
        }
        
        // Pillars with Station Signboards & Fake 3D Shadows
        for (int p = 70; p < SCREEN_WIDTH; p += 190) {
            Render_DrawFake3DShadow(p + 16, 230, 24.0f, 8.0f);
            DrawRectangle(p, 160, 32, 70, pillarCol);
            DrawRectangleLines(p, 160, 32, 70, pal.bg_dark);
            
            // Green Line 2 Station Signboard
            DrawRectangle(p + 3, 178, 26, 12, pal.subway_line);
            DrawRectangleLines(p + 3, 178, 26, 12, WHITE);
            FontKorean_DrawText("2호선", p + 5, 180, 8, WHITE);
        }
        
        // Platform Benches with Wood Texture
        Render_DrawFake3DShadow(320, 280, 28.0f, 8.0f);
        DrawRectangle(296, 262, 48, 16, Palette_ApplyMonochrome(pal.poke_wood, saturation));
        DrawRectangleLines(296, 262, 48, 16, pal.bg_dark);
        DrawLine(296, 270, 344, 270, Palette_ApplyMonochrome((Color){ 120, 70, 35, 255 }, saturation));
        
        // Vending Machine
        Render_DrawFake3DShadow(530, 240, 20.0f, 6.0f);
        DrawRectangle(515, 185, 30, 52, Palette_ApplyMonochrome(pal.poke_red, saturation));
        DrawRectangleLines(515, 185, 30, 52, pal.bg_dark);
        DrawRectangle(520, 192, 20, 20, Palette_ApplyMonochrome((Color){ 40, 60, 80, 255 }, saturation));
        DrawRectangle(522, 196, 4, 6, pal.poke_gold);
        DrawRectangle(528, 196, 4, 6, pal.subway_line);
        DrawRectangle(534, 196, 4, 6, pal.poke_red);
    }
    // =========================================================================
    // 2. AREA_MARKET_ALLEY: 옛 골목 재래시장
    // =========================================================================
    else if (area == AREA_MARKET_ALLEY) {
        Color asphalt = Palette_ApplyMonochrome((Color){ 65, 68, 72, 255 }, saturation);
        ClearBackground(asphalt);
        
        // Concrete Sidewalk
        DrawRectangle(0, 0, SCREEN_WIDTH, 60, Palette_ApplyMonochrome((Color){ 150, 155, 160, 255 }, saturation));
        DrawLine(0, 60, SCREEN_WIDTH, 60, pal.bg_dark);
        
        // Brick Storefronts & Market Stalls
        for (int i = 0; i < 4; i++) {
            int sx = 30 + i * 150;
            Render_DrawFake3DShadow(sx + 55, 130, 50.0f, 12.0f);
            DrawRectangle(sx, 50, 110, 75, Palette_ApplyMonochrome(pal.poke_brick, saturation));
            DrawRectangleLines(sx, 50, 110, 75, pal.bg_dark);
            
            // Striped Awning Roof
            for (int stripe = 0; stripe < 110; stripe += 14) {
                Color sCol = ((stripe / 14) % 2 == 0) ? pal.poke_red : pal.fg_highlight;
                DrawRectangle(sx + stripe, 45, 14, 20, Palette_ApplyMonochrome(sCol, saturation));
            }
            DrawRectangleLines(sx, 45, 110, 20, pal.bg_dark);
            
            // Display Crates with Vegetables & Fish
            DrawRectangle(sx + 10, 95, 40, 25, Palette_ApplyMonochrome(pal.poke_wood, saturation));
            DrawRectangle(sx + 55, 95, 45, 25, Palette_ApplyMonochrome(pal.poke_wood, saturation));
            // Cabbage / Radish / Peppers
            DrawCircle(sx + 20, 107, 5, Palette_ApplyMonochrome(pal.poke_leaf_light, saturation));
            DrawCircle(sx + 32, 107, 5, Palette_ApplyMonochrome(pal.poke_leaf_light, saturation));
            DrawCircle(sx + 68, 107, 4, Palette_ApplyMonochrome(pal.poke_red, saturation));
            DrawCircle(sx + 80, 107, 4, Palette_ApplyMonochrome(pal.poke_red, saturation));
        }
        
        // Zebra Crosswalk on Street
        for (int cw = 220; cw < 340; cw += 18) {
            DrawRectangle(cw, 140, 12, 100, Palette_ApplyMonochrome((Color){ 240, 245, 250, 255 }, saturation));
        }
        
        // Pyeongsang (Wooden Rest Bench)
        Render_DrawFake3DShadow(460, 240, 32.0f, 10.0f);
        DrawRectangle(430, 220, 60, 22, Palette_ApplyMonochrome(pal.poke_wood, saturation));
        DrawRectangleLines(430, 220, 60, 22, pal.bg_dark);
        
        // Return Subway Station Entrance Portal at bottom left
        DrawRectangle(20, SCREEN_HEIGHT - 65, 90, 45, Palette_ApplyMonochrome(pal.subway_line, saturation));
        DrawRectangleLines(20, SCREEN_HEIGHT - 65, 90, 45, WHITE);
        FontKorean_DrawText("지하철역", 32, SCREEN_HEIGHT - 50, 16, WHITE);
    }
    // =========================================================================
    // 3. AREA_SCHOOL_PLAYGROUND: 초등학교 앞 놀이터
    // =========================================================================
    else if (area == AREA_SCHOOL_PLAYGROUND) {
        Color sandGround = Palette_ApplyMonochrome((Color){ 218, 198, 158, 255 }, saturation);
        ClearBackground(sandGround);
        
        // School Brick Building & Iron Fence in Background
        DrawRectangle(0, 0, SCREEN_WIDTH, 60, Palette_ApplyMonochrome(pal.poke_brick, saturation));
        for (int f = 0; f < SCREEN_WIDTH; f += 12) {
            DrawRectangle(f, 50, 2, 18, pal.bg_dark);
        }
        DrawLine(0, 50, SCREEN_WIDTH, 50, pal.bg_dark);
        DrawLine(0, 60, SCREEN_WIDTH, 60, pal.bg_dark);
        
        // Big Cherry Blossom Tree (Pokemon GBC Style)
        Render_DrawFake3DShadow(530, 120, 36.0f, 12.0f);
        DrawRectangle(522, 90, 16, 30, Palette_ApplyMonochrome(pal.poke_wood, saturation));
        DrawCircle(530, 70, 34, Palette_ApplyMonochrome((Color){ 255, 175, 195, 255 }, saturation));
        DrawCircle(520, 58, 26, Palette_ApplyMonochrome((Color){ 255, 205, 220, 255 }, saturation));
        
        // Soccer Goal with Netting
        Render_DrawFake3DShadow(120, 150, 42.0f, 10.0f);
        DrawRectangleLines(80, 80, 80, 68, WHITE);
        for (int net = 88; net < 160; net += 12) {
            DrawLine(net, 80, net - 10, 148, Palette_ApplyMonochrome((Color){ 230, 235, 240, 180 }, saturation));
        }
        
        // Children's Swings
        Render_DrawFake3DShadow(440, 170, 35.0f, 10.0f);
        DrawLine(410, 90, 395, 170, pal.bg_dark);
        DrawLine(470, 90, 485, 170, pal.bg_dark);
        DrawLine(395, 90, 485, 90, pal.bg_dark);
        DrawLine(425, 90, 422, 145, pal.bg_dark);
        DrawLine(437, 90, 435, 145, pal.bg_dark);
        DrawRectangle(418, 145, 22, 6, Palette_ApplyMonochrome(pal.poke_wood, saturation));
        
        // Return Subway Station Portal
        DrawRectangle(20, SCREEN_HEIGHT - 65, 90, 45, Palette_ApplyMonochrome(pal.subway_line, saturation));
        DrawRectangleLines(20, SCREEN_HEIGHT - 65, 90, 45, WHITE);
        FontKorean_DrawText("지하철역", 32, SCREEN_HEIGHT - 50, 16, WHITE);
    }
    // =========================================================================
    // 4. AREA_OLD_HOUSE: 추억의 옛집 단독주택 (안방)
    // =========================================================================
    else if (area == AREA_OLD_HOUSE) {
        Color woodFloor = Palette_ApplyMonochrome(pal.poke_wood, saturation);
        Color wallPaper = Palette_ApplyMonochrome((Color){ 242, 238, 224, 255 }, saturation);
        ClearBackground(woodFloor);
        
        // Floor Plank Lines
        for (int y = 90; y < SCREEN_HEIGHT; y += 24) {
            DrawLine(0, y, SCREEN_WIDTH, y, Palette_ApplyMonochrome((Color){ 140, 85, 45, 255 }, saturation));
        }
        
        // Wallpaper & Ceiling Molding
        DrawRectangle(0, 0, SCREEN_WIDTH, 90, wallPaper);
        DrawRectangle(0, 85, SCREEN_WIDTH, 6, Palette_ApplyMonochrome((Color){ 130, 80, 40, 255 }, saturation));
        
        // Window with Sunbeams
        DrawRectangle(80, 20, 70, 50, Palette_ApplyMonochrome(pal.poke_silver, saturation));
        DrawRectangleLines(80, 20, 70, 50, pal.bg_dark);
        DrawLine(115, 20, 115, 70, pal.bg_dark);
        DrawLine(80, 45, 150, 45, pal.bg_dark);
        
        // Traditional Lacquer Wardrobe (자개장) with Inlay Shimmer
        Render_DrawFake3DShadow(280, 105, 48.0f, 12.0f);
        DrawRectangle(235, 30, 90, 70, Palette_ApplyMonochrome((Color){ 20, 22, 26, 255 }, saturation));
        DrawRectangleLines(235, 30, 90, 70, pal.poke_gold);
        DrawCircle(260, 65, 4, (Color){ 180, 240, 240, 255 });
        DrawCircle(300, 65, 4, (Color){ 180, 240, 240, 255 });
        DrawRectangle(278, 60, 4, 10, pal.poke_gold);
        
        // Vanity Table with Mirror
        Render_DrawFake3DShadow(440, 100, 30.0f, 10.0f);
        DrawRectangle(415, 45, 50, 50, Palette_ApplyMonochrome(pal.poke_wood, saturation));
        DrawRectangle(422, 15, 36, 42, Palette_ApplyMonochrome((Color){ 175, 210, 235, 255 }, saturation));
        DrawRectangleLines(422, 15, 36, 42, pal.bg_dark);
        DrawLine(426, 50, 448, 20, WHITE);
        
        // CRT Television with Antenna
        Render_DrawFake3DShadow(550, 100, 25.0f, 8.0f);
        DrawRectangle(530, 50, 40, 44, Palette_ApplyMonochrome((Color){ 40, 42, 48, 255 }, saturation));
        DrawRectangle(535, 56, 24, 22, Palette_ApplyMonochrome((Color){ 60, 160, 180, 255 }, saturation));
        DrawLine(545, 50, 538, 32, pal.bg_dark);
        DrawLine(555, 50, 562, 32, pal.bg_dark);
        
        // Return Subway Station Portal
        DrawRectangle(20, SCREEN_HEIGHT - 65, 90, 45, Palette_ApplyMonochrome(pal.subway_line, saturation));
        DrawRectangleLines(20, SCREEN_HEIGHT - 65, 90, 45, WHITE);
        FontKorean_DrawText("지하철역", 32, SCREEN_HEIGHT - 50, 16, WHITE);
    }
}

void Render_DrawOpeningEyeBlink(float eye_open_ratio) {
    if (eye_open_ratio >= 1.0f) return;
    if (eye_open_ratio < 0.0f) eye_open_ratio = 0.0f;
    int h = (int)((1.0f - eye_open_ratio) * (SCREEN_HEIGHT / 2.0f));
    DrawRectangle(0, 0, SCREEN_WIDTH, h, BLACK);
    DrawRectangle(0, SCREEN_HEIGHT - h, SCREEN_WIDTH, h, BLACK);
}

void Render_DrawIntroDarkPrologue(float timer, const char* monologue_line) {
    ClearBackground((Color){ 14, 16, 20, 255 });
    
    // Pulsing Music Box Notes
    float pulse = (sinf(timer * 2.5f) + 1.0f) * 0.5f;
    DrawCircle(SCREEN_WIDTH / 2, 90, 30.0f + pulse * 8.0f, (Color){ 70, 90, 120, (unsigned char)(40 + pulse * 60) });
    DrawCircle(SCREEN_WIDTH / 2, 90, 14.0f, (Color){ 240, 230, 200, (unsigned char)(180 + pulse * 75) });
    
    // Title
    FontKorean_DrawText("《 엄마, 어딨어? 》", SCREEN_WIDTH / 2 - 110, 45, 24, (Color){ 245, 215, 140, 255 });
    
    if (monologue_line) {
        FontKorean_DrawText(monologue_line, SCREEN_WIDTH / 2 - 160, 175, 18, (Color){ 230, 230, 235, 255 });
    }
    
    FontKorean_DrawText("[ SPACE / Z : 계속 진행 ]", SCREEN_WIDTH / 2 - 95, 305, 14, (Color){ 140, 145, 160, 255 });
}

void Render_DrawIntroMirrorAwakening(float blur_ratio, float mirror_zoom, const char* monologue_line) {
    GBCPalette pal = Palette_GetDefault();
    ClearBackground((Color){ 18, 22, 28, 255 });
    
    int frameW = (int)(320 * mirror_zoom);
    int frameH = (int)(290 * mirror_zoom);
    int frameX = SCREEN_WIDTH / 2 - frameW / 2;
    int frameY = SCREEN_HEIGHT / 2 - frameH / 2 - 10;
    
    // 1. Subway Screen Door Glass Frame with Metallic Bevels
    DrawRectangle(frameX + 4, frameY + 4, frameW, frameH, (Color){ 8, 10, 14, 180 });
    DrawRectangle(frameX, frameY, frameW, frameH, (Color){ 32, 44, 56, 255 });
    DrawRectangleLines(frameX, frameY, frameW, frameH, pal.bg_dark);
    DrawRectangleLines(frameX + 3, frameY + 3, frameW - 6, frameH - 6, (Color){ 65, 85, 105, 255 });
    
    // Corner Mounting Bolts
    DrawCircle(frameX + 10, frameY + 10, 3, pal.poke_silver);
    DrawCircle(frameX + frameW - 10, frameY + 10, 3, pal.poke_silver);
    DrawCircle(frameX + 10, frameY + frameH - 10, 3, pal.poke_silver);
    DrawCircle(frameX + frameW - 10, frameY + frameH - 10, 3, pal.poke_silver);
    
    // Glass Background (Subway dark tunnel reflection)
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2 - 25;
    DrawRectangle(frameX + 8, frameY + 8, frameW - 16, frameH - 16, (Color){ 24, 34, 46, 255 });
    
    // 2. Beautiful 20s Anime Portrait (Young Mother in 1980s White Dress)
    // Body & White Dress with collar & ribbon
    int bodyY = cy + 30;
    DrawRectangle(cx - 55, bodyY, 110, 95, pal.dress_white);
    DrawRectangleLines(cx - 55, bodyY, 110, 95, pal.bg_dark);
    // V-neck Collar & Golden Pendant Ribbon
    DrawTriangle((Vector2){ cx - 22, bodyY }, (Vector2){ cx + 22, bodyY }, (Vector2){ cx, bodyY + 24 }, pal.skin_tone);
    DrawLine(cx - 22, bodyY, cx, bodyY + 24, pal.bg_dark);
    DrawLine(cx + 22, bodyY, cx, bodyY + 24, pal.bg_dark);
    DrawCircle(cx, bodyY + 24, 4.0f, pal.poke_gold); // Golden brooch
    
    // Slender Neck
    DrawRectangle(cx - 14, cy + 10, 28, 24, pal.skin_tone);
    DrawLine(cx - 14, cy + 10, cx - 14, cy + 32, pal.bg_dark);
    DrawLine(cx + 14, cy + 10, cx + 14, cy + 32, pal.bg_dark);
    
    // Hair Back Volume
    DrawEllipse(cx, cy - 10, 48, 48, pal.bg_dark);
    
    // Face Shape
    DrawEllipse(cx, cy - 2, 34, 38, pal.skin_tone);
    DrawEllipseLines(cx, cy - 2, 34, 38, pal.bg_dark);
    
    // Neat 80s Bob Hair & Bangs
    DrawRectangle(cx - 42, cy - 45, 84, 36, pal.bg_dark);
    DrawTriangle((Vector2){ cx - 44, cy - 40 }, (Vector2){ cx - 34, cy + 15 }, (Vector2){ cx - 26, cy - 10 }, pal.bg_dark); // Left lock
    DrawTriangle((Vector2){ cx + 44, cy - 40 }, (Vector2){ cx + 34, cy + 15 }, (Vector2){ cx + 26, cy - 10 }, pal.bg_dark); // Right lock
    // Layered Bangs
    DrawTriangle((Vector2){ cx - 32, cy - 30 }, (Vector2){ cx - 14, cy - 12 }, (Vector2){ cx - 4, cy - 30 }, pal.bg_dark);
    DrawTriangle((Vector2){ cx - 6, cy - 30 }, (Vector2){ cx + 12, cy - 14 }, (Vector2){ cx + 26, cy - 30 }, pal.bg_dark);
    // Hair Specular Highlight
    DrawLine(cx - 25, cy - 38, cx + 25, cy - 38, (Color){ 80, 75, 90, 255 });
    
    // Expressive Anime Eyes (Almond shape)
    // Left Eye
    DrawRectangle(cx - 24, cy - 8, 12, 11, WHITE);
    DrawRectangleLines(cx - 24, cy - 8, 12, 11, pal.bg_dark);
    DrawRectangle(cx - 21, cy - 6, 7, 9, pal.bg_dark); // Pupil
    DrawRectangle(cx - 20, cy - 6, 3, 3, WHITE);       // Catchlight glint
    DrawLine(cx - 25, cy - 10, cx - 11, cy - 10, pal.bg_dark); // Eyelash
    DrawLine(cx - 23, cy - 14, cx - 11, cy - 14, (Color){ 60, 50, 45, 255 }); // Eyebrow
    
    // Right Eye
    DrawRectangle(cx + 12, cy - 8, 12, 11, WHITE);
    DrawRectangleLines(cx + 12, cy - 8, 12, 11, pal.bg_dark);
    DrawRectangle(cx + 14, cy - 6, 7, 9, pal.bg_dark); // Pupil
    DrawRectangle(cx + 15, cy - 6, 3, 3, WHITE);       // Catchlight glint
    DrawLine(cx + 11, cy - 10, cx + 25, cy - 10, pal.bg_dark); // Eyelash
    DrawLine(cx + 11, cy - 14, cx + 23, cy - 14, (Color){ 60, 50, 45, 255 }); // Eyebrow
    
    // Soft Pink Blush Cheeks
    DrawCircle(cx - 22, cy + 8, 6.0f, (Color){ 245, 140, 150, 180 });
    DrawCircle(cx + 22, cy + 8, 6.0f, (Color){ 245, 140, 150, 180 });
    
    // Dainty Nose & Soft Coral Lips
    DrawRectangle(cx - 1, cy + 3, 2, 2, (Color){ 190, 130, 95, 255 });
    DrawRectangle(cx - 6, cy + 15, 12, 3, (Color){ 225, 95, 95, 255 });
    DrawLine(cx - 6, cy + 15, cx + 6, cy + 15, (Color){ 160, 50, 50, 255 });
    
    // 3. 1st-Person Perspective Hand Touching Mirror Glass
    int handX = cx + 45;
    int handY = cy + 40;
    DrawEllipse(handX, handY, 14, 20, pal.skin_tone);
    DrawEllipseLines(handX, handY, 14, 20, pal.bg_dark);
    // Slender fingers resting on glass
    for (int f = 0; f < 4; f++) {
        DrawRectangle(handX - 10 + f * 6, handY - 18 - f * 2, 5, 16, pal.skin_tone);
        DrawRectangleLines(handX - 10 + f * 6, handY - 18 - f * 2, 5, 16, pal.bg_dark);
        // Fingertip reflection
        DrawCircle(handX - 8 + f * 6, handY - 18 - f * 2, 2.5f, (Color){ 255, 255, 255, 140 });
    }
    
    // 4. Diagonal Specular Glass Reflection Sheen
    DrawLine(frameX + 20, frameY + frameH - 20, frameX + frameW - 20, frameY + 20, (Color){ 255, 255, 255, 80 });
    DrawLine(frameX + 45, frameY + frameH - 20, frameX + frameW - 20, frameY + 45, (Color){ 255, 255, 255, 50 });
    
    // Focus blur scanlines
    if (blur_ratio > 0.05f) {
        for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
            DrawRectangle(0, y, SCREEN_WIDTH, 2, (Color){ 0, 0, 0, (unsigned char)(blur_ratio * 160) });
        }
    }
    
    if (monologue_line) {
        Render_DrawDialogueBox("나 (20대)", monologue_line, 0.3f);
    }
}

void Render_DrawSubwayInteriorTransit(float transit_timer, AreaId dest_area, float saturation) {
    GBCPalette pal = Palette_GetDefault();
    Color interior = Palette_ApplyMonochrome((Color){ 220, 225, 218, 255 }, saturation);
    Color seatCol  = Palette_ApplyMonochrome(pal.poke_blue, saturation);
    ClearBackground(interior);
    
    float shake = sinf(transit_timer * 22.0f) * 1.5f;
    
    // Windows with moving tunnel pillars
    DrawRectangle(50, 45 + (int)shake, SCREEN_WIDTH - 100, 110, (Color){ 18, 22, 28, 255 });
    int pillarOffset = (int)(transit_timer * 450.0f) % 140;
    for (int p = 0; p < SCREEN_WIDTH; p += 140) {
        DrawRectangle(p - pillarOffset + 50, 45 + (int)shake, 18, 110, (Color){ 55, 65, 75, 255 });
    }
    
    // Swinging Hand Straps
    float swing = sinf(transit_timer * 6.5f) * 14.0f;
    for (int h = 100; h < SCREEN_WIDTH - 80; h += 75) {
        DrawLine(h, 15, (int)(h + swing), 75 + (int)shake, pal.bg_dark);
        DrawCircle((int)(h + swing), 82 + (int)shake, 9.0f, pal.fg_highlight);
    }
    
    // Long Seats with passengers
    DrawRectangle(30, 185 + (int)shake, SCREEN_WIDTH - 60, 55, seatCol);
    DrawRectangleLines(30, 185 + (int)shake, SCREEN_WIDTH - 60, 55, pal.bg_dark);
    
    // Subway Destination Announcement
    const char* station_names[4] = { "지하철 환승 승강장", "옛 골목 재래시장", "초등학교 앞 놀이터", "추억의 옛집 단독주택" };
    char announce[128];
    strcpy(announce, "열차가 이동 중입니다. 이번 역은 [ ");
    strcat(announce, station_names[(int)dest_area]);
    strcat(announce, " ] 역입니다.");
    
    Render_DrawDialogueBox("지하철 안내방송", announce, saturation);
}

void Render_DrawStationSelectMenu(int selected_index, AreaId current_area) {
    GBCPalette pal = Palette_GetDefault();
    int menuW = 420;
    int menuH = 200;
    int menuX = SCREEN_WIDTH / 2 - menuW / 2;
    int menuY = SCREEN_HEIGHT / 2 - menuH / 2;
    
    // Pokemon Gold Style Menu Frame
    DrawRectangle(menuX + 4, menuY + 4, menuW, menuH, (Color){ 10, 10, 10, 150 });
    DrawRectangle(menuX, menuY, menuW, menuH, pal.fg_highlight);
    DrawRectangleLines(menuX, menuY, menuW, menuH, pal.bg_dark);
    DrawRectangleLines(menuX + 2, menuY + 2, menuW - 4, menuH - 4, pal.subway_line);
    
    FontKorean_DrawText("지하철 2호선 노선도 (목적지 선택)", menuX + 25, menuY + 15, 16, (Color){ 40, 60, 45, 255 });
    
    const char* stations[4] = {
        "1. 지하철 환승 승강장 (중앙역)",
        "2. 옛 골목 재래시장",
        "3. 초등학교 앞 놀이터",
        "4. 추억의 옛집 단독주택"
    };
    
    for (int i = 0; i < 4; i++) {
        int itemY = menuY + 48 + i * 30;
        if (i == selected_index) {
            DrawRectangle(menuX + 18, itemY - 2, menuW - 36, 26, pal.subway_line);
            FontKorean_DrawText(stations[i], menuX + 38, itemY + 4, 15, WHITE);
            FontKorean_DrawText("▶", menuX + 24, itemY + 4, 14, WHITE);
        } else {
            FontKorean_DrawText(stations[i], menuX + 38, itemY + 4, 15, pal.bg_dark);
        }
        if ((int)current_area == i) {
            FontKorean_DrawText("(현재역)", menuX + 310, itemY + 4, 14, pal.poke_red);
        }
    }
    
    FontKorean_DrawText("[ W/S : 노선 선택 | SPACE : 탑승 ]", menuX + 75, menuY + menuH - 24, 13, (Color){ 100, 105, 115, 255 });
}

void Render_DrawTutorialBanner(const char* prompt, int step, int total_steps) {
    GBCPalette pal = Palette_GetDefault();
    int bannerW = 440;
    int bannerH = 34;
    int bannerX = 20;
    int bannerY = 15;
    
    // Pokemon Gold Style Quest Banner
    DrawRectangle(bannerX + 3, bannerY + 3, bannerW, bannerH, (Color){ 10, 10, 10, 130 });
    DrawRectangle(bannerX, bannerY, bannerW, bannerH, (Color){ 250, 245, 195, 255 });
    DrawRectangleLines(bannerX, bannerY, bannerW, bannerH, pal.bg_dark);
    DrawRectangleLines(bannerX + 2, bannerY + 2, bannerW - 4, bannerH - 4, pal.poke_gold);
    
    char badge[32];
    badge[0] = '[';
    badge[1] = '0' + step;
    badge[2] = '/';
    badge[3] = '0' + total_steps;
    badge[4] = ']';
    badge[5] = '\0';
    
    FontKorean_DrawText("튜토리얼", bannerX + 12, bannerY + 8, 15, pal.poke_red);
    FontKorean_DrawText(badge, bannerX + 78, bannerY + 8, 14, pal.bg_mid);
    if (prompt) {
        FontKorean_DrawText(prompt, bannerX + 120, bannerY + 8, 14, pal.bg_dark);
    }
}

void Render_DrawDialogueBox(const char* speaker, const char* text, float saturation) {
    if (!text || text[0] == '\0') return;
    GBCPalette pal = Palette_GetDefault();
    Color bg = Palette_ApplyMonochrome(pal.fg_highlight, saturation);
    Color border = pal.bg_dark;
    
    int box_x = 30;
    int box_y = SCREEN_HEIGHT - 105;
    int box_w = SCREEN_WIDTH - 60;
    int box_h = 88;
    
    // Pokemon Gold Signature Dialogue Frame with beveled insets
    DrawRectangle(box_x + 3, box_y + 3, box_w, box_h, (Color){ 10, 10, 10, 130 });
    DrawRectangle(box_x, box_y, box_w, box_h, bg);
    DrawRectangleLines(box_x, box_y, box_w, box_h, border);
    DrawRectangleLines(box_x + 2, box_y + 2, box_w - 4, box_h - 4, (Color){ 180, 185, 175, 255 });
    
    // Corner accent dots
    DrawRectangle(box_x + 4, box_y + 4, 3, 3, pal.poke_gold);
    DrawRectangle(box_x + box_w - 7, box_y + 4, 3, 3, pal.poke_gold);
    DrawRectangle(box_x + 4, box_y + box_h - 7, 3, 3, pal.poke_gold);
    DrawRectangle(box_x + box_w - 7, box_y + box_h - 7, 3, 3, pal.poke_gold);
    
    if (speaker && speaker[0] != '\0') {
        DrawRectangle(box_x + 12, box_y - 14, 130, 20, border);
        DrawRectangle(box_x + 14, box_y - 12, 126, 16, pal.poke_blue);
        FontKorean_DrawText(speaker, box_x + 18, box_y - 12, 14, WHITE);
    }
    FontKorean_DrawText(text, box_x + 18, box_y + 16, 16, border);
    
    // Pokemon classic blinking down arrow indicator
    if (((int)(GetTime() * 3.0f)) % 2 == 0) {
        FontKorean_DrawText("▼", box_x + box_w - 24, box_y + box_h - 22, 14, pal.poke_red);
    }
}

void Render_DrawColorBloom(float x, float y, float radius) {
    if (radius <= 0.0f) return;
    for (int r = (int)radius; r > 0; r -= 14) {
        Color ring = (Color){ 245, 230, 120, (unsigned char)((1.0f - (float)r / (radius + 20.0f)) * 150) };
        DrawCircleLines((int)x, (int)y, (float)r, ring);
    }
}
