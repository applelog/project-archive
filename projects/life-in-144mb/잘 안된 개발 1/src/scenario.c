#include "scenario.h"
#include "render_world.h"
#include "audio_synth.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static char s_cur_line1[128];
static char s_cur_line2[128];
static char s_cur_choice_a[96];
static char s_cur_choice_b[96];

void Scenario_Init(ScenarioManager* mgr, PlayerState* player) {
    ScenarioData_Init(player);
    mgr->active = false;
    mgr->current_npc = NULL;
    mgr->is_inspecting_prop = false;
    mgr->inspection_title[0] = '\0';
    mgr->inspection_desc[0] = '\0';
    mgr->selected_choice = 0;
    mgr->typewriter_timer = 0.0f;
    mgr->awaiting_choice = false;
    mgr->memory_log.count = 0;
}

void Scenario_ShowMessage(ScenarioManager* mgr, PlayerState* player, const char* title, const char* msg) {
    mgr->active = true;
    mgr->current_npc = NULL;
    mgr->is_inspecting_prop = true;
    strncpy(mgr->inspection_title, title ? title : "조사", sizeof(mgr->inspection_title) - 1);
    strncpy(mgr->inspection_desc, msg ? msg : "", sizeof(mgr->inspection_desc) - 1);
    mgr->selected_choice = 0;
    mgr->awaiting_choice = false;
    player->in_dialogue = true;
    AudioSynth_PlaySFX(SFX_CHOICE_SELECT);
}

bool Scenario_TryInteract(ScenarioManager* mgr, PlayerState* player) {
    if (mgr->active) return false;

    // 1. Check facing tile for NPCs
    int act_idx = 0;
    if (player->current_act == ACT_1_CHILDHOOD) act_idx = 0;
    else if (player->current_act == ACT_2_YOUTH) act_idx = 1;
    else if (player->current_act == ACT_3_ADULTHOOD) act_idx = 2;
    else if (player->current_act == ACT_4_ELDERLY) act_idx = 3;
    else if (player->current_act >= ACT_5_DEATH_REPLAY) act_idx = 4;

    int count = g_stage_npc_counts[act_idx];
    for (int i = 0; i < count; i++) {
        NPC* npc = &g_stage_npcs[act_idx][i];
        if (npc->map_id != player->current_map) continue;

        float dx = npc->x - player->x;
        float dy = npc->y - player->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < 36.0f) {
            mgr->active = true;
            mgr->current_npc = npc;
            mgr->is_inspecting_prop = false;
            mgr->selected_choice = 0;
            mgr->typewriter_timer = 0.0f;
            mgr->awaiting_choice = true;
            player->in_dialogue = true;

            ScenarioData_GetDialogue(
                npc, player,
                s_cur_line1, sizeof(s_cur_line1),
                s_cur_line2, sizeof(s_cur_line2),
                s_cur_choice_a, sizeof(s_cur_choice_a),
                s_cur_choice_b, sizeof(s_cur_choice_b)
            );

            AudioSynth_PlaySFX(SFX_CHOICE_SELECT);
            return true;
        }
    }

    // 2. Check facing tile for Inspectable Props (TV, Fridge, Bookshelf, Locked Doors, etc.)
    int target_tx = (int)(player->x / 24.0f);
    int target_ty = (int)(player->y / 24.0f);
    if (player->dir == DIR_UP) target_ty -= 1;
    else if (player->dir == DIR_DOWN) target_ty += 1;
    else if (player->dir == DIR_LEFT) target_tx -= 1;
    else if (player->dir == DIR_RIGHT) target_tx += 1;

    char p_title[32];
    char p_desc[96];
    if (ScenarioData_CheckInspectableProp(player->current_map, target_tx, target_ty, p_title, sizeof(p_title), p_desc, sizeof(p_desc))) {
        Scenario_ShowMessage(mgr, player, p_title, p_desc);
        return true;
    }

    return false;
}

void Scenario_Update(ScenarioManager* mgr, PlayerState* player, float dt) {
    if (!mgr->active) return;

    mgr->typewriter_timer += dt;

    if (mgr->is_inspecting_prop) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER)) {
            mgr->active = false;
            mgr->is_inspecting_prop = false;
            player->in_dialogue = false;
        }
        return;
    }

    if (!mgr->current_npc) return;

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        if (mgr->selected_choice > 0) {
            mgr->selected_choice--;
            AudioSynth_PlaySFX(SFX_CHOICE_SELECT);
        }
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        if (mgr->selected_choice < 1) {
            mgr->selected_choice++;
            AudioSynth_PlaySFX(SFX_CHOICE_SELECT);
        }
    }

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER)) {
        NPC* npc = mgr->current_npc;
        ScenarioData_ApplyChoice(npc, player, mgr->selected_choice, &mgr->memory_log);

        mgr->active = false;
        mgr->current_npc = NULL;
        player->in_dialogue = false;
    }
}

void Scenario_Render(const ScenarioManager* mgr, float saturation) {
    if (!mgr->active) return;

    if (mgr->is_inspecting_prop) {
        RenderWorld_DrawDialogueBox(
            mgr->inspection_title,
            "조사",
            mgr->inspection_desc,
            "",
            "",
            "",
            0,
            saturation
        );
        return;
    }

    if (!mgr->current_npc) return;

    NPC* npc = mgr->current_npc;
    RenderWorld_DrawDialogueBox(
        npc->name,
        npc->role_title,
        s_cur_line1,
        s_cur_line2,
        s_cur_choice_a,
        s_cur_choice_b,
        mgr->selected_choice,
        saturation
    );
}

const MemoryLog* Scenario_GetMemoryLog(const ScenarioManager* mgr) {
    return &mgr->memory_log;
}
