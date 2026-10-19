#include "life_engine.h"
#include "render_world.h"
#include "audio_synth.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float s_location_banner_timer = 0.0f;
static char s_location_banner_name[64] = "";

static void TriggerLocationBanner(const char* name) {
    if (name) {
        strncpy(s_location_banner_name, name, sizeof(s_location_banner_name) - 1);
        s_location_banner_timer = 3.0f;
    }
}

static void StartWarp(LifeEngine* engine, MapId target_map, float spawn_x, float spawn_y, Direction dir, const char* loc_name) {
    engine->player.is_warping = true;
    engine->player.warp_fade_timer = 0.0f;
    engine->player.pending_warp_target_map = target_map;
    engine->player.pending_warp_spawn_x = spawn_x;
    engine->player.pending_warp_spawn_y = spawn_y;
    engine->player.pending_warp_spawn_dir = dir;
    AudioSynth_PlaySFX(SFX_STEP);
    TriggerLocationBanner(loc_name);
}

void LifeEngine_Init(LifeEngine* engine) {
    engine->player = (PlayerState){
        .x = 8 * 24.0f,
        .y = 8 * 24.0f,
        .vx = 0.0f,
        .vy = 0.0f,
        .dir = DIR_DOWN,
        .age = 0.0f,
        .progress = 0.0f,
        .is_moving = false,
        .anim_timer = 0.0f,
        .in_dialogue = false,
        .current_act = ACT_0_INFANT,
        .current_map = MAP_MY_HOME, // Start in our home room!
        .color_saturation = 0.0f,
        .color_spark_timer = 0.0f,
        .color_bloom_radius = 0.0f,
        .true_love_awakened = false,
        .is_married = false,
        .married_target = ROMANCE_NONE,
        .career_score = 0,
        .family_score = 0,
        .stair_sensor_light = 0.0f,
        .warp_fade_timer = 0.0f,
        .is_warping = false
    };
    engine->player.spouse_name[0] = '\0';

    engine->camera = (Camera2D){
        .offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
        .target = (Vector2){ engine->player.x, engine->player.y },
        .rotation = 0.0f,
        .zoom = 1.25f // Game Boy feel
    };

    Scenario_Init(&engine->scenario, &engine->player);

    engine->act_timer = 0.0f;
    engine->act_duration = 55.0f;
    engine->total_lived_years = 0.0f;

    // Act 0 Infant Setup
    engine->blink_alpha = 0.8f;
    engine->vision_clarity = 0.2f;
    engine->infant_milestone = 0;
    engine->baby_hand_y = SCREEN_HEIGHT - 30.0f;
    engine->mobile_angle = 0.0f;
    engine->zoom_transition = 1.0f;
    engine->is_game_over = false;

    Palette_Init();
    AudioSynth_PlayBGM(ACT_0_INFANT);
}

