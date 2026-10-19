#include "life_engine.h"
#include "render_world.h"
#include "audio_synth.h"
#include "font_korean.h"
#include <math.h>
#include <stdio.h>

void LifeEngine_Init(LifeEngine* engine) {
    engine->state = STATE_INTRO_DARK_PROLOGUE;
    engine->intro_timer = 0.0f;
    engine->prologue_line_index = 0;
    engine->eye_blink_ratio = 0.0f;
    engine->focus_blur_ratio = 1.0f;
    engine->mirror_zoom = 1.0f;
    engine->ending_timer = 0.0f;
    engine->tut_move_dist = 0.0f;
    engine->selected_station_index = 1;
    engine->destination_area = AREA_MARKET_ALLEY;
    engine->transit_timer = 0.0f;
    
    // Player Setup
    engine->player.x = 240.0f;
    engine->player.y = 200.0f;
    engine->player.vx = 0.0f;
    engine->player.vy = 0.0f;
    engine->player.dir = DIR_DOWN;
    engine->player.anim_frame = 0;
    engine->player.anim_timer = 0.0f;
    engine->player.is_moving = false;
    engine->player.memory_clarity = 0.15f;
    engine->player.collected_shards = 0;
    engine->player.wandering_time = 0.0f;
    engine->player.current_area = AREA_SUBWAY_PLATFORM;
    
    // Train Setup
    engine->train.x = -600.0f;
    engine->train.target_x = 30.0f;
    engine->train.speed = 220.0f;
    engine->train.is_stopped = false;
    engine->train.doors_open = false;
    engine->train.door_open_ratio = 0.0f;
    engine->train.stop_timer = 0.0f;
    
    // Camera
    engine->camera.target = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    engine->camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    engine->camera.rotation = 0.0f;
    engine->camera.zoom = 1.0f;
    
    Scenario_Init(&engine->scenario);
    AudioSynth_SetBGM(BGM_THEME_INTRO);
}

