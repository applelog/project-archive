#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

#include "types.h"

typedef enum {
    SFX_BLINK = 0,
    SFX_STEP,
    SFX_BABY_COO,
    SFX_CHOICE_SELECT,
    SFX_MEMORY_DISCOVER,
    SFX_WIND_AMBIENT,
    SFX_HEARTBEAT,
    SFX_COUNT
} SFXType;

void AudioSynth_Init(void);
void AudioSynth_Update(float dt);
void AudioSynth_PlayBGM(LifeAct act);
void AudioSynth_PlaySFX(SFXType sfx);
void AudioSynth_SetMasterVolume(float volume);
void AudioSynth_Close(void);

#endif // AUDIO_SYNTH_H
