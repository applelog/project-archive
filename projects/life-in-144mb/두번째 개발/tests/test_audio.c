#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../src/audio_synth.h"

int main(void) {
    printf("=== Running Task 4: Procedural Audio Synthesizer Tests ===\n");

    // 1. Verify Enum Definitions
    assert(BGM_THEME_INTRO == 0);
    assert(BGM_THEME_SEARCHING == 1);
    assert(BGM_THEME_CLUE_FOUND == 2);
    assert(BGM_THEME_TRUE_ENDING == 3);
    assert(BGM_THEME_HOSPITAL == 4);

    assert(SFX_SUBWAY_CHIME == 0);
    assert(SFX_TRAIN_RAIL_LOOP == 1);
    assert(SFX_DOOR_OPEN == 2);
    assert(SFX_DOOR_CLOSE == 3);
    assert(SFX_HEARTBEAT == 4);
    assert(SFX_MEMORY_SHIMMER == 5);

    // 2. Initialize Audio Synthesizer
    AudioSynth_Init();
    printf("[TEST] AudioSynth_Init() completed. Initialized: %d\n", AudioSynth_IsInitialized());

    // 3. Test Volume Control and Clamping
    AudioSynth_SetMasterVolume(0.5f);
    AudioSynth_SetMasterVolume(1.5f); // Should clamp to 1.0f
    AudioSynth_SetMasterVolume(-0.2f); // Should clamp to 0.0f
    AudioSynth_SetMasterVolume(0.8f);

    AudioSynth_SetBGMVolume(0.7f);
    AudioSynth_SetSFXVolume(0.9f);

    // 4. Test Theme Management
    AudioSynth_SetBGM(BGM_THEME_INTRO);
    assert(AudioSynth_GetTheme() == BGM_THEME_INTRO);

    AudioSynth_SetTheme(BGM_THEME_SEARCHING);
    assert(AudioSynth_GetTheme() == BGM_THEME_SEARCHING);

    AudioSynth_SetTheme(BGM_THEME_CLUE_FOUND);
    assert(AudioSynth_GetTheme() == BGM_THEME_CLUE_FOUND);

    AudioSynth_SetTheme(BGM_THEME_TRUE_ENDING);
    assert(AudioSynth_GetTheme() == BGM_THEME_TRUE_ENDING);

    AudioSynth_SetTheme(BGM_THEME_HOSPITAL);
    assert(AudioSynth_GetTheme() == BGM_THEME_HOSPITAL);

    // 5. Test Procedural Piano Note Triggering
    AudioSynth_PlayPianoNote(261.63f, 0.7f, 1.5f); // C4
    AudioSynth_PlayPianoNote(329.63f, 0.7f, 1.5f); // E4
    AudioSynth_PlayPianoNote(392.00f, 0.7f, 1.5f); // G4

    // 6. Test All SFX Types
    printf("[TEST] Triggering SFX_SUBWAY_CHIME...\n");
    AudioSynth_PlaySFX(SFX_SUBWAY_CHIME);
    AudioSynth_PlaySubwayChime();

    printf("[TEST] Triggering SFX_TRAIN_RAIL_LOOP...\n");
    AudioSynth_PlaySFX(SFX_TRAIN_RAIL_LOOP);

    printf("[TEST] Triggering SFX_DOOR_OPEN...\n");
    AudioSynth_PlaySFX(SFX_DOOR_OPEN);

    printf("[TEST] Triggering SFX_DOOR_CLOSE...\n");
    AudioSynth_PlaySFX(SFX_DOOR_CLOSE);

    printf("[TEST] Triggering SFX_HEARTBEAT...\n");
    AudioSynth_PlaySFX(SFX_HEARTBEAT);

    printf("[TEST] Triggering SFX_MEMORY_SHIMMER...\n");
    AudioSynth_PlaySFX(SFX_MEMORY_SHIMMER);

    // 7. Simulate Real-time Audio Stream Updates for all themes
    BGMTheme test_themes[] = {
        BGM_THEME_INTRO,
        BGM_THEME_SEARCHING,
        BGM_THEME_CLUE_FOUND,
        BGM_THEME_TRUE_ENDING,
        BGM_THEME_HOSPITAL
    };

    for (int t = 0; t < 5; t++) {
        AudioSynth_SetBGM(test_themes[t]);
        // Simulate 60 frames (~1.0 sec) per theme
        for (int frame = 0; frame < 60; frame++) {
            AudioSynth_Update(1.0f / 60.0f);
        }
    }

    // 8. Test Clean Shutdown and Idempotency
    AudioSynth_Close();
    assert(!AudioSynth_IsInitialized());
    AudioSynth_Close(); // Should not crash on duplicate close

    printf("[PASS] All Procedural Audio Synthesizer tests passed successfully!\n");
    return 0;
}
