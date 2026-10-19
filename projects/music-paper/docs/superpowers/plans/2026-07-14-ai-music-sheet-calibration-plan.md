# AI Music Sheet Calibration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a hybrid audio-to-sheet analysis pipeline with relaxed compressor settings to prevent volume pumping, and integrate Gemini-based AI calibration of pitch/chord sequences with custom user prompts.

**Architecture:** Refactor the transcription endpoint to analyze vocals for melody and MR for chords, route pitch/chord arrays through Gemini API (with local rule-based fallback), and expose UI configurations for settings.

**Tech Stack:** Flask, HTML/JS/CSS (VexFlow), Pedalboard, basic-pitch, music21, google-generativeai

---

## Global Constraints

- **Python Environment**: Conda min environment (Python 3.8.13).
- **Server Port**: Flask running on port `5001`.
- **API Key Portability**: Respect user-supplied keys in requests or `GEMINI_API_KEY` env var. Do not hardcode keys.
- **Graceful Failure**: If Gemini API fails or MuseScore CLI is missing, fallback gracefully to local rule-based parsing and frontend VexFlow rendering.

---

## Task 1: UI Upgrades for Settings & Calibration Customization

**Files:**
- Modify: `templates/index.html`

**Interfaces:**
- Consumes: None (UI elements)
- Produces: Updates `/upload` and `/transcribe` fetch payloads to include custom header `X-Gemini-API-Key` and parameters `apply_compression` and `calibration_prompt`.

- [ ] **Step 1: Add Settings Gear Icon and API Key Input Modal**
  Add a settings cog icon to the top right of the header and a simple CSS-styled modal allowing the user to input and save their `GEMINI_API_KEY` to `localStorage`.
  - Add to `<style>`:
    ```css
    .settings-btn { position: absolute; top: 20px; right: 20px; background: none; border: none; font-size: 24px; cursor: pointer; color: var(--text-muted); transition: color 0.2s; z-index: 10; }
    .settings-btn:hover { color: var(--primary-light); }
    .modal { display: none; position: fixed; inset: 0; background: rgba(0,0,0,0.6); backdrop-filter: blur(5px); z-index: 1000; align-items: center; justify-content: center; }
    .modal.show { display: flex; }
    .modal-content { background: var(--bg2); border: 1px solid var(--border); border-radius: var(--radius); padding: 24px; width: 90%; max-width: 450px; position: relative; }
    .modal-close { position: absolute; top: 16px; right: 16px; background: none; border: none; color: var(--text-muted); font-size: 20px; cursor: pointer; }
    .modal-title { font-size: 18px; font-weight: 700; margin-bottom: 16px; }
    .form-group { margin-bottom: 16px; }
    .form-group label { display: block; font-size: 13px; color: var(--text-muted); margin-bottom: 6px; }
    .form-control { width: 100%; padding: 10px; background: rgba(255,255,255,0.05); border: 1px solid var(--border); border-radius: 8px; color: var(--text); font-family: inherit; font-size: 14px; }
    .form-control:focus { outline: none; border-color: var(--primary-light); }
    ```
  - Add to HTML `<body>`:
    ```html
    <button class="settings-btn" id="openSettings" title="설정">⚙️</button>
    <div class="modal" id="settingsModal">
      <div class="modal-content">
        <button class="modal-close" id="closeSettings">✕</button>
        <div class="modal-title">⚙️ 설정</div>
        <div class="form-group">
          <label for="apiKeyInput">Gemini API Key</label>
          <input type="password" class="form-control" id="apiKeyInput" placeholder="AIzaSy..." />
          <p style="font-size: 11px; color: var(--text-muted); margin-top: 4px;">입력한 키는 브라우저(localStorage)에 안전하게 저장됩니다.</p>
        </div>
        <button class="btn btn-primary" id="saveSettings" style="margin-top:8px;">저장하기</button>
      </div>
    </div>
    ```

- [ ] **Step 2: Add Option Panel inside Upload Card**
  Add a toggler for Compressor/Pumping control and a textarea for custom AI prompts below the drag-and-drop zone.
  - Insert in `<div class="card">` right before the "MR 분리 시작" button:
    ```html
    <div style="margin-top: 16px; padding: 14px; background: rgba(255,255,255,0.02); border: 1px solid var(--border); border-radius: var(--radius-sm);">
      <label style="display:flex; align-items:center; gap:8px; font-size:14px; cursor:pointer; font-weight:600;">
        <input type="checkbox" id="applyCompression" checked style="width:16px; height:16px; accent-color:var(--primary);" />
        음량 펌핑 방지 컴프레서 적용 (음질 개선)
      </label>
    </div>
    <div style="margin-top: 12px; display:flex; flex-direction:column; gap:6px;">
      <label for="calibrationPrompt" style="font-size: 13px; color: var(--text-muted); font-weight:600;">AI 악보 보정 가이드 (선택)</label>
      <textarea id="calibrationPrompt" rows="2" class="form-control" style="resize:none;" placeholder="예: 이 곡은 잔잔한 발라드입니다. 멜로디 음표를 단순하고 매끄럽게 정리해주세요."></textarea>
    </div>
    ```

