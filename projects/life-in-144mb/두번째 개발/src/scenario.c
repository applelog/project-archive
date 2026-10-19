#include "scenario.h"
#include "audio_synth.h"
#include <string.h>

void Scenario_Init(ScenarioManager* sm) {
    sm->shard_count = 4;
    sm->dialogue_active = false;
    sm->current_dialogue_npc = -1;
    sm->bloom_timer = 0.0f;
    sm->bloom_x = 0.0f;
    sm->bloom_y = 0.0f;
    sm->alert_gauge = 0.0f;
    
    sm->tut_moved = false;
    sm->tut_talked = false;
    sm->tut_shard_collected = false;
    sm->tut_boarded = false;
    
    // Shard 0: Subway Platform Bench
    sm->shards[0] = (MemoryShard){ 0, AREA_SUBWAY_PLATFORM, 320, 265, "낡은 지하철 승차권", "손등의 주름... 내가 기억하는 내 손이 아니야.", false };
    // Shard 1: Market Alley
    sm->shards[1] = (MemoryShard){ 1, AREA_MARKET_ALLEY, 340, 220, "작은 배냇저고리", "아기 냄새... 내가 이 옷을 밤새 바느질해서 입혔었지.", false };
    // Shard 2: School Playground Swings
    sm->shards[2] = (MemoryShard){ 2, AREA_SCHOOL_PLAYGROUND, 450, 140, "노란 유치원 가방", "\"엄마! 나 오늘 상 받았어!\" 아이의 목소리가 귓가에 맴돈다.", false };
    // Shard 3: Old House Vanity
    sm->shards[3] = (MemoryShard){ 3, AREA_OLD_HOUSE, 360, 110, "낡은 가족사진", "사진 속 다정한 어머니의 얼굴... 바로 지금의 나였구나.", false };
    
    // NPCs
    sm->npc_count = 4;
    sm->npcs[0] = (NPC){ 0, AREA_SUBWAY_PLATFORM, 420, 220, DIR_LEFT, "역무원", 
                         "아가씨, 혼자서 어딜 가시려고요? 표는 챙기셨나요?", 
                         "할머니... 아니, 댁이 어디시길래 이 시간에 혼자...", false };
                         
    sm->npcs[1] = (NPC){ 1, AREA_MARKET_ALLEY, 200, 220, DIR_DOWN, "채소가게 상인", 
                         "새댁, 오늘 콩나물 싱싱해! 한 봉지 사가!", 
                         "어르신, 혹시 길을 잃으셨나요? 가족분들 전화번호 아세요?", false };
                         
    sm->npcs[2] = (NPC){ 2, AREA_SCHOOL_PLAYGROUND, 300, 180, DIR_RIGHT, "문방구 할아버지", 
                         "젊은 처자가 왜 초등학교 앞을 서성이고 있어?", 
                         "어머님, 날이 저무는데 얼른 댁으로 들어가셔야죠...", false };
                         
    sm->npcs[3] = (NPC){ 3, AREA_OLD_HOUSE, 180, 140, DIR_DOWN, "이웃집 할머니", 
                         "새댁네 둘째 아이가 벌써 걸음마를 뗐다면서?", 
                         "영순이 엄마... 나야, 옆집 순이 엄마. 나 못 알아보겠어?", false };
}

void Scenario_Update(ScenarioManager* sm, Player* player, float dt) {
    if (sm->bloom_timer > 0.0f) {
        sm->bloom_timer -= dt;
    }
    
    // Alert gauge accumulates over time (Wandering at night)
    sm->alert_gauge += dt * 0.8f; // ~125 seconds per full cycle
    player->wandering_time = sm->alert_gauge;
}

bool Scenario_TryInteract(ScenarioManager* sm, Player* player) {
    // 1. If dialogue is already active, dismiss it immediately!
    if (sm->dialogue_active) {
        sm->dialogue_active = false;
        sm->current_dialogue_npc = -1;
        return true;
    }
    
    // 2. Check Shard pickup in current area
    for (int i = 0; i < sm->shard_count; i++) {
        if (!sm->shards[i].is_collected && sm->shards[i].area == player->current_area) {
            float dx = player->x - sm->shards[i].x;
            float dy = player->y - sm->shards[i].y;
            if (dx*dx + dy*dy < 36.0f * 36.0f) {
                sm->shards[i].is_collected = true;
                player->collected_shards++;
                player->memory_clarity = 0.25f + ((float)player->collected_shards / (float)sm->shard_count) * 0.75f;
                sm->bloom_timer = 2.5f;
                sm->bloom_x = sm->shards[i].x;
                sm->bloom_y = sm->shards[i].y;
                sm->tut_shard_collected = true;
                AudioSynth_PlaySFX(SFX_MEMORY_SHIMMER);
                return true;
            }
        }
    }
    
    // 3. Check NPC dialogue in current area
    for (int i = 0; i < sm->npc_count; i++) {
        if (sm->npcs[i].area == player->current_area) {
            float dx = player->x - sm->npcs[i].x;
            float dy = player->y - sm->npcs[i].y;
            if (dx*dx + dy*dy < 40.0f * 40.0f) {
                sm->current_dialogue_npc = i;
                sm->dialogue_active = true;
                sm->tut_talked = true;
                AudioSynth_PlaySFX(SFX_SUBWAY_CHIME);
                return true;
            }
        }
    }
    
    return false;
}

GameState Scenario_EvaluateEnding(const ScenarioManager* sm, const Player* player) {
    // Ending A: All 4 memories found
    if (player->collected_shards >= 4) {
        return STATE_ENDING_A_TRUE;
    }
    // Time exceeded
    if (sm->alert_gauge >= 100.0f) {
        if (player->collected_shards >= 2) {
            return STATE_ENDING_B_FAMILY;
        } else {
            return STATE_ENDING_C_HOSPITAL;
        }
    }
    return STATE_GAMEPLAY;
}
