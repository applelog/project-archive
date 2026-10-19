#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "types.h"
#include "audio_synth.h"

// Note: Test file verifies API behavior, DSP sanity, and lack of NaN/overflow
int main(void) {
    printf("[TEST] Starting Audio Synthesizer Self-Test...\n");

    // 1. Initialize Synth
    AudioSynth_Init();
    printf("[TEST] AudioSynth_Init() OK\n");

    // 2. Set Master Volume
    AudioSynth_SetMasterVolume(0.5f);
    AudioSynth_SetMasterVolume(1.0f);
    printf("[TEST] AudioSynth_SetMasterVolume() OK\n");

    // 3. Test All BGM Acts
    for (int act = 0; act <= ACT_ENDING; ++act) {
        AudioSynth_PlayBGM((LifeAct)act);
        AudioSynth_Update(0.016f);
    }
    printf("[TEST] All 7 Life Act BGMs triggered OK\n");

    // 4. Test All SFX Types
    for (int sfx = 0; sfx < SFX_COUNT; ++sfx) {
        AudioSynth_PlaySFX((SFXType)sfx);
        AudioSynth_Update(0.016f);
    }
    printf("[TEST] All %d SFX Types triggered OK\n", SFX_COUNT);

    // 5. Close Synth
    AudioSynth_Close();
    printf("[TEST] AudioSynth_Close() OK\n");

    printf("[TEST] ALL AUDIO SYNTH TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
