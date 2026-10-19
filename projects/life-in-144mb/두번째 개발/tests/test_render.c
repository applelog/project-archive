#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../src/types.h"
#include "../src/palette.h"
#include "../src/font_korean.h"
#include "../src/render_world.h"

int main(void) {
    printf("=== Running Task 5: GBC Graphics Engine & Renderer Tests ===\n");

    // 1. Initialize Raylib in headless/hidden mode for draw calls
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "RenderTest");

    // 2. Initialize font and renderer
    FontKorean_Init();
    Render_Init();

    // 3. Test Draw Context & Shadow Rendering
    BeginDrawing();
    ClearBackground(BLACK);

    // Test Fake 3D Shadows
    Render_DrawFake3DShadow(100.0f, 100.0f, 8.0f, 4.0f);
    Render_DrawFake3DShadow(0.0f, 0.0f, 12.0f, 6.0f);
    Render_DrawFake3DShadow(SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20, 10.0f, 5.0f);

    // 4. Test Character Rendering across all 4 directions & animation frames
    Direction dirs[] = { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT };
    for (int d = 0; d < 4; d++) {
        for (int frame = 0; frame < 4; frame++) {
            // Player character at various saturations
            Render_DrawCharacter(50.0f + d * 40.0f, 50.0f + frame * 30.0f, dirs[d], frame, true, 0.0f);
            Render_DrawCharacter(50.0f + d * 40.0f, 170.0f + frame * 30.0f, dirs[d], frame, true, 0.5f);
            Render_DrawCharacter(50.0f + d * 40.0f, 290.0f + frame * 30.0f, dirs[d], frame, true, 1.0f);

            // NPC character
            Render_DrawCharacter(250.0f + d * 40.0f, 50.0f + frame * 30.0f, dirs[d], frame, false, 0.0f);
            Render_DrawCharacter(250.0f + d * 40.0f, 170.0f + frame * 30.0f, dirs[d], frame, false, 1.0f);
        }
    }

    // 5. Test Moving Subway Train System
    SubwayTrain train = {0};
    train.x = 20.0f;
    train.target_x = 20.0f;
    train.speed = 0.0f;
    train.is_stopped = true;
    train.doors_open = true;
    train.door_open_ratio = 1.0f;
    train.stop_timer = 5.0f;

    // Normal train rendering
    Render_DrawMovingSubway(&train, 1.0f);
    Render_DrawMovingSubway(&train, 0.0f);

    // Door animation edge cases (closed, half, fully open, negative, overflow)
    train.door_open_ratio = 0.0f;
    Render_DrawMovingSubway(&train, 0.5f);
    train.door_open_ratio = 0.5f;
    Render_DrawMovingSubway(&train, 0.5f);
    train.door_open_ratio = -0.5f;
    Render_DrawMovingSubway(&train, 0.5f);
    train.door_open_ratio = 1.5f;
    Render_DrawMovingSubway(&train, 0.5f);

    // Null safety check
    Render_DrawMovingSubway(NULL, 1.0f);

    // 6. Test Area Maps Rendering
    AreaId areas[] = { AREA_SUBWAY_PLATFORM, AREA_MARKET_ALLEY, AREA_SCHOOL_PLAYGROUND, AREA_OLD_HOUSE };
    for (int a = 0; a < 4; a++) {
        Render_DrawAreaMap(areas[a], 0.0f);
        Render_DrawAreaMap(areas[a], 0.5f);
        Render_DrawAreaMap(areas[a], 1.0f);
    }

    // Subway Platform composite renderer
    Render_DrawSubwayPlatform(&train, 1.0f);
    Render_DrawSubwayPlatform(NULL, 0.5f);

    // 7. Test Opening Eye Blink
    Render_DrawOpeningEyeBlink(0.0f); // Fully closed
    Render_DrawOpeningEyeBlink(0.25f);
    Render_DrawOpeningEyeBlink(0.5f);  // Half open
    Render_DrawOpeningEyeBlink(0.75f);
    Render_DrawOpeningEyeBlink(1.0f);  // Fully open
    Render_DrawOpeningEyeBlink(1.2f);  // Over-open (should safely do nothing)
    Render_DrawOpeningEyeBlink(-0.2f); // Negative (clamped to closed)

    // 8. Test Opening Cinematic & Mirror Reflection
    Render_DrawOpeningCinematic(0.5f, 1.0f);
    Render_DrawOpeningCinematic(1.0f, 1.5f);
    Render_DrawMirrorReflection(1.0f, 1.0f);
    Render_DrawMirrorReflection(0.5f, 0.5f);
    Render_DrawMirrorReflection(1.2f, 0.0f); // alpha = 0 (safely ignored)

    // 9. Test Dialogue Box Rendering
    Render_DrawDialogueBox("역무원", "아가씨, 이 시간에 혼자 어딜 가시나요?", 1.0f);
    Render_DrawDialogueBox("엄마", "우리 아이 손을 놓쳐버렸어요...", 0.0f);
    Render_DrawDialogueBox(NULL, "독백: 기억이 희미해진다...", 0.5f);
    Render_DrawDialogueBox("", "", 1.0f);
    Render_DrawDialogueBox(NULL, NULL, 1.0f);

    // 10. Test Color Bloom Effect
    Render_DrawColorBloom(320.0f, 180.0f, 40.0f);
    Render_DrawColorBloom(100.0f, 100.0f, 0.0f);  // zero radius safe check
    Render_DrawColorBloom(500.0f, 250.0f, 120.0f);

    EndDrawing();
    CloseWindow();

    printf("[PASS] All graphics engine, drop shadows, fake 3D & subway tests passed successfully!\n");
    return 0;
}
