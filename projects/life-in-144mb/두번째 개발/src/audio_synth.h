#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

#include <stdbool.h>

typedef enum {
    BGM_THEME_INTRO = 0,
    BGM_THEME_SEARCHING,
    BGM_THEME_CLUE_FOUND,
    BGM_THEME_TRUE_ENDING,
    BGM_THEME_HOSPITAL
} BGMTheme;

typedef enum {
    SFX_SUBWAY_CHIME = 0,
    SFX_TRAIN_RAIL_LOOP,
    SFX_DOOR_OPEN,
    SFX_DOOR_CLOSE,
    SFX_HEARTBEAT,
    SFX_MEMORY_SHIMMER
} SFXType;

// Core Lifecycle & Updates
void AudioSynth_Init(void);
void AudioSynth_Update(float dt);
void AudioSynth_Close(void);

// BGM Management
void AudioSynth_SetBGM(BGMTheme theme);
void AudioSynth_SetTheme(BGMTheme theme);
BGMTheme AudioSynth_GetTheme(void);

// SFX & Procedural Notes
void AudioSynth_PlaySFX(SFXType sfx);
void AudioSynth_PlaySubwayChime(void);
void AudioSynth_PlayPianoNote(float freq, float volume, float decay);

// Volume Controls
void AudioSynth_SetMasterVolume(float vol);
void AudioSynth_SetBGMVolume(float vol);
void AudioSynth_SetSFXVolume(float vol);
bool AudioSynth_IsInitialized(void);

#endif // AUDIO_SYNTH_H
