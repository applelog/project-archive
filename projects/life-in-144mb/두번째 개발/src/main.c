#include <raylib.h>
#include "types.h"
#include "audio_synth.h"
#include "font_korean.h"
#include "life_engine.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "엄마, 어딨어? (Where is Mom?) - 1.44MB");
    SetTargetFPS(TARGET_FPS);
    
    AudioSynth_Init();
    FontKorean_Init();
    
    LifeEngine engine;
    LifeEngine_Init(&engine);
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        
        LifeEngine_Update(&engine, dt);
        
        BeginDrawing();
        LifeEngine_Render(&engine);
        EndDrawing();
    }
    
    AudioSynth_Close();
    CloseWindow();
    return 0;
}