- [ ] **Step 3: Save settings locally and attach payload to Fetch requests**
  Write JS event listeners to handle the Settings modal save/load and modify `/upload` and `/transcribe` fetch calls to forward parameters.
  - Load `GEMINI_API_KEY` from localStorage on page load:
    ```javascript
    document.addEventListener('DOMContentLoaded', () => {
      const savedKey = localStorage.getItem('GEMINI_API_KEY');
      if (savedKey) document.getElementById('apiKeyInput').value = savedKey;
    });
    ```
  - Settings save:
    ```javascript
    document.getElementById('openSettings').addEventListener('click', () => document.getElementById('settingsModal').classList.add('show'));
    document.getElementById('closeSettings').addEventListener('click', () => document.getElementById('settingsModal').classList.remove('show'));
    document.getElementById('saveSettings').addEventListener('click', () => {
      const key = document.getElementById('apiKeyInput').value.trim();
      localStorage.setItem('GEMINI_API_KEY', key);
      document.getElementById('settingsModal').classList.remove('show');
      showToast('설정이 저장되었습니다!', 'success');
    });
    ```
  - Pass parameter `apply_compression` during `/upload`:
    Modify `startSeparation()`:
    ```javascript
    const applyComp = document.getElementById('applyCompression').checked;
    const formData = new FormData();
    formData.append('file', file);
    formData.append('apply_compression', applyComp ? 'true' : 'false');
    ```
  - Pass custom header `X-Gemini-API-Key` and parameter `calibration_prompt` during `/transcribe`:
    Modify `/transcribe/<task_id>/vocal` fetch in click handler for `makeSheetFromVocal`:
    ```javascript
    const apiKey = localStorage.getItem('GEMINI_API_KEY') || '';
    const promptText = document.getElementById('calibrationPrompt').value;
    const res = await fetch(`/transcribe/${currentTaskId}/vocal`, {
      method: 'POST',
      headers: { 
        'Content-Type': 'application/json',
        'X-Gemini-API-Key': apiKey
      },
      body: JSON.stringify({ calibration_prompt: promptText })
    });
    ```
    Also modify `startSheetDirect()` to send the key and prompt text similarly to `/transcribe/direct`.

- [ ] **Step 4: Verify UI layout and settings saving**
  Open the web app or verify HTML syntactical validity. Ensure that localStorage persists input correctly.

- [ ] **Step 5: Commit UI modifications**
  ```bash
  git add templates/index.html
  git commit -m "feat: add settings UI and configuration options to frontend"
  ```

---

## Task 2: Backend Tuned Compressor & Request Arguments Support

**Files:**
- Modify: `app.py`

**Interfaces:**
- Consumes: Request arguments `apply_compression` (JSON or form-data).
- Produces: Adjusted `apply_compressor` settings. Optional execution of compressor inside `run_separation`.

- [ ] **Step 1: Adjust Compressor Parameters in `apply_compressor`**
  Modify line range 122-137 in `app.py` to relax compression parameters:
  ```python
  def apply_compressor(audio: np.ndarray, sr: int) -> np.ndarray:
      """보컬 제거 후 볼륨 pumping 억제 — 부드러운 컴프레서 + 리미터"""
      board = Pedalboard([
          Compressor(threshold_db=-12.0, ratio=2.0, attack_ms=15.0, release_ms=150.0),
          Limiter(threshold_db=-1.0, release_ms=100.0),
      ])
      ...
  ```

- [ ] **Step 2: Read `apply_compression` option in upload route and threads**
  Modify `/upload` endpoint in `app.py` to extract `apply_compression`:
  ```python
  apply_comp_str = request.form.get('apply_compression', 'true')
  apply_compression = apply_comp_str.lower() == 'true'
  ...
  # Pass apply_compression to run_separation
  t = threading.Thread(target=run_separation, args=(input_path, task_id, apply_compression))
  ```
  And modify `run_separation` signature:
  ```python
  def run_separation(input_path: Path, task_id: str, apply_compression: bool = True):
  ```
  Ensure inside `run_separation` the compressor is executed only if `apply_compression` is true:
  ```python
          # MR 임시 저장 후 컴프레서 적용
          save_audio(mr_tensor, str(mr_path), samplerate=model.samplerate)
          if apply_compression:
              tasks[task_id].update(progress=88, message='🎚️ 볼륨 보정 중...')
              mr_np, sr_mr = sf.read(str(mr_path))
              mr_fixed = apply_compressor(mr_np, sr_mr)
              sf.write(str(mr_path), mr_fixed, sr_mr)
  ```

