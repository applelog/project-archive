#ifndef LIFE_ENGINE_H
#define LIFE_ENGINE_H

#include "types.h"
#include "palette.h"
#include "scenario.h"
#include <raylib.h>

typedef struct {
    PlayerState player;
    Camera2D camera;
    ScenarioManager scenario;
    
    // Finite Life Clock & Progression
    float act_timer;            // Current act timer
    float act_duration;         // Time allocated for current act
    float total_lived_years;    // 0.0 ~ 80.0
    
    // Act 0 Infant 1st-Person Variables
    float blink_alpha;
    float vision_clarity;
    int infant_milestone;
    float baby_hand_y;
    float mobile_angle;
    float zoom_transition;      // 1.0 (1st person) -> 0.0 (top down)
    
    bool is_game_over;
} LifeEngine;

void LifeEngine_Init(LifeEngine* engine);
void LifeEngine_Update(LifeEngine* engine, float dt);
void LifeEngine_Render(LifeEngine* engine);
void LifeEngine_TransitionToNextAct(LifeEngine* engine);

#endif // LIFE_ENGINE_H