void LifeEngine_TransitionToNextAct(LifeEngine* engine) {
    if (engine->player.current_act < ACT_5_DEATH_REPLAY) {
        engine->player.current_act++;
        engine->act_timer = 0.0f;

        if (engine->player.current_act == ACT_1_CHILDHOOD) {
            engine->player.current_map = MAP_MY_HOME;
            engine->player.x = 8 * 24.0f;
            engine->player.y = 8 * 24.0f;
            TriggerLocationBanner("우리 집 안방 (봄)");
        } else if (engine->player.current_act == ACT_2_YOUTH) {
            engine->player.current_map = MAP_OVERWORLD;
            engine->player.x = 34 * 24.0f;
            engine->player.y = 26 * 24.0f; // 대학가 광장
            TriggerLocationBanner("대학 캠퍼스 & 한강 (여름)");
        } else if (engine->player.current_act == ACT_3_ADULTHOOD) {
            engine->player.current_map = MAP_OFFICE_INTERIOR;
            engine->player.x = 9 * 24.0f;
            engine->player.y = 8 * 24.0f; // 회사 사무실
            TriggerLocationBanner("회사 사무실 & 퇴근길 (가을)");
            if (engine->player.is_married && strlen(engine->player.spouse_name) > 0) {
                strncpy(g_stage_npcs[2][0].name, engine->player.spouse_name, sizeof(g_stage_npcs[2][0].name) - 1);
                g_stage_npcs[2][0].sprite_id = (engine->player.married_target == ROMANCE_SUA) ? 3 :
                                               (engine->player.married_target == ROMANCE_JIEUN) ? 4 : 5;
            }
        } else if (engine->player.current_act == ACT_4_ELDERLY) {
            engine->player.current_map = MAP_OVERWORLD;
            engine->player.x = 24 * 24.0f;
            engine->player.y = 18 * 24.0f; // 느티나무 평상 앞
            TriggerLocationBanner("고향 마을 느티나무 평상 (겨울)");
            if (engine->player.is_married && strlen(engine->player.spouse_name) > 0) {
                strncpy(g_stage_npcs[3][1].name, engine->player.spouse_name, sizeof(g_stage_npcs[3][1].name) - 1);
            }
        } else if (engine->player.current_act == ACT_5_DEATH_REPLAY) {
            engine->player.current_map = MAP_OVERWORLD;
            engine->player.x = 28 * 24.0f;
            engine->player.y = 30 * 24.0f;
            engine->player.color_saturation = 1.0f;
            TriggerLocationBanner("기억의 성역 (영원)");
        }

        AudioSynth_PlayBGM((LifeAct)engine->player.current_act);
        AudioSynth_PlaySFX(SFX_MEMORY_DISCOVER);
    }
}