- [ ] **Step 3: Test Upload workflow with bypassed compression**
  Verify the python file compiles and executes without syntax errors. Run flask server locally or verify imports.

- [ ] **Step 4: Commit compression modifications**
  ```bash
  git add app.py
  git commit -m "feat: tune compressor parameters and add option to bypass compression"
  ```

---

## Task 3: Refactor Transcription Pipeline to Hybrid Model

**Files:**
- Modify: `app.py`

**Interfaces:**
- Consumes: Task ID, output directory.
- Produces: Modified `audio_to_sheet` accepting `melody_audio_path` and `chord_audio_path`.

- [ ] **Step 1: Modify `audio_to_sheet` to accept dual audio sources**
  Modify signature of `audio_to_sheet` to process melody and chords from separate tracks:
  ```python
  def audio_to_sheet(melody_path: str, chord_path: str, out_dir: Path, label: str) -> dict:
      """오디오 → 주선율(melody_path) + 코드감지(chord_path) → MuseScore PDF"""
      ...
      # ① basic-pitch: Run melody extraction on melody_path (vocals)
      _, midi_data, _ = predict(melody_path, ICASSP_2022_MODEL_PATH)
      midi_data.write(midi_path)

      # ② chord detection: Run beat-tracking/chroma on chord_path (MR)
      chords, bpm = detect_chords_per_measure(chord_path)
      ...
  ```

- [ ] **Step 2: Refactor `/transcribe/<task_id>/<track>`**
  Identify vocal track and MR track from task storage, pass vocal track to `melody_path` and MR track to `chord_path`:
  ```python
  @app.route('/transcribe/<task_id>/<track>', methods=['POST'])
  def transcribe(task_id, track):
      if task_id not in tasks or tasks[task_id].get('status') not in ('done', 'sheet_done'):
          return jsonify({'error': 'MR 분리가 완료되지 않았습니다.'}), 400

      vocal_path = tasks[task_id].get('vocal_file')
      mr_path = tasks[task_id].get('mr_file')
      
      if not vocal_path or not mr_path:
          return jsonify({'error': '분리된 오디오 파일을 찾을 수 없습니다.'}), 404

      out_dir = OUTPUT_DIR / task_id
      label = f'sheet_{track}'

      # Extract user prompt from post request body
      req_data = request.get_json() or {}
      prompt = req_data.get('calibration_prompt', '')
      api_key = request.headers.get('X-Gemini-API-Key') or os.environ.get('GEMINI_API_KEY', '')

      tasks[task_id].update(status='transcribing', progress=5, message='🎼 악보 생성 준비 중...')
      t = threading.Thread(target=run_transcribe,
                           args=(vocal_path, mr_path, out_dir, task_id, label, prompt, api_key))
      t.daemon = True
      t.start()
      return jsonify({'status': 'started', 'task_id': task_id})
  ```
  And update `run_transcribe`:
  ```python
  def run_transcribe(vocal_path: str, mr_path: str, out_dir: Path, task_id: str, label: str, prompt: str, api_key: str):
      try:
          tasks[task_id].update(status='transcribing', progress=10, message='🎼 음표를 분석하고 있습니다...')
          result = audio_to_sheet_hybrid(vocal_path, mr_path, out_dir, label, prompt, api_key)
          ...
  ```

- [ ] **Step 3: Modify Direct Transcribe Endpoint to auto-split**
  If a direct upload is performed in `/transcribe/direct`, run Demucs under the hood first to split them, then perform hybrid analysis.
  ```python
  # In transcribe_direct, inside the thread target:
  # 1. Run Demucs on direct file (similar to run_separation)
  # 2. Extract vocal and mr files
  # 3. Call audio_to_sheet_hybrid
  ```

- [ ] **Step 4: Commit hybrid refactoring**
  ```bash
  git add app.py
  git commit -m "feat: implement hybrid transcription pipeline splitting melody and chord analysis"
  ```

---

## Task 4: Gemini AI Calibration Integration & Rule-Based Fallback

**Files:**
- Modify: `app.py`

**Interfaces:**
- Consumes: JSON notes list, JSON chords list, user prompt, API key.
- Produces: Corrected JSON notes list and chords list.