void LifeEngine_Update(LifeEngine* engine, float dt) {
    AudioSynth_Update(dt);
    
    // -------------------------------------------------------------
    // PHASE 1: DARK PROLOGUE MONOLOGUE
    // -------------------------------------------------------------
    if (engine->state == STATE_INTRO_DARK_PROLOGUE) {
        engine->intro_timer += dt;
        
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || engine->intro_timer > 3.5f) {
            engine->intro_timer = 0.0f;
            engine->prologue_line_index++;
            AudioSynth_PlaySFX(SFX_MEMORY_SHIMMER);
            if (engine->prologue_line_index >= 3) {
                engine->state = STATE_INTRO_EYE_AWAKENING;
                AudioSynth_PlaySFX(SFX_HEARTBEAT);
            }
        }
    }
    // -------------------------------------------------------------
    // PHASE 2: 1ST-PERSON EYE AWAKENING & FOCUS SHIFT
    // -------------------------------------------------------------
    else if (engine->state == STATE_INTRO_EYE_AWAKENING) {
        engine->intro_timer += dt;
        
        // Eyelid flutter effect
        float blink_cycle = sinf(engine->intro_timer * 4.0f);
        engine->eye_blink_ratio = (engine->intro_timer / 4.0f) + 0.25f * blink_cycle;
        engine->focus_blur_ratio = 1.0f - (engine->intro_timer / 4.0f);
        if (engine->focus_blur_ratio < 0.0f) engine->focus_blur_ratio = 0.0f;
        
        if (engine->intro_timer >= 4.0f || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z)) {
            engine->eye_blink_ratio = 1.0f;
            engine->focus_blur_ratio = 0.0f;
            engine->intro_timer = 0.0f;
            engine->state = STATE_INTRO_MIRROR_TALK;
            AudioSynth_PlaySFX(SFX_SUBWAY_CHIME);
        }
    }
    // -------------------------------------------------------------
    // PHASE 3: MIRROR REFLECTION MONOLOGUE
    // -------------------------------------------------------------
    else if (engine->state == STATE_INTRO_MIRROR_TALK) {
        engine->intro_timer += dt;
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || engine->intro_timer > 4.0f) {
            engine->intro_timer = 0.0f;
            engine->state = STATE_INTRO_ZOOM_OUT;
            AudioSynth_PlaySFX(SFX_TRAIN_RAIL_LOOP);
        }
    }
    // -------------------------------------------------------------
    // PHASE 4: TRAIN ROARS IN & 3D ZOOM OUT
    // -------------------------------------------------------------
    else if (engine->state == STATE_INTRO_ZOOM_OUT) {
        engine->intro_timer += dt;
        engine->mirror_zoom -= dt * 0.75f;
        
        // Train approaches
        if (engine->train.x < 30.0f) {
            engine->train.x += engine->train.speed * dt;
        } else {
            engine->train.is_stopped = true;
            if (engine->train.door_open_ratio < 1.0f) {
                engine->train.door_open_ratio += dt * 1.5f;
                if (engine->train.door_open_ratio >= 1.0f) {
                    engine->train.door_open_ratio = 1.0f;
                    AudioSynth_PlaySFX(SFX_DOOR_OPEN);
                }
            }
        }
        
        if (engine->mirror_zoom <= 0.0f) {
            engine->mirror_zoom = 0.0f;
            engine->state = STATE_TUTORIAL_MOVE;
            AudioSynth_SetBGM(BGM_THEME_SEARCHING);
        }
    }
    // -------------------------------------------------------------
    // PHASE 5: INTERACTIVE TUTORIAL STEPS
    // -------------------------------------------------------------
    else if (engine->state == STATE_TUTORIAL_MOVE || 
             engine->state == STATE_TUTORIAL_TALK || 
             engine->state == STATE_TUTORIAL_SHARD || 
             engine->state == STATE_TUTORIAL_BOARD || 
             engine->state == STATE_GAMEPLAY) 
    {
        // 1. Player Movement
        float speed = 100.0f;
        engine->player.is_moving = false;
        
        if (!engine->scenario.dialogue_active) {
            float old_x = engine->player.x;
            float old_y = engine->player.y;
            
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
                engine->player.x -= speed * dt;
                engine->player.dir = DIR_LEFT;
                engine->player.is_moving = true;
            } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
                engine->player.x += speed * dt;
                engine->player.dir = DIR_RIGHT;
                engine->player.is_moving = true;
            }
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
                engine->player.y -= speed * dt;
                engine->player.dir = DIR_UP;
                engine->player.is_moving = true;
            } else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
                engine->player.y += speed * dt;
                engine->player.dir = DIR_DOWN;
                engine->player.is_moving = true;
            }
            
            // Screen Clamping & Dynamic Train Door Entry
            if (engine->player.x < 15.0f) engine->player.x = 15.0f;
            if (engine->player.x > SCREEN_WIDTH - 35.0f) engine->player.x = SCREEN_WIDTH - 35.0f;
            if (engine->player.y > SCREEN_HEIGHT - 35.0f) engine->player.y = SCREEN_HEIGHT - 35.0f;
            
            if (engine->player.current_area == AREA_SUBWAY_PLATFORM) {
                // If in front of open train doors, allow stepping inside up to y = 115
                bool near_doors = (engine->train.is_stopped && engine->train.door_open_ratio > 0.4f && 
                                  ((engine->player.x >= 90.0f && engine->player.x <= 210.0f) || 
                                   (engine->player.x >= 360.0f && engine->player.x <= 480.0f)));
                float min_y = near_doors ? 115.0f : 148.0f;
                if (engine->player.y < min_y) engine->player.y = min_y;
            } else {
                if (engine->player.y < 70.0f) engine->player.y = 70.0f;
            }
            
            float dist = sqrtf((engine->player.x - old_x)*(engine->player.x - old_x) + (engine->player.y - old_y)*(engine->player.y - old_y));
            engine->tut_move_dist += dist;
        }
        
        if (engine->player.is_moving) {
            engine->player.anim_timer += dt * 6.0f;
            engine->player.anim_frame = (int)engine->player.anim_timer % 4;
        }
        
        // 2. Train Idle/Doors in Subway Platform
        if (engine->player.current_area == AREA_SUBWAY_PLATFORM) {
            if (engine->train.x < 30.0f) {
                engine->train.x += engine->train.speed * dt;
            } else {
                engine->train.is_stopped = true;
                if (engine->train.door_open_ratio < 1.0f) {
                    engine->train.door_open_ratio += dt * 1.5f;
                    if (engine->train.door_open_ratio > 1.0f) engine->train.door_open_ratio = 1.0f;
                }
            }
            
            // Check Subway Door Boarding zone (stepping in or pressing SPACE / UP)
            bool at_door = (engine->train.is_stopped && engine->train.door_open_ratio > 0.4f &&
                           ((engine->player.x >= 85.0f && engine->player.x <= 215.0f) ||
                            (engine->player.x >= 355.0f && engine->player.x <= 485.0f)));
            if (at_door) {
                if (engine->player.y <= 135.0f || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    engine->state = STATE_SUBWAY_MAP_SELECT;
                    AudioSynth_PlaySFX(SFX_SUBWAY_CHIME);
                }
            }
        } else {
            // Check Return Subway Portal (bottom left at x: 20..110, y: SCREEN_HEIGHT - 75..SCREEN_HEIGHT)
            if (engine->player.x <= 110.0f && engine->player.y >= SCREEN_HEIGHT - 75.0f) {
                if (engine->player.x <= 60.0f || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z)) {
                    engine->state = STATE_SUBWAY_MAP_SELECT;
                    AudioSynth_PlaySFX(SFX_SUBWAY_CHIME);
                }
            }
        }
        
        // 3. Interactions (NPC & Shards)
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z)) {
            Scenario_TryInteract(&engine->scenario, &engine->player);
        }
        
        // 4. Progress Tutorial Steps
        if (engine->state == STATE_TUTORIAL_MOVE && engine->tut_move_dist > 60.0f) {
            engine->state = STATE_TUTORIAL_TALK;
        } else if (engine->state == STATE_TUTORIAL_TALK && engine->scenario.tut_talked) {
            engine->state = STATE_TUTORIAL_SHARD;
        } else if (engine->state == STATE_TUTORIAL_SHARD && engine->scenario.tut_shard_collected) {
            engine->state = STATE_TUTORIAL_BOARD;
        }
        
        Scenario_Update(&engine->scenario, &engine->player, dt);
        
        // 5. Evaluate Multi-Endings
        GameState nextState = Scenario_EvaluateEnding(&engine->scenario, &engine->player);
        if (nextState != STATE_GAMEPLAY && engine->state == STATE_GAMEPLAY) {
            engine->state = nextState;
            if (nextState == STATE_ENDING_A_TRUE) AudioSynth_SetBGM(BGM_THEME_TRUE_ENDING);
            else if (nextState == STATE_ENDING_B_FAMILY) AudioSynth_SetBGM(BGM_THEME_SEARCHING);
            else AudioSynth_SetBGM(BGM_THEME_HOSPITAL);
        }
    }
    // -------------------------------------------------------------
    // PHASE 6: SUBWAY DESTINATION SELECTION
    // -------------------------------------------------------------
    else if (engine->state == STATE_SUBWAY_MAP_SELECT) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            engine->selected_station_index--;
            if (engine->selected_station_index < 0) engine->selected_station_index = 3;
            AudioSynth_PlaySFX(SFX_MEMORY_SHIMMER);
        } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            engine->selected_station_index++;
            if (engine->selected_station_index > 3) engine->selected_station_index = 0;
            AudioSynth_PlaySFX(SFX_MEMORY_SHIMMER);
        }
        
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER)) {
            engine->destination_area = (AreaId)engine->selected_station_index;
            engine->state = STATE_SUBWAY_RIDING_CUTSCENE;
            engine->transit_timer = 0.0f;
            AudioSynth_PlaySFX(SFX_DOOR_CLOSE);
            AudioSynth_PlaySFX(SFX_TRAIN_RAIL_LOOP);
        } else if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_X)) {
            engine->state = (engine->state == STATE_TUTORIAL_BOARD) ? STATE_TUTORIAL_BOARD : STATE_GAMEPLAY;
        }
    }
    // -------------------------------------------------------------
    // PHASE 7: SUBWAY RIDING TRANSIT CUTSCENE
    // -------------------------------------------------------------
    else if (engine->state == STATE_SUBWAY_RIDING_CUTSCENE) {
        engine->transit_timer += dt;
        
        if (engine->transit_timer > 3.2f) {
            // Arrival at destination station!
            engine->player.current_area = engine->destination_area;
            engine->player.x = 240.0f;
            engine->player.y = 200.0f;
            engine->player.dir = DIR_DOWN;
            engine->state = STATE_GAMEPLAY;
            AudioSynth_PlaySFX(SFX_DOOR_OPEN);
            AudioSynth_PlaySFX(SFX_SUBWAY_CHIME);
        }
    }
    // -------------------------------------------------------------
    // PHASE 8: MULTI-ENDINGS
    // -------------------------------------------------------------
    else {
        engine->ending_timer += dt;
        if (IsKeyPressed(KEY_R) && engine->ending_timer > 2.5f) {
            LifeEngine_Init(engine);
        }
    }
}