void LifeEngine_Update(LifeEngine* engine, float dt) {
    PlayerState* p = &engine->player;

    if (s_location_banner_timer > 0.0f) {
        s_location_banner_timer -= dt;
    }

    // 1. Screen Warp Fade Handler
    if (p->is_warping) {
        p->warp_fade_timer += dt * 3.5f;
        if (p->warp_fade_timer >= 1.0f) {
            p->current_map = (MapId)p->pending_warp_target_map;
            p->x = p->pending_warp_spawn_x;
            p->y = p->pending_warp_spawn_y;
            p->dir = p->pending_warp_spawn_dir;
            p->is_warping = false;
            p->warp_fade_timer = 0.0f;
            engine->camera.target = (Vector2){ p->x, p->y };
        }
        return;
    }

    // 2. ACT 0 Infant 1st-Person
    if (p->current_act == ACT_0_INFANT) {
        engine->mobile_angle += dt * 0.8f;

        engine->blink_alpha -= dt * 0.9f;
        if (engine->blink_alpha < 0.0f) engine->blink_alpha = 0.0f;

        engine->baby_hand_y = (SCREEN_HEIGHT - 40.0f) + sinf(GetTime() * 2.5f) * 8.0f;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z)) {
            engine->blink_alpha = 1.0f;
            engine->vision_clarity += 0.25f;
            AudioSynth_PlaySFX(SFX_BLINK);

            if (engine->vision_clarity >= 0.7f && engine->infant_milestone == 0) {
                engine->infant_milestone = 1;
                AudioSynth_PlaySFX(SFX_BABY_COO);
            } else if (engine->vision_clarity >= 1.0f && engine->infant_milestone == 1) {
                engine->infant_milestone = 2;
                AudioSynth_PlaySFX(SFX_BABY_COO);
            }
        }

        if (engine->infant_milestone == 2) {
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
                engine->infant_milestone = 3;
                AudioSynth_PlaySFX(SFX_STEP);
            }
        }

        if (engine->infant_milestone == 3) {
            engine->zoom_transition -= dt * 0.8f;
            if (engine->zoom_transition <= 0.0f) {
                engine->zoom_transition = 0.0f;
                LifeEngine_TransitionToNextAct(engine);
            }
        }
        return;
    }

    // 3. Finite Life Clock Progression
    engine->act_timer += dt;
    float time_ratio = engine->act_timer / engine->act_duration;
    if (time_ratio > 1.0f) time_ratio = 1.0f;

    float base_act_age = 7.0f;
    float act_age_span = 11.0f;
    if (p->current_act == ACT_2_YOUTH) {
        base_act_age = 19.0f;
        act_age_span = 11.0f;
    } else if (p->current_act == ACT_3_ADULTHOOD) {
        base_act_age = 31.0f;
        act_age_span = 29.0f;
    } else if (p->current_act == ACT_4_ELDERLY) {
        base_act_age = 61.0f;
        act_age_span = 19.0f;
    }
    p->age = base_act_age + time_ratio * act_age_span;
    engine->total_lived_years = p->age;

    if (engine->act_timer >= engine->act_duration && p->current_act < ACT_5_DEATH_REPLAY) {
        LifeEngine_TransitionToNextAct(engine);
    }

    // 4. Color Bloom & Sparks
    if (p->color_spark_timer > 0.0f) {
        p->color_spark_timer -= dt;
        p->color_bloom_radius += dt * 350.0f;
        p->color_saturation += dt * 0.8f;
        if (p->color_saturation > 1.0f) p->color_saturation = 1.0f;
    } else if (p->true_love_awakened) {
        p->color_saturation = 1.0f;
    }

    // 5. 아파트 계단실 센서등 제어
    if (p->current_map == MAP_APARTMENT_STAIRS) {
        if (p->y <= 12 * 24.0f) {
            p->stair_sensor_light += dt * 3.0f;
            if (p->stair_sensor_light > 1.0f) p->stair_sensor_light = 1.0f;
        } else {
            p->stair_sensor_light -= dt * 1.5f;
            if (p->stair_sensor_light < 0.0f) p->stair_sensor_light = 0.0f;
        }
    }

    // 6. Scenario / Dialogue Update
    if (engine->scenario.active) {
        Scenario_Update(&engine->scenario, p, dt);
        p->is_moving = false;
        return;
    }

    // 7. Movement & Controls
    float speed = 85.0f;
    if (p->current_act == ACT_2_YOUTH) speed = 110.0f;
    else if (p->current_act == ACT_4_ELDERLY) speed = 55.0f;

    p->vx = 0.0f;
    p->vy = 0.0f;
    p->is_moving = false;

    float move_dx = 0.0f;
    float move_dy = 0.0f;

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        move_dx = -speed * dt;
        p->dir = DIR_LEFT;
        p->is_moving = true;
    } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        move_dx = speed * dt;
        p->dir = DIR_RIGHT;
        p->is_moving = true;
    }

    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        move_dy = -speed * dt;
        p->dir = DIR_UP;
        p->is_moving = true;
    } else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        move_dy = speed * dt;
        p->dir = DIR_DOWN;
        p->is_moving = true;
    }

    if (p->is_moving) {
        p->anim_timer += dt;
        float next_x = p->x + move_dx;
        float next_y = p->y + move_dy;

        int map_w, map_h;
        RenderWorld_GetMapDimensions(p->current_map, &map_w, &map_h);

        // Collision Check with Solid Walls & Props
        int tile_x = (int)(next_x / 24.0f);
        int tile_y = (int)(next_y / 24.0f);
        TileType current_t = RenderWorld_GetTile(p->current_map, tile_x, tile_y);

        bool is_solid = false;
        if (current_t == TILE_WALL_BRICK || current_t == TILE_WALL_CONCRETE ||
            current_t == TILE_WALL_WOOD  || current_t == TILE_WALL_INTERIOR ||
            current_t == TILE_TV || current_t == TILE_FRIDGE || current_t == TILE_SINK ||
            current_t == TILE_DOOR_LOCKED || current_t == TILE_VOID) {
            is_solid = true;
        }

        if (!is_solid) {
            p->x = next_x;
            p->y = next_y;
        }

        if (p->x < 16.0f) p->x = 16.0f;
        if (p->x > (map_w * 24.0f - 16.0f)) p->x = map_w * 24.0f - 16.0f;
        if (p->y < 16.0f) p->y = 16.0f;
        if (p->y > (map_h * 24.0f - 16.0f)) p->y = map_h * 24.0f - 16.0f;

        // ★ 포켓몬식 워프/문 진입 검사 (Pokemon Warp Trigger)
        int cur_tx = (int)(p->x / 24.0f);
        int cur_ty = (int)(p->y / 24.0f);
        TileType stand_tile = RenderWorld_GetTile(p->current_map, cur_tx, cur_ty);

        if (p->current_map == MAP_OVERWORLD) {
            if (stand_tile == TILE_DOOR_HOME) {
                StartWarp(engine, MAP_MY_HOME, 8 * 24.0f, 11 * 24.0f, DIR_UP, "우리 집 안방");
            } else if (stand_tile == TILE_DOOR_SHOP) {
                StartWarp(engine, MAP_STATIONERY_STORE, 7 * 24.0f, 9 * 24.0f, DIR_UP, "그린 문방구");
            } else if (stand_tile == TILE_DOOR_SCHOOL) {
                StartWarp(engine, MAP_CLASSROOM, 9 * 24.0f, 11 * 24.0f, DIR_UP, "1학년 2반 교실");
            } else if (stand_tile == TILE_DOOR_POCHA) {
                StartWarp(engine, MAP_POJANGMACHA_INTERIOR, 6 * 24.0f, 7 * 24.0f, DIR_UP, "골목길 포장마차");
            } else if (stand_tile == TILE_DOOR_APT) {
                StartWarp(engine, MAP_APARTMENT_STAIRS, 6 * 24.0f, 23 * 24.0f, DIR_UP, "아파트 계단실 (1F)");
            } else if (stand_tile == TILE_DOOR_OFFICE) {
                StartWarp(engine, MAP_OFFICE_INTERIOR, 9 * 24.0f, 11 * 24.0f, DIR_UP, "회사 사무실");
            }
        } else if (stand_tile == TILE_DOORMAT) {
            // 실내에서 야외로 나가기
            if (p->current_map == MAP_MY_HOME) {
                StartWarp(engine, MAP_OVERWORLD, 8 * 24.0f, 11 * 24.0f, DIR_DOWN, "동네 골목길");
            } else if (p->current_map == MAP_STATIONERY_STORE) {
                StartWarp(engine, MAP_OVERWORLD, 20 * 24.0f, 24 * 24.0f, DIR_DOWN, "문방구 앞 골목");
            } else if (p->current_map == MAP_CLASSROOM) {
                StartWarp(engine, MAP_OVERWORLD, 44 * 24.0f, 12 * 24.0f, DIR_DOWN, "초등학교 운동장");
            } else if (p->current_map == MAP_POJANGMACHA_INTERIOR) {
                StartWarp(engine, MAP_OVERWORLD, 28 * 24.0f, 30 * 24.0f, DIR_DOWN, "포차 앞 번화가");
            } else if (p->current_map == MAP_APARTMENT_STAIRS) {
                StartWarp(engine, MAP_OVERWORLD, 42 * 24.0f, 40 * 24.0f, DIR_DOWN, "아파트 101동 앞");
            } else if (p->current_map == MAP_APARTMENT_302) {
                StartWarp(engine, MAP_APARTMENT_STAIRS, 6 * 24.0f, 5 * 24.0f, DIR_DOWN, "아파트 계단실 (3F)");
            } else if (p->current_map == MAP_OFFICE_INTERIOR) {
                StartWarp(engine, MAP_OVERWORLD, 10 * 24.0f, 42 * 24.0f, DIR_DOWN, "테헤란로 오피스 거리");
            } else if (p->current_map == MAP_SENIOR_HALL) {
                StartWarp(engine, MAP_OVERWORLD, 24 * 24.0f, 20 * 24.0f, DIR_DOWN, "고향 마을 어귀");
            }
        } else if (p->current_map == MAP_APARTMENT_STAIRS && stand_tile == TILE_DOORBELL) {
            StartWarp(engine, MAP_APARTMENT_302, 9 * 24.0f, 11 * 24.0f, DIR_UP, "우리 집 아파트 302호");
        }
    }

    // 8. Interaction Key (Z / Space)
    if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
        Scenario_TryInteract(&engine->scenario, p);
    }

    // 9. Smooth Camera Follow
    float target_cam_x = p->x;
    float target_cam_y = p->y;
    engine->camera.target.x += (target_cam_x - engine->camera.target.x) * (dt * 6.0f);
    engine->camera.target.y += (target_cam_y - engine->camera.target.y) * (dt * 6.0f);
}

