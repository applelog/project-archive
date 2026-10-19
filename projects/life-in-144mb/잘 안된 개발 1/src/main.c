#include <raylib.h>
#include <stdio.h>
#include "types.h"
#include "audio_synth.h"
#include "palette.h"
#include "render_world.h"
#include "life_engine.h"
#include "reminiscence.h"

typedef enum {
    STATE_TITLE = 0,
    STATE_GAMEPLAY,
    STATE_REMINISCENCE,
    STATE_ENDING
} GameState;

int main(void) {
    // 640x360 16:9 Retro Resolution
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "The Path of Life - 1.44MB");
    SetTargetFPS(TARGET_FPS);

    // Initialize Subsystems
    AudioSynth_Init();
    RenderWorld_Init();

    LifeEngine life;
    LifeEngine_Init(&life);

    ReminiscenceEngine reminiscence;
    Reminiscence_Init(&reminiscence);

    GameState state = STATE_TITLE;
    float title_timer = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f; // Clamp delta time

        AudioSynth_Update(dt);

        // 1. TITLE SCREEN
        if (state == STATE_TITLE) {
            title_timer += dt;
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER)) {
                state = STATE_GAMEPLAY;
                AudioSynth_PlaySFX(SFX_CHOICE_SELECT);
                AudioSynth_PlayBGM(ACT_0_INFANT);
            }
        }
        // 2. GAMEPLAY (Acts 0 ~ 4)
        else if (state == STATE_GAMEPLAY) {
            LifeEngine_Update(&life, dt);

            // If player reaches Act 5 (Death & Reminiscence)
            if (life.player.current_act >= ACT_5_DEATH_REPLAY) {
                state = STATE_REMINISCENCE;
                Reminiscence_Start(&reminiscence);
            }
        }
        // 3. SCENIC REMINISCENCE (Act 5 Full-Color Flashback)
        else if (state == STATE_REMINISCENCE) {
            const MemoryLog* log = Scenario_GetMemoryLog(&life.scenario);
            Reminiscence_Update(&reminiscence, log, dt);

            if (reminiscence.is_finished) {
                if (IsKeyPressed(KEY_R)) {
                    // Reset game
                    LifeEngine_Init(&life);
                    Reminiscence_Init(&reminiscence);
                    state = STATE_TITLE;
                }
            }
        }

        // RENDERING
        BeginDrawing();
        ClearBackground((Color){ 17, 17, 14, 255 });

        if (state == STATE_TITLE) {
            // Floppy Disk Aesthetic Title Screen
            ClearBackground((Color){ 20, 21, 18, 255 });

            // Floppy disk frame
            DrawRectangle(SCREEN_WIDTH / 2 - 140, 40, 280, 240, (Color){ 41, 43, 40, 255 });
            DrawRectangleLines(SCREEN_WIDTH / 2 - 140, 40, 280, 240, (Color){ 65, 68, 62, 255 });
            // Floppy label
            DrawRectangle(SCREEN_WIDTH / 2 - 120, 55, 240, 110, (Color){ 236, 229, 213, 255 });
            DrawRectangle(SCREEN_WIDTH / 2 - 120, 55, 240, 12, (Color){ 240, 107, 50, 255 });

            RenderWorld_DrawPixelText("1,474,560 BYTES", SCREEN_WIDTH / 2 - 50, 72, 12, (Color){ 30, 30, 30, 255 });
            RenderWorld_DrawPixelText("《 인생의 길 》", SCREEN_WIDTH / 2 - 65, 90, 20, (Color){ 20, 20, 20, 255 });
            RenderWorld_DrawPixelText("The Path of Life", SCREEN_WIDTH / 2 - 45, 118, 12, (Color){ 80, 80, 80, 255 });
            RenderWorld_DrawPixelText("한 사람의 탄생부터 사랑, 그리고 영원까지", SCREEN_WIDTH / 2 - 110, 138, 11, (Color){ 110, 100, 90, 255 });

            // Shutter
            DrawRectangle(SCREEN_WIDTH / 2 - 90, 200, 180, 65, (Color){ 184, 181, 169, 255 });
            DrawRectangle(SCREEN_WIDTH / 2 - 40, 210, 80, 45, (Color){ 23, 24, 20, 255 });

            // Start Prompt (Blinking)
            if (((int)(title_timer * 2.0f)) % 2 == 0) {
                RenderWorld_DrawPixelText("[ SPACE / Z 키를 눌러 삶을 시작하기 ]", SCREEN_WIDTH / 2 - 140, 305, 14, (Color){ 235, 166, 56, 255 });
            }
            RenderWorld_DrawPixelText("2P GAME ARCADE 1.44MB CONTEST", SCREEN_WIDTH / 2 - 110, 335, 11, (Color){ 84, 201, 206, 255 });
        }
        else if (state == STATE_GAMEPLAY) {
            LifeEngine_Render(&life);
        }
        else if (state == STATE_REMINISCENCE) {
            const MemoryLog* log = Scenario_GetMemoryLog(&life.scenario);
            Reminiscence_Render(&reminiscence, log);
        }

        EndDrawing();
    }

    AudioSynth_Close();
    CloseWindow();
    return 0;
}