void LifeEngine_Render(const LifeEngine* engine) {
    float sat = engine->player.memory_clarity;
    
    // 1. Dark Prologue
    if (engine->state == STATE_INTRO_DARK_PROLOGUE) {
        const char* monologue_lines[3] = {
            "엄마... 어디 계신 거예요?",
            "저를 두고 어디로 가신 건가요...",
            "제가 꼭... 엄마를 찾으러 갈게요."
        };
        Render_DrawIntroDarkPrologue(engine->intro_timer, monologue_lines[engine->prologue_line_index]);
    }
    // 2. 1st-Person Awakening
    else if (engine->state == STATE_INTRO_EYE_AWAKENING) {
        Render_DrawIntroMirrorAwakening(engine->focus_blur_ratio, 1.0f, "눈앞이 흐릿하다... 여긴 어디지?");
        Render_DrawOpeningEyeBlink(engine->eye_blink_ratio);
    }
    // 3. Mirror Reflection Talk
    else if (engine->state == STATE_INTRO_MIRROR_TALK) {
        Render_DrawIntroMirrorAwakening(0.0f, 1.0f, "유리에 비친 내 모습... 엄마가 사주신 하얀 원피스 그대로야.");
    }
    // 4. Zoom out / Gameplay / Tutorial
    else if (engine->state == STATE_INTRO_ZOOM_OUT || 
             engine->state == STATE_TUTORIAL_MOVE || 
             engine->state == STATE_TUTORIAL_TALK || 
             engine->state == STATE_TUTORIAL_SHARD || 
             engine->state == STATE_TUTORIAL_BOARD || 
             engine->state == STATE_GAMEPLAY || 
             engine->state == STATE_SUBWAY_MAP_SELECT) 
    {
        // 1. Draw Area Background Map
        Render_DrawAreaMap(engine->player.current_area, sat);
        
        // 2. Draw Moving Subway Train if on Platform
        if (engine->player.current_area == AREA_SUBWAY_PLATFORM) {
            Render_DrawMovingSubway(&engine->train, sat);
        }
        
        // 3. Draw Uncollected Shards in Current Area
        for (int i = 0; i < engine->scenario.shard_count; i++) {
            if (!engine->scenario.shards[i].is_collected && engine->scenario.shards[i].area == engine->player.current_area) {
                float sx = engine->scenario.shards[i].x;
                float sy = engine->scenario.shards[i].y;
                Render_DrawFake3DShadow(sx, sy + 4, 10.0f, 4.0f);
                DrawCircle((int)sx, (int)sy, 6.0f, (Color){ 245, 220, 80, 255 });
                DrawCircleLines((int)sx, (int)sy, 8.0f, WHITE);
            }
        }
        
        // 4. Draw NPCs in Current Area
        for (int i = 0; i < engine->scenario.npc_count; i++) {
            if (engine->scenario.npcs[i].area == engine->player.current_area) {
                Render_DrawCharacter(engine->scenario.npcs[i].x, engine->scenario.npcs[i].y, 
                                     engine->scenario.npcs[i].dir, 0, false, sat);
            }
        }
        
        // 5. Draw Player Character
        Render_DrawCharacter(engine->player.x, engine->player.y, engine->player.dir, 
                             engine->player.anim_frame, true, sat);
                             
        // Floating prompt when standing near Subway Doors or Return Portal
        if (engine->player.current_area == AREA_SUBWAY_PLATFORM) {
            bool near_door = (engine->train.is_stopped && engine->train.door_open_ratio > 0.4f &&
                             ((engine->player.x >= 85.0f && engine->player.x <= 215.0f) ||
                              (engine->player.x >= 355.0f && engine->player.x <= 485.0f)) &&
                             engine->player.y <= 170.0f);
            if (near_door && !engine->scenario.dialogue_active) {
                DrawRectangle((int)engine->player.x - 45, (int)engine->player.y - 30, 115, 20, (Color){ 20, 30, 20, 220 });
                DrawRectangleLines((int)engine->player.x - 45, (int)engine->player.y - 30, 115, 20, (Color){ 46, 145, 80, 255 });
                FontKorean_DrawText("SPACE : 탑승", (int)engine->player.x - 38, (int)engine->player.y - 28, 13, WHITE);
            }
        } else {
            if (engine->player.x <= 110.0f && engine->player.y >= SCREEN_HEIGHT - 75.0f && !engine->scenario.dialogue_active) {
                DrawRectangle(20, SCREEN_HEIGHT - 90, 120, 20, (Color){ 20, 30, 20, 220 });
                DrawRectangleLines(20, SCREEN_HEIGHT - 90, 120, 20, (Color){ 46, 145, 80, 255 });
                FontKorean_DrawText("SPACE : 지하철", 26, SCREEN_HEIGHT - 88, 13, WHITE);
            }
        }
                             
        // 6. Draw Color Bloom Wave if active
        if (engine->scenario.bloom_timer > 0.0f) {
            float bloomRadius = (2.5f - engine->scenario.bloom_timer) * 120.0f;
            Render_DrawColorBloom(engine->scenario.bloom_x, engine->scenario.bloom_y, bloomRadius);
        }
        
        // 7. Active Dialogue Box
        if (engine->scenario.dialogue_active && engine->scenario.current_dialogue_npc >= 0) {
            int npc_idx = engine->scenario.current_dialogue_npc;
            const NPC* npc = &engine->scenario.npcs[npc_idx];
            const char* speech = (engine->player.collected_shards >= 2) ? npc->dialogue_hesitant : npc->dialogue_initial;
            Render_DrawDialogueBox(npc->name, speech, sat);
        }
        
        // 8. Tutorial Banner Overlays
        if (engine->state == STATE_TUTORIAL_MOVE) {
            Render_DrawTutorialBanner("[ 방향키 / WASD 키로 움직여 보세요 ]", 1, 4);
        } else if (engine->state == STATE_TUTORIAL_TALK) {
            Render_DrawTutorialBanner("[ 역무원에게 다가가 SPACE / Z 키로 대화하세요 ]", 2, 4);
        } else if (engine->state == STATE_TUTORIAL_SHARD) {
            Render_DrawTutorialBanner("[ 벤치 위의 반짝이는 승차권을 조사하세요 ]", 3, 4);
        } else if (engine->state == STATE_TUTORIAL_BOARD) {
            Render_DrawTutorialBanner("[ 열차 문 안으로 걸어 들어가 탑승하세요 ]", 4, 4);
        }
        
        // 9. HUD (Clues & Alert) in dedicated Top-Right Box
        GBCPalette pal = Palette_GetDefault();
        int hudX = SCREEN_WIDTH - 155;
        int hudY = 15;
        int hudW = 135;
        int hudH = 34;
        DrawRectangle(hudX + 3, hudY + 3, hudW, hudH, (Color){ 10, 10, 10, 130 });
        DrawRectangle(hudX, hudY, hudW, hudH, (Color){ 245, 240, 230, 255 });
        DrawRectangleLines(hudX, hudY, hudW, hudH, pal.bg_dark);
        
        FontKorean_DrawText("기억", hudX + 10, hudY + 8, 14, (Color){ 160, 80, 40, 255 });
        for (int i = 0; i < 4; i++) {
            Color shardCol = (i < engine->player.collected_shards) ? (Color){ 245, 205, 50, 255 } : (Color){ 80, 85, 90, 255 };
            DrawCircle(hudX + 52 + i * 20, hudY + 17, 7, shardCol);
            DrawCircleLines(hudX + 52 + i * 20, hudY + 17, 7, pal.bg_dark);
        }
        
        // Station Map Modal if active
        if (engine->state == STATE_SUBWAY_MAP_SELECT) {
            Render_DrawStationSelectMenu(engine->selected_station_index, engine->player.current_area);
        }
    }
    // 5. Subway Riding Transit View
    else if (engine->state == STATE_SUBWAY_RIDING_CUTSCENE) {
        Render_DrawSubwayInteriorTransit(engine->transit_timer, engine->destination_area, sat);
    }
    // 6. Ending A (True Realization)
    else if (engine->state == STATE_ENDING_A_TRUE) {
        ClearBackground((Color){ 245, 240, 230, 255 });
        FontKorean_DrawText("《 진실의 거울 》", SCREEN_WIDTH / 2 - 80, 60, 2, (Color){ 30, 30, 30, 255 });
        FontKorean_DrawText("거울 속에 비친 주름진 손과 백발의 어머니.", SCREEN_WIDTH / 2 - 160, 120, 1, (Color){ 60, 50, 40, 255 });
        FontKorean_DrawText("\"아... 내가 우리 아이들의 엄마였구나.\"", SCREEN_WIDTH / 2 - 150, 160, 1, (Color){ 200, 60, 40, 255 });
        FontKorean_DrawText("기억을 되찾은 어머니의 미소와 함께 가족들이 눈물을 흘리며 달려옵니다.", SCREEN_WIDTH / 2 - 220, 200, 1, (Color){ 80, 70, 60, 255 });
        FontKorean_DrawText("[ R : 다시 시작 ]", SCREEN_WIDTH / 2 - 50, 310, 1, (Color){ 100, 100, 100, 255 });
    }
    // 7. Ending B (Family Guide)
    else if (engine->state == STATE_ENDING_B_FAMILY) {
        ClearBackground((Color){ 20, 25, 30, 255 });
        FontKorean_DrawText("《 가족의 품으로 》", SCREEN_WIDTH / 2 - 80, 60, 2, (Color){ 230, 210, 160, 255 });
        FontKorean_DrawText("어두워진 밤길, 멀리서 숨을 헐떡이며 달려온 딸의 목소리.", SCREEN_WIDTH / 2 - 190, 120, 1, (Color){ 180, 180, 190, 255 });
        FontKorean_DrawText("\"엄마! 한참 찾았잖아... 집에 가자.\"", SCREEN_WIDTH / 2 - 140, 160, 1, (Color){ 240, 200, 100, 255 });
        FontKorean_DrawText("따스한 손에 이끌려 집으로 돌아갑니다.", SCREEN_WIDTH / 2 - 130, 200, 1, (Color){ 150, 150, 160, 255 });
        FontKorean_DrawText("[ R : 다시 시작 ]", SCREEN_WIDTH / 2 - 50, 310, 1, (Color){ 100, 100, 100, 255 });
    }
    // 8. Ending C (Hospital)
    else if (engine->state == STATE_ENDING_C_HOSPITAL) {
        ClearBackground((Color){ 10, 10, 10, 255 });
        FontKorean_DrawText("《 기억의 암전 》", SCREEN_WIDTH / 2 - 70, 60, 2, (Color){ 180, 40, 40, 255 });
        FontKorean_DrawText("눈앞이 점점 어두워지며 모든 소리가 아득해집니다.", SCREEN_WIDTH / 2 - 170, 120, 1, (Color){ 160, 160, 160, 255 });
        FontKorean_DrawText("눈을 떴을 때, 차가운 병실 침대와 창문만이 덩그러니 보입니다.", SCREEN_WIDTH / 2 - 200, 160, 1, (Color){ 140, 140, 140, 255 });
        FontKorean_DrawText("[ R : 다시 시작 ]", SCREEN_WIDTH / 2 - 50, 310, 1, (Color){ 100, 100, 100, 255 });
    }
}
