# Design Spec: AI Music Sheet Calibration & Hybrid Pipeline

This specification outlines the design for upgrading the AI Music Studio web application to produce clean, professional-grade sheet music. It addresses issues of volume pumping in MR extraction, messy chord progressions, and noisy melody lines by introducing a hybrid processing pipeline and Gemini-based AI calibration.

---

## 1. Objectives

- **Calibrate MR Extraction Dynamics**: Relax compressor settings to prevent volume pumping and audio distortion. Make compression optional.
- **Hybrid Extraction Pipeline**: Decouple melody and chord sources. Extract melody from the Vocal track and chords from the MR track.
- **Gemini-powered AI Calibration**: Use the Gemini API to clean up noisy MIDI notes (ghost notes) and regularize chord progressions based on music theory, scale alignment, and user-provided style prompts.
- **Portability & Zero-Cost Readiness**: Maintain 100% serverless/portable fallback capability. Allow user-supplied Gemini API Keys (saved in browser `localStorage`), server-side keys, and a local rule-based fallback if no key is present.

---

## 2. Architecture & Data Flow

### Hybrid Calibration Pipeline

```
              [Uploaded Song]
                     │
              (Demucs Engine)
              ╱             ╲
     [Vocal Track]       [MR Track]
          │                  │
    (Basic Pitch)      (Chroma Beats)
          │                  │
   [Raw Melody MIDI]    [Raw Chords]
          ╲                  ╱
        (Gemini AI Calibrator)  <── [User Style Prompt] & [API Key]
                  │
        [Refined Melody & Chords]
                  │
        (music21 Sheet Builder)
                  │
          (MuseScore Engine)
                  │
              [Final PDF]
```

### Components

#### A. Frontend UI Modifications (`templates/index.html`)
1. **Settings Modal**: A gear-icon modal letting users input their own `GEMINI_API_KEY`. Saved locally in browser `localStorage`.
2. **Options Panel**:
   - Checkbox: "볼륨 보정(컴프레서) 적용" (Apply volume compression) — Default: checked.
   - Textarea: "AI 악보 보정 스타일 가이드" (AI Sheet Calibration Prompt) — Default: blank (e.g., "이 곡은 잔잔한 C major 발라드곡이야, 멜로디를 깔끔하게 정돈해줘").
3. **API Requests**: Pass `api_key`, `apply_compression`, and `calibration_prompt` in payload/headers for `/transcribe` and `/upload` requests.

#### B. Audio Compression Tuning (`app.py`)
1. Relax `Pedalboard` compressor/limiter settings:
   - `Compressor(threshold_db=-12, ratio=2.0, attack_ms=15.0, release_ms=150.0)`
   - `Limiter(threshold_db=-1.0, release_ms=100.0)`
2. Add flag to bypass `apply_compressor` if `apply_compression` parameter is false.

#### C. Hybrid Transcribe Endpoint (`app.py`)
1. When `/transcribe/<task_id>/vocal` is invoked:
   - Identify both `vocal_file` and `mr_file` belonging to the task.
   - Extract melody from the `vocal_file`.
   - Extract chords and BPM from the `mr_file`.
2. When `/transcribe/direct` (direct upload) is invoked:
   - Run Demucs separation under the hood asynchronously first to obtain temporary Vocal and MR tracks.
   - Proceed with the same hybrid extraction (Melody from Vocal, Chords from MR). This guarantees high-quality results even for direct uploads.

#### D. Gemini AI Calibrator (`app.py`)
1. Check for `api_key` in request headers, fallback to environment variable `GEMINI_API_KEY`.
2. If an API key is available:
   - Construct a JSON payload of raw melody notes: `[{"pitch": 60, "start": 0.5, "duration": 0.5}, ...]` (capped to first 150 notes to save tokens and fit constraints).
   - Construct a JSON payload of raw chords: `[{"measure": 0, "chord": "C"}, ...]`
   - Formulate a prompt describing the task:
     - Identify the key.
     - Remove short ghost notes.
     - Align notes to scale.
     - Correct out-of-place chords.
     - Apply user's custom style prompt.
     - Output the response strictly as a JSON object matching the input structure.
   - Call Gemini API using python's `google-generativeai` SDK or raw HTTPS POST.
   - Parse and load the corrected JSON.
3. If no key is available or the API call fails:
   - Fall back to a rule-based sanitizer: filter out notes with `duration < 0.15` seconds, and snap notes to standard diatonic scales.

---

## 3. Gemini API Prompt Design

```
You are an expert music transcription post-processor. 
Your input consists of raw melody notes and raw measure-by-measure chords detected from an audio file.

RAW MELODY NOTES: {raw_notes_json}
RAW CHORDS: {raw_chords_json}
USER CUSTOM STYLE PROMPT: {user_prompt}

Please perform the following operations:
1. Detect the key of the song (e.g., C Major, A Minor).
2. Filter out "ghost notes" (spurious notes with duration < 0.15s or volume spikes that don't fit melodic contour).
3. Align/quantize note pitches to fit the detected key (correcting out-of-key sharps/flats unless they are chromatic passing notes).
4. Regularize chord progression: substitute weird chords with standard diatonic progressions fitting the detected key (e.g., if key is C Major, change G#m to Am or G).
5. Adjust behavior based on USER CUSTOM STYLE PROMPT.

Output MUST be a JSON object containing:
{
  "key": "C Major",
  "notes": [
    {"pitch": 60, "start": 0.5, "duration": 0.5},
    ...
  ],
  "chords": [
    {"measure": 0, "offset": 0.0, "chord": "C"},
    ...
  ]
}
```

---

## 4. Fallbacks & Error Handling

- **API Failures**: If Gemini API returns invalid JSON or fails (rate limits, network issues), catch the exception, log the error, and execute the rule-based local correction. Notify the user via the progress message ("AI 보정 실패로 기본 보정이 적용되었습니다").
- **Missing MuseScore on Server**: If `find_musescore()` returns `None` (typical on cloud hosts), return the `notes` and `chords` to the frontend, render inline using VexFlow, and hide the MuseScore PDF frame.

---

## 5. Verification Plan

1. **Test relaxed compressor**: Upload a dynamic audio track, check output WAV for volume levels (no extreme pumping).
2. **Verify hybrid transcription data flow**: Trigger transcription, assert in logs that `basic_pitch` runs on the vocal file and `detect_chords` runs on the MR file.
3. **Verify Gemini API integration**: Use the test key, verify that notes and chords are successfully regularized and returned. Verify fallback logic by temporarily disabling the API key.
