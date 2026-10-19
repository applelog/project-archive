#ifndef LIFE_ENGINE_H
#define LIFE_ENGINE_H

#include "types.h"
#include "scenario.h"

typedef struct {
    Player player;
    SubwayTrain train;
    ScenarioManager scenario;
    GameState state;
    Camera2D camera;
    
    // Intro sequence
    float intro_timer;
    int prologue_line_index;
    float eye_blink_ratio;
    float focus_blur_ratio;
    float mirror_zoom;
    
    // Tutorial tracking
    float tut_move_dist;
    
    // Subway Transit
    int selected_station_index;
    AreaId destination_area;
    float transit_timer;
    
    // Ending presentation
    float ending_timer;
} LifeEngine;

void LifeEngine_Init(LifeEngine* engine);
void LifeEngine_Update(LifeEngine* engine, float dt);
void LifeEngine_Render(const LifeEngine* engine);

#endif // LIFE_ENGINE_H