void LifeEngine_Render(LifeEngine* engine) {
    PlayerState* p = &engine->player;
    MasterPalette pal = Palette_GetForAct((LifeAct)p->current_act);
    float sat = p->color_saturation;

    if (p->current_act >= ACT_5_DEATH_REPLAY) sat = 1.0f;

    if (p->current_act == ACT_0_INFANT) {
        RenderWorld_DrawInfant1stPerson(
            engine->blink_alpha,
            engine->vision_clarity,
            engine->infant_milestone,
            engine->baby_hand_y,
            engine->mobile_angle
        );
        return;
    }

    BeginMode2D(engine->camera);

    // Draw Current Map
    RenderWorld_DrawMap(p->current_map, &pal, sat, (LifeAct)p->current_act, engine->camera, p->stair_sensor_light);

    // Draw Map-specific NPCs
    int act_idx = 0;
    if (p->current_act == ACT_1_CHILDHOOD) act_idx = 0;
    else if (p->current_act == ACT_2_YOUTH) act_idx = 1;
    else if (p->current_act == ACT_3_ADULTHOOD) act_idx = 2;
    else if (p->current_act == ACT_4_ELDERLY) act_idx = 3;
    else if (p->current_act >= ACT_5_DEATH_REPLAY) act_idx = 4;

    int npc_count = g_stage_npc_counts[act_idx];
    for (int i = 0; i < npc_count; i++) {
        NPC* npc = &g_stage_npcs[act_idx][i];
        if (npc->map_id == p->current_map) {
            RenderWorld_DrawNPC(npc, &pal, sat, GetTime());
        }
    }

    RenderWorld_DrawPlayerTopDown(p, &pal);

    if (p->color_spark_timer > 0.0f) {
        RenderWorld_DrawColorBloomWave(p->x, p->y, p->color_bloom_radius, 600.0f);
    }

    EndMode2D();

    RenderWorld_DrawHUD(p, engine->total_lived_years, sat);

    float bar_ratio = 1.0f - (engine->act_timer / engine->act_duration);
    if (bar_ratio < 0.0f) bar_ratio = 0.0f;
    DrawRectangle(SCREEN_WIDTH - 150, 14, 130, 8, (Color){ 20, 20, 24, 180 });
    DrawRectangle(SCREEN_WIDTH - 148, 16, (int)(126 * bar_ratio), 4, (Color){ 240, 107, 50, 255 });
    RenderWorld_DrawPixelText("계절의 흐름", SCREEN_WIDTH - 150, 26, 11, (Color){ 200, 200, 200, 255 });

    // Location Banner
    RenderWorld_DrawLocationBanner(s_location_banner_name, s_location_banner_timer);

    // Screen Warp Fade Transition
    if (p->is_warping) {
        RenderWorld_DrawWarpFade(p->warp_fade_timer);
    }

    Scenario_Render(&engine->scenario, sat);
}