- [ ] **Step 1: Write `calibrate_music_data_with_gemini` function**
  Define a function using standard Gemini API call:
  ```python
  def calibrate_music_data_with_gemini(raw_notes: list, raw_chords: list, user_prompt: str, api_key: str) -> dict:
      """Gemini API를 사용하여 노츠와 코드를 정밀 교정"""
      import google.generativeai as genai
      import json
      
      genai.configure(api_key=api_key)
      model = genai.GenerativeModel('gemini-1.5-flash', generation_config={"response_mime_type": "application/json"})
      
      # 150개 노트 이상은 토큰 절약 및 정밀도를 위해 자름
      truncated_notes = raw_notes[:150]
      
      prompt = f"""
      You are an expert music transcriber. Clean up this raw transcription data.
      RAW NOTES: {json.dumps(truncated_notes)}
      RAW CHORDS: {json.dumps(raw_chords)}
      USER STYLE PROMPT: {user_prompt}
      
      Tasks:
      1. Determine key.
      2. Filter out short ghost notes (duration < 0.15s).
      3. Align pitch to the detected key/scale.
      4. Correct chords to fit standard diatonic progressions in that key.
      
      Return JSON:
      {{
        "key": "Detected Key",
        "notes": [{"pitch": int, "start": float, "duration": float}],
        "chords": [{"measure": int, "offset": float, "chord": "str"}]
      }}
      """
      response = model.generate_content(prompt)
      return json.loads(response.text)
  ```

- [ ] **Step 2: Write rule-based fallback method**
  ```python
  def calibrate_music_data_rules(raw_notes: list, raw_chords: list) -> dict:
      """기본 룰 기반 노이즈 필터링 (Gemini API 키가 없을 때 작동)"""
      filtered_notes = [n for n in raw_notes if n['duration'] >= 0.18]
      return {
          "key": "Unknown (Rule-based)",
          "notes": filtered_notes,
          "chords": raw_chords
      }
  ```

- [ ] **Step 3: Integrate Calibration inside hybrid audio-to-sheet**
  Call the calibrator in `audio_to_sheet_hybrid`:
  ```python
  # Detect melody and chords
  # Build lists of notes and chords
  if api_key:
      try:
          calibrated = calibrate_music_data_with_gemini(notes_json, chords, prompt, api_key)
          notes_json = calibrated.get('notes', notes_json)
          chords = calibrated.get('chords', chords)
      except Exception as e:
          print("AI Calibration failed:", e)
          calibrated = calibrate_music_data_rules(notes_json, chords)
          notes_json = calibrated['notes']
  else:
      calibrated = calibrate_music_data_rules(notes_json, chords)
      notes_json = calibrated['notes']
  
  # Then build music21 score using calibrated notes & chords
  ```

- [ ] **Step 4: Verify with simulated test payload**
  Run a mock execution in python CLI to ensure API returns valid output structure.

- [ ] **Step 5: Commit calibration code**
  ```bash
  git add app.py
  git commit -m "feat: integrate Gemini AI calibration and rule-based fallback"
  ```

---

## Task 5: VexFlow Rendering Fallback & Graceful Degradation

**Files:**
- Modify: `templates/index.html`
- Modify: `app.py`

**Interfaces:**
- Consumes: PDF presence flag.
- Produces: Handles MuseScore missing error, rendering notes directly on canvas via VexFlow.

- [ ] **Step 1: Check MuseScore CLI Availability**
  In `app.py` `audio_to_sheet_hybrid`, if `pdf_path` is not created (e.g. `find_musescore()` returned `None`), ensure the task state sets `pdf_path = None` and returns notes/chords structure successfully without crash.

- [ ] **Step 2: Update UI handling in `onSheetDone`**
  Modify `onSheetDone(taskId, data)` in `templates/index.html`:
  If `data.pdf_path` is null, show VexFlow container (`#sheetCanvas`) and run `renderVexFlow(data.notes)`. Show a notice "서버에 MuseScore가 없어 웹 뷰어로 악보를 표시합니다."
  - Add to HTML:
    ```html
    <div id="sheetCanvas" style="width:100%; overflow-x:auto; background:#fff; border-radius:12px; padding:20px; display:none; margin-bottom:14px;"></div>
    ```
  - Update `onSheetDone`:
    ```javascript
    if (data.pdf_path) {
      document.getElementById('sheetPdfWrap').style.display = 'block';
      document.getElementById('sheetCanvas').style.display = 'none';
      ...
    } else {
      document.getElementById('sheetPdfWrap').style.display = 'none';
      document.getElementById('sheetCanvas').style.display = 'block';
      renderVexFlow(data.notes);
      showToast('서버에 MuseScore가 없어 웹 악보로 렌더링했습니다.', 'success');
    }
    ```

- [ ] **Step 3: Test entire pipeline**
  Run Flask, verify that a transcribing request executes successfully.

- [ ] **Step 4: Commit VexFlow updates**
  ```bash
  git add templates/index.html app.py
  git commit -m "feat: implement graceful fallback to client-side VexFlow rendering when MuseScore is missing"
  ```
