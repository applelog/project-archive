import os
os.environ['KMP_DUPLICATE_LIB_OK'] = 'TRUE'  # macOS OpenMP 충돌 방지
import uuid
import shutil
import threading
from pathlib import Path

import numpy as np
import soundfile as sf
import librosa
from pedalboard import Pedalboard, Compressor, Limiter
from flask import Flask, request, jsonify, send_file, render_template
from werkzeug.utils import secure_filename

app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 500 * 1024 * 1024  # 500MB

BASE_DIR = Path(__file__).parent
UPLOAD_DIR = BASE_DIR / 'uploads'
OUTPUT_DIR = BASE_DIR / 'outputs'
UPLOAD_DIR.mkdir(exist_ok=True)
OUTPUT_DIR.mkdir(exist_ok=True)

ALLOWED_EXTENSIONS = {'mp3', 'mp4', 'wav', 'flac', 'm4a', 'ogg', 'aac'}
tasks = {}  # task_id → state dict


def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS


def get_device():
    """Apple Silicon MPS > CPU 순으로 선택"""
    try:
        import torch
        if torch.backends.mps.is_available():
            return 'mps'
    except Exception:
        pass
    return 'cpu'


def run_separation(input_path: Path, task_id: str, apply_compression: bool = True):
    """Demucs Python API로 보컬/MR 분리"""
    try:
        tasks[task_id].update(status='separating', progress=10,
                              message='🎵 AI가 음원을 분석하고 있습니다...')

        # demucs Python API 사용
        import torch
        from demucs.pretrained import get_model
        from demucs.apply import apply_model
        from demucs.audio import AudioFile, save_audio

        device = get_device()
        tasks[task_id].update(progress=20,
                              message=f'🤖 모델 로드 중... ({"GPU" if device == "mps" else "CPU"})')

        model = get_model('htdemucs')
        model.eval()

        tasks[task_id].update(progress=35, message='🎤 보컬과 MR을 분리하고 있습니다...')

        # 오디오 로드 (demucs AudioFile 사용)
        wav = AudioFile(input_path).read(
            streams=0,
            samplerate=model.samplerate,
            channels=model.audio_channels
        )
        ref = wav.mean(0)
        wav = (wav - ref.mean()) / ref.std()

        # 분리 실행 (shifts=0, overlap=0.1 → 빠른 모드)
        with torch.no_grad():
            sources = apply_model(model, wav[None], device=device,
                                  shifts=0, split=True, overlap=0.1,
                                  progress=False)[0]

        sources = sources * ref.std() + ref.mean()

        tasks[task_id].update(progress=80, message='💾 파일을 저장하고 있습니다...')

        # 출력 저장
        task_out = OUTPUT_DIR / task_id
        task_out.mkdir(exist_ok=True)

        stem = input_path.stem
        source_names = model.sources  # ['drums','bass','other','vocals']

        # vocals 인덱스 찾기
        vocal_idx = source_names.index('vocals')

        # MR = 보컬을 제외한 나머지 합산
        mr_tensor = sum(sources[i] for i, s in enumerate(source_names) if s != 'vocals')
        vocal_tensor = sources[vocal_idx]

        mr_path = task_out / f'MR_{stem}.wav'
        vocal_path = task_out / f'Vocal_{stem}.wav'

        # MR 임시 저장 후 컴프레서 적용 (pumping 억제)
        save_audio(mr_tensor, str(mr_path), samplerate=model.samplerate)
        if apply_compression:
            tasks[task_id].update(progress=88, message='🎚️ 볼륨 보정 중...')
            mr_np, sr_mr = sf.read(str(mr_path))
            mr_fixed = apply_compressor(mr_np, sr_mr)
            sf.write(str(mr_path), mr_fixed, sr_mr)

        save_audio(vocal_tensor, str(vocal_path), samplerate=model.samplerate)

        tasks[task_id].update(
            status='done', progress=100, message='🎉 완료!',
            mr_file=str(mr_path), mr_filename=mr_path.name, mr_suffix='.wav',
            vocal_file=str(vocal_path), vocal_filename=vocal_path.name,
            original_name=stem
        )

    except Exception as e:
        tasks[task_id].update(status='error', message=f'❌ 오류: {str(e)}')
        import traceback
        traceback.print_exc()


def apply_compressor(audio: np.ndarray, sr: int) -> np.ndarray:
    """보컬 제거 후 볼륨 pumping 억제 — 부드러운 컴프레서 + 리미터"""
    board = Pedalboard([
        Compressor(threshold_db=-12.0, ratio=2.0, attack_ms=15.0, release_ms=150.0),
        Limiter(threshold_db=-1.0, release_ms=100.0),
    ])
    # pedalboard: (channels, samples) float32
    if audio.ndim == 1:
        audio_in = audio[np.newaxis, :].astype(np.float32)
        out = board(audio_in, sr)
        return out[0]
    else:
        audio_in = audio.T.astype(np.float32)  # (ch, samples)
        out = board(audio_in, sr)
        return out.T  # (samples, ch)


def pitch_shift_audio(input_path: str, output_path: str, semitones: float):
    """librosa로 피치 조절"""
    y, sr = librosa.load(input_path, sr=None, mono=False)

    if y.ndim == 1:
        y_shifted = librosa.effects.pitch_shift(y, sr=sr, n_steps=semitones)
    else:
        channels = [librosa.effects.pitch_shift(y[i], sr=sr, n_steps=semitones)
                    for i in range(y.shape[0])]
        y_shifted = np.array(channels).T  # (samples, channels)

    sf.write(output_path, y_shifted, sr)


def wav_to_mp3(wav_path: str, mp3_path: str):
    """pedalboard로 WAV → MP3 변환 (320kbps)"""
    from pedalboard.io import AudioFile
    with AudioFile(wav_path) as f:
        audio = f.read(f.frames)
        sr = f.samplerate
    with AudioFile(mp3_path, 'w', samplerate=sr,
                   num_channels=audio.shape[0], quality='320kbps') as f:
        f.write(audio)


# ── Routes ─────────────────────────────────────────────────────────────────

@app.route('/')
def index():
    return render_template('index.html')


@app.route('/upload', methods=['POST'])
def upload():
    if 'file' not in request.files:
        return jsonify({'error': '파일이 없습니다.'}), 400
    file = request.files['file']
    if not file.filename or not allowed_file(file.filename):
        return jsonify({'error': '지원하지 않는 파일 형식입니다.'}), 400

    apply_comp_str = request.form.get('apply_compression', 'true')
    apply_compression = apply_comp_str.lower() == 'true'

    task_id = str(uuid.uuid4())
    stem = Path(secure_filename(file.filename)).stem
    suffix = Path(secure_filename(file.filename)).suffix
    input_path = UPLOAD_DIR / f'{task_id}{suffix}'
    file.save(str(input_path))

    tasks[task_id] = {'status': 'queued', 'progress': 0,
                      'message': '⏳ 대기 중...', 'original_name': stem}

    t = threading.Thread(target=run_separation, args=(input_path, task_id, apply_compression))
    t.daemon = True
    t.start()

    return jsonify({'task_id': task_id})


@app.route('/status/<task_id>')
def status(task_id):
    if task_id not in tasks:
        return jsonify({'error': '없는 작업입니다.'}), 404
    return jsonify(tasks[task_id])


@app.route('/pitch/<task_id>', methods=['POST'])
def pitch(task_id):
    if task_id not in tasks or tasks[task_id].get('status') not in ('done', 'sheet_done'):
        return jsonify({'error': '분리가 완료되지 않았습니다.'}), 400

    data = request.get_json() or {}
    semitones = float(data.get('semitones', 0))
    fmt = data.get('fmt', 'wav')  # 'wav' or 'mp3'
    mr_file = tasks[task_id].get('mr_file')
    if not mr_file or not Path(mr_file).exists():
        return jsonify({'error': 'MR 파일을 찾을 수 없습니다.'}), 404

    name = tasks[task_id].get('original_name', 'audio')
    sign = f"+{int(semitones)}" if semitones > 0 else str(int(semitones))
    out_dir = OUTPUT_DIR / task_id

    try:
        if semitones != 0:
            wav_out = str(out_dir / f'MR_pitch_{semitones:+.0f}.wav')
            pitch_shift_audio(mr_file, wav_out, semitones)
        else:
            wav_out = mr_file

        if fmt == 'mp3':
            mp3_out = str(out_dir / f'MR_pitch_{semitones:+.0f}.mp3')
            wav_to_mp3(wav_out, mp3_out)
            dl_name = f"MR_{name}_key{sign}.mp3" if semitones != 0 else f"MR_{name}_original.mp3"
            return send_file(mp3_out, as_attachment=True, download_name=dl_name)
        else:
            dl_name = f"MR_{name}_key{sign}.wav" if semitones != 0 else f"MR_{name}_original.wav"
            return send_file(wav_out, as_attachment=True, download_name=dl_name)
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/download/<task_id>/<track>')
def download(task_id, track):
    if task_id not in tasks or tasks[task_id].get('status') not in ('done', 'sheet_done'):
        return jsonify({'error': '없는 작업이거나 처리가 완료되지 않았습니다.'}), 404
    if track not in ('mr', 'vocal'):
        return jsonify({'error': '잘못된 트랙 요청입니다.'}), 400

    key = 'mr_file' if track == 'mr' else 'vocal_file'
    fpath = tasks[task_id].get(key)
    if not fpath or not Path(fpath).exists():
        return jsonify({'error': '파일을 찾을 수 없습니다.'}), 404

    fmt = request.args.get('fmt', 'wav')
    if fmt == 'mp3':
        mp3_path = str(Path(fpath).with_suffix('.mp3'))
        if not Path(mp3_path).exists():
            wav_to_mp3(fpath, mp3_path)
        return send_file(mp3_path, as_attachment=True,
                         download_name=Path(mp3_path).name)
    return send_file(fpath, as_attachment=True, download_name=Path(fpath).name)


@app.route('/preview/<task_id>/<track>')
def preview(task_id, track):
    if task_id not in tasks or tasks[task_id].get('status') not in ('done', 'sheet_done'):
        return jsonify({'error': '없는 작업이거나 처리가 완료되지 않았습니다.'}), 404
    if track not in ('mr', 'vocal'):
        return jsonify({'error': '잘못된 트랙 요청입니다.'}), 400

    key = 'mr_file' if track == 'mr' else 'vocal_file'
    fpath = tasks[task_id].get(key)
    if not fpath or not Path(fpath).exists():
        return jsonify({'error': '파일을 찾을 수 없습니다.'}), 404
    return send_file(fpath)


# ── Sheet Music ─────────────────────────────────────────────────────────────

def find_musescore():
    """MuseScore 실행 경로 탐색"""
    candidates = [
        '/Applications/MuseScore 4.app/Contents/MacOS/mscore',
        '/Applications/MuseScore 3.app/Contents/MacOS/mscore',
        'mscore', 'musescore', 'musescore4', 'mscore4',
        '/opt/homebrew/bin/mscore', '/opt/homebrew/bin/musescore',
        '/usr/local/bin/mscore', '/usr/local/bin/musescore',
        '/usr/bin/musescore',
    ]
    import shutil as sh
    for c in candidates:
        if Path(c).exists() or sh.which(c):
            return c
    return None



# ── 코드 감지 템플릿 ─────────────────────────────────────────────────────────
_CHORD_TEMPLATES = {
    'C':   [1,0,0,0,1,0,0,1,0,0,0,0], 'Cm':  [1,0,0,1,0,0,0,1,0,0,0,0],
    'C#':  [0,1,0,0,0,1,0,0,1,0,0,0], 'C#m': [0,1,0,0,1,0,0,0,1,0,0,0],
    'D':   [0,0,1,0,0,0,1,0,0,1,0,0], 'Dm':  [0,0,1,0,0,1,0,0,0,1,0,0],
    'D#':  [0,0,0,1,0,0,0,1,0,0,1,0], 'D#m': [0,0,0,1,0,0,1,0,0,0,1,0],
    'E':   [0,0,0,0,1,0,0,0,1,0,0,1], 'Em':  [0,0,0,0,1,0,0,1,0,0,0,1],
    'F':   [1,0,0,0,0,1,0,0,0,1,0,0], 'Fm':  [1,0,0,0,0,1,0,0,1,0,0,0],
    'F#':  [0,1,0,0,0,0,1,0,0,0,1,0], 'F#m': [0,1,0,0,0,0,1,0,0,1,0,0],
    'G':   [0,0,1,0,0,0,0,1,0,0,0,1], 'Gm':  [0,0,1,0,0,0,0,1,0,0,1,0],
    'G#':  [0,0,0,1,0,0,0,0,1,0,0,0], 'G#m': [0,0,0,1,0,0,0,0,1,0,0,1],
    'A':   [0,1,0,0,1,0,0,0,0,1,0,0], 'Am':  [1,0,0,0,1,0,0,0,0,1,0,0],
    'A#':  [0,0,1,0,0,1,0,0,0,0,1,0], 'A#m': [0,1,0,0,0,1,0,0,0,0,1,0],
    'B':   [0,0,0,1,0,0,1,0,0,0,0,1], 'Bm':  [0,0,1,0,0,0,1,0,0,0,0,1],
}

# 코드별 오블리가토 보이싱 (MIDI 염수)
_CHORD_VOICINGS = {
    'C':   [60,64,67], 'Cm':  [60,63,67], 'C#':  [61,65,68], 'C#m': [61,64,68],
    'D':   [62,66,69], 'Dm':  [62,65,69], 'D#':  [63,67,70], 'D#m': [63,66,70],
    'E':   [64,68,71], 'Em':  [64,67,71], 'F':   [65,69,72], 'Fm':  [65,68,72],
    'F#':  [66,70,73], 'F#m': [66,69,73], 'G':   [55,59,62], 'Gm':  [55,58,62],
    'G#':  [56,60,63], 'G#m': [56,59,63], 'A':   [57,61,64], 'Am':  [57,60,64],
    'A#':  [58,62,65], 'A#m': [58,61,65], 'B':   [59,63,66], 'Bm':  [59,62,66],
}


def _match_chord(chroma_vec: np.ndarray) -> str:
    """크로마 벡터 → 가장 유사한 코드명"""
    best, best_score = 'C', -1.0
    for name, tmpl in _CHORD_TEMPLATES.items():
        score = float(np.dot(chroma_vec, tmpl))
        if score > best_score:
            best_score, best = score, name
    return best


def detect_chords_per_measure(audio_path: str):
    """librosa 크로마로 마디별 코드 감지, (chords_list, bpm) 반환"""
    y, sr = librosa.load(audio_path, sr=22050, mono=True)
    tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
    bpm = float(tempo)
    if bpm < 40:  bpm = 80.0
    if bpm > 240: bpm = 120.0

    measure_secs = 4 * 60.0 / bpm
    duration = librosa.get_duration(y=y, sr=sr)
    n_measures = max(1, int(duration / measure_secs))

    chroma = librosa.feature.chroma_cqt(y=y, sr=sr)
    chords = []
    for i in range(n_measures):
        fs = librosa.time_to_frames(i * measure_secs, sr=sr)
        fe = librosa.time_to_frames((i + 1) * measure_secs, sr=sr)
        fe = min(fe, chroma.shape[1])
        if fs >= chroma.shape[1]: break
        vec = chroma[:, fs:fe].mean(axis=1)
        chords.append({'measure': i, 'offset': float(i * 4), 'chord': _match_chord(vec)})
    return chords, bpm


def extract_melody_from_midi(midi_path: str, bpm: float):
    """미디 파일에서 멘로디 단음 추출 (8분음표 기반 양자화)"""
    import music21 as m21
    GRID = 0.5  # 8분음표 = 0.5 사분음표

    raw = m21.converter.parse(midi_path)
    all_notes = []
    for part in raw.parts:
        for el in part.flatten().notes:
            if isinstance(el, m21.note.Note):
                all_notes.append((float(el.offset), el.pitch.midi, float(el.duration.quarterLength)))
            elif isinstance(el, m21.chord.Chord):
                top = max(el.notes, key=lambda n: n.pitch.midi)
                all_notes.append((float(el.offset), top.pitch.midi, float(el.duration.quarterLength)))

    if not all_notes:
        return []

    # 양자화: 8분음표 그리드로 스냅
    snapped = {}
    for offset, pitch, dur in all_notes:
        slot = round(offset / GRID) * GRID
        if slot not in snapped or pitch > snapped[slot][0]:
            snapped[slot] = (pitch, max(GRID, round(dur / GRID) * GRID))

    melody = sorted([(slot, p, d) for slot, (p, d) in snapped.items()])
    # 중복 오프셋 제거 / 누락 슬롯 Rest 채우기
    result = []
    prev_end = 0.0
    for offset, pitch, dur in melody:
        if offset > prev_end + GRID:
            result.append(('rest', offset - prev_end, prev_end))
        result.append(('note', pitch, offset, min(dur, GRID * 4)))
        prev_end = offset + min(dur, GRID * 4)
    return result


def build_two_part_score_calibrated(calibrated_notes, calibrated_chords, bpm):
    """보정된 JSON 노츠 및 코드를 바탕으로 music21 악보(Score) 생성"""
    import music21 as m21

    score = m21.stream.Score()

    # ── Part 1: Melody (주악보) ──
    p1 = m21.stream.Part()
    p1.id = 'Melody'
    p1.partName = 'Melody'
    p1.insert(0, m21.clef.TrebleClef())
    p1.insert(0, m21.meter.TimeSignature('4/4'))
    p1.insert(0, m21.tempo.MetronomeMark(number=int(bpm)))

    # 코드 심볼 추가
    for c in calibrated_chords:
        try:
            cs = m21.harmony.ChordSymbol(c['chord'])
            p1.insert(c['offset'], cs)
        except Exception:
            pass

    # 멜로디 음표 추가 (공백에 Rest 자동 생성)
    notes = sorted(calibrated_notes, key=lambda x: x['start'])
    prev_end = 0.0
    for item in notes:
        start = float(item['start'])
        dur = float(item['duration'])
        pitch = int(item['pitch'])

        # 음표 사이에 간격이 있을 경우 쉼표 추가
        if start > prev_end + 0.05:
            r = m21.note.Rest(quarterLength=start - prev_end)
            p1.insert(prev_end, r)

        n = m21.note.Note()
        n.pitch.midi = pitch
        n.duration.quarterLength = dur
        p1.insert(start, n)
        prev_end = start + dur

    # ── Part 2: Obbligato (보조악보) ──
    p2 = m21.stream.Part()
    p2.id = 'Obbligato'
    p2.partName = 'Obbligato'
    p2.insert(0, m21.clef.TrebleClef())
    p2.insert(0, m21.meter.TimeSignature('4/4'))

    for c in calibrated_chords:
        voicing = _CHORD_VOICINGS.get(c['chord'], [60, 64, 67])
        pitches = [m21.pitch.Pitch(midi=p) for p in voicing]
        # 반음표 2개로 한 마디 채우기
        ch1 = m21.chord.Chord(pitches, quarterLength=2.0)
        ch2 = m21.chord.Chord(pitches, quarterLength=2.0)
        p2.insert(c['offset'], ch1)
        p2.insert(c['offset'] + 2.0, ch2)

    score.insert(0, p1)
    score.insert(0, p2)
    score.makeNotation(inPlace=True)
    return score


def calibrate_music_data_with_gemini(raw_notes: list, raw_chords: list, user_prompt: str, api_key: str) -> dict:
    """Gemini API를 사용하여 노츠와 코드를 정밀 교정"""
    import google.generativeai as genai
    import json
    import re

    genai.configure(api_key=api_key)
    model = genai.GenerativeModel('gemini-1.5-flash', generation_config={"response_mime_type": "application/json"})

    # 150개 노트 이상은 토큰 절약 및 속도 향상을 위해 절단
    truncated_notes = raw_notes[:150]

    prompt = f"""
    You are an expert music transcriber and music theorist.
    Your input consists of raw melody notes and raw measure-by-measure chords detected from an audio file.

    RAW MELODY NOTES:
    {json.dumps(truncated_notes)}

    RAW CHORDS:
    {json.dumps(raw_chords)}

    USER STYLE PROMPT:
    {user_prompt}

    Tasks:
    1. Determine the key of the song (e.g., C Major, A Minor).
    2. Filter out "ghost notes" (spurious notes with duration < 0.18s that do not fit the main melody line).
    3. Align/quantize note pitches to fit the detected key (correcting out-of-key sharps/flats unless they are chromatic passing notes).
    4. Regularize chord progression: substitute weird chords with standard diatonic progressions fitting the detected key (e.g., if key is C Major, change G#m to Am or G).
    5. Adjust behavior based on USER STYLE PROMPT if provided.

    Return STRICTLY a JSON object matching the following structure:
    {{
      "key": "Detected Key (e.g., C Major)",
      "notes": [
        {{"pitch": 60, "start": 0.5, "duration": 0.5}}
      ],
      "chords": [
        {{"measure": 0, "offset": 0.0, "chord": "C"}}
      ]
    }}
    """
    response = model.generate_content(prompt)
    text = response.text.strip()
    if text.startswith('```json'):
        text = text[7:]
    elif text.startswith('```'):
        text = text[3:]
    if text.endswith('```'):
        text = text[:-3]
    text = text.strip()

    # JSON 블록 정밀 추출
    match = re.search(r'\{.*\}', text, re.DOTALL)
    if match:
        text = match.group(0)

    return json.loads(text)


def calibrate_music_data_rules(raw_notes: list, raw_chords: list) -> dict:
    """기본 룰 기반 노이즈 필터링 (Gemini API 키가 없을 때 작동)"""
    # 0.18초 미만의 짧은 음은 노이즈로 보고 제외 (고스트 노트)
    filtered_notes = [n for n in raw_notes if n['duration'] >= 0.18]
    return {
        "key": "Auto (Rule-based)",
        "notes": filtered_notes,
        "chords": raw_chords
    }


def audio_to_sheet_hybrid(melody_path: str, chord_path: str, out_dir: Path, label: str, prompt: str = '', api_key: str = '') -> dict:
    """오디오 → 주선율(melody_path) + 코드감지(chord_path) → MuseScore PDF"""
    from basic_pitch.inference import predict
    from basic_pitch import ICASSP_2022_MODEL_PATH
    import music21 as m21
    import subprocess as sp

    midi_path = str(out_dir / f'{label}.mid')
    xml_path  = str(out_dir / f'{label}.musicxml')
    pdf_path  = str(out_dir / f'{label}.pdf')

    # ① basic-pitch: Run melody extraction on melody_path (vocals)
    _, midi_data, _ = predict(melody_path, ICASSP_2022_MODEL_PATH)
    midi_data.write(midi_path)

    # ② chord detection: Run beat-tracking/chroma on chord_path (MR)
    chords, bpm = detect_chords_per_measure(chord_path)

    # ③ 멜로디 추출 + 양자화
    melody_seq = extract_melody_from_midi(midi_path, bpm)
    notes_json = [
        {'pitch': item[1], 'start': item[2], 'duration': item[3]}
        for item in melody_seq if item[0] == 'note'
    ]

    detected_key = "Auto"
    # ④ Gemini AI 보정 적용
    if api_key:
        try:
            print("🚀 Gemini AI 보정을 시작합니다...")
            calibrated = calibrate_music_data_with_gemini(notes_json, chords, prompt, api_key)
            notes_json = calibrated.get('notes', notes_json)
            chords = calibrated.get('chords', chords)
            detected_key = calibrated.get('key', 'Auto (Gemini)')
            print(f"✅ Gemini AI 보정 완료 (Key: {detected_key})")
        except Exception as e:
            print("❌ AI Calibration failed, falling back to rule-based:", e)
            calibrated = calibrate_music_data_rules(notes_json, chords)
            notes_json = calibrated['notes']
            chords = calibrated['chords']
            detected_key = calibrated.get('key', 'Auto (Rule-based)')
    else:
        print("ℹ️ Gemini API Key가 없어 로컬 룰 기반 보정을 적용합니다.")
        calibrated = calibrate_music_data_rules(notes_json, chords)
        notes_json = calibrated['notes']
        chords = calibrated['chords']
        detected_key = calibrated.get('key', 'Auto (Rule-based)')

    # ⑤ music21 두 파트 악보 생성
    score = build_two_part_score_calibrated(notes_json, chords, bpm)

    # ⑥ MusicXML 저장
    score.write('musicxml', fp=xml_path)

    # ⑦ MuseScore → PDF
    ms_bin = find_musescore()
    if ms_bin:
        sp.run([ms_bin, '-o', pdf_path, xml_path],
               capture_output=True, timeout=180)

    return {
        'midi_path': midi_path,
        'xml_path':  xml_path,
        'pdf_path':  pdf_path if Path(pdf_path).exists() else None,
        'notes':     notes_json,
        'chords':    chords,
        'bpm':       bpm,
        'key':       detected_key,
    }


def run_transcribe(vocal_path: str, mr_path: str, out_dir: Path, task_id: str, label: str, prompt: str, api_key: str):
    """백그라운드 악보 생성 작업"""
    try:
        tasks[task_id].update(
            status='transcribing', progress=10,
            message='🎼 음표를 분석하고 있습니다...'
        )
        result = audio_to_sheet_hybrid(vocal_path, mr_path, out_dir, label, prompt, api_key)
        tasks[task_id].update(
            status='sheet_done', progress=100,
            message='🎉 악보 생성 완료!',
            midi_path=result['midi_path'],
            pdf_path=result['pdf_path'],
            notes=result['notes'],
            chords=result['chords'],
            bpm=result['bpm'],
            key=result['key'],
        )
    except Exception as e:
        tasks[task_id].update(status='error', message=f'❌ 악보 오류: {e}')
        import traceback; traceback.print_exc()


def run_transcribe_direct(input_path: Path, out_dir: Path, task_id: str, label: str, prompt: str, api_key: str):
    """직접 업로드 파일 → 보컬/MR 임시 분리 후 하이브리드 악보 생성"""
    try:
        tasks[task_id].update(status='transcribing', progress=10, message='🎵 음원 분리 중... (이 단계가 끝나면 악보를 생성합니다)')

        # Demucs 분리 실행 (음량 펌핑 방지는 필요 없음, 악보 생성용이므로 가벼운 세팅)
        import torch
        from demucs.pretrained import get_model
        from demucs.apply import apply_model
        from demucs.audio import AudioFile, save_audio

        device = get_device()
        model = get_model('htdemucs')
        model.eval()

        wav = AudioFile(input_path).read(
            streams=0,
            samplerate=model.samplerate,
            channels=model.audio_channels
        )
        ref = wav.mean(0)
        wav = (wav - ref.mean()) / ref.std()

        with torch.no_grad():
            sources = apply_model(model, wav[None], device=device,
                                  shifts=0, split=True, overlap=0.1,
                                  progress=False)[0]
        sources = sources * ref.std() + ref.mean()

        source_names = model.sources
        vocal_idx = source_names.index('vocals')
        mr_tensor = sum(sources[i] for i, s in enumerate(source_names) if s != 'vocals')
        vocal_tensor = sources[vocal_idx]

        mr_path = out_dir / f'temp_mr.wav'
        vocal_path = out_dir / f'temp_vocals.wav'

        save_audio(mr_tensor, str(mr_path), samplerate=model.samplerate)
        save_audio(vocal_tensor, str(vocal_path), samplerate=model.samplerate)

        tasks[task_id].update(progress=50, message='🎼 음표 및 코드 감지 중...')

        result = audio_to_sheet_hybrid(str(vocal_path), str(mr_path), out_dir, label, prompt, api_key)

        tasks[task_id].update(
            status='sheet_done', progress=100,
            message='🎉 악보 생성 완료!',
            midi_path=result['midi_path'],
            pdf_path=result['pdf_path'],
            notes=result['notes'],
            chords=result['chords'],
            bpm=result['bpm'],
            key=result['key'],
        )

    except Exception as e:
        tasks[task_id].update(status='error', message=f'❌ 악보 오류: {e}')
        import traceback; traceback.print_exc()


@app.route('/transcribe/<task_id>/<track>', methods=['POST'])
def transcribe(task_id, track):
    """분리된 트랙(보컬/MR)으로 악보 생성"""
    if task_id not in tasks or tasks[task_id].get('status') not in ('done', 'sheet_done'):
        return jsonify({'error': 'MR 분리가 완료되지 않았습니다.'}), 400

    vocal_path = tasks[task_id].get('vocal_file')
    mr_path = tasks[task_id].get('mr_file')
    if not vocal_path or not mr_path or not Path(vocal_path).exists() or not Path(mr_path).exists():
        return jsonify({'error': '오디오 파일을 찾을 수 없습니다.'}), 404

    out_dir = OUTPUT_DIR / task_id
    label = f'sheet_{track}'

    # 이미 생성된 결과 반환
    if tasks[task_id].get('status') == 'sheet_done':
        return jsonify({'status': 'sheet_done', 'task_id': task_id})

    # Extract user prompt and api key
    data = request.get_json() or {}
    prompt = data.get('calibration_prompt', '')
    api_key = request.headers.get('X-Gemini-API-Key') or os.environ.get('GEMINI_API_KEY', '')

    tasks[task_id].update(status='transcribing', progress=5,
                          message='🎼 악보 생성 준비 중...')
    t = threading.Thread(target=run_transcribe,
                         args=(vocal_path, mr_path, out_dir, task_id, label, prompt, api_key))
    t.daemon = True
    t.start()
    return jsonify({'status': 'started', 'task_id': task_id})


@app.route('/transcribe/direct', methods=['POST'])
def transcribe_direct():
    """파일 업로드 → 바로 악보 생성 (MR 분리 없이)"""
    if 'file' not in request.files:
        return jsonify({'error': '파일이 없습니다.'}), 400
    file = request.files['file']
    if not file.filename or not allowed_file(file.filename):
        return jsonify({'error': '지원하지 않는 형식입니다.'}), 400

    prompt = request.form.get('calibration_prompt', '')
    api_key = request.headers.get('X-Gemini-API-Key') or os.environ.get('GEMINI_API_KEY', '')

    task_id = str(uuid.uuid4())
    suffix = Path(secure_filename(file.filename)).suffix
    stem = Path(secure_filename(file.filename)).stem
    input_path = UPLOAD_DIR / f'{task_id}{suffix}'
    file.save(str(input_path))

    out_dir = OUTPUT_DIR / task_id
    out_dir.mkdir(exist_ok=True)

    tasks[task_id] = {
        'status': 'transcribing', 'progress': 5,
        'message': '🎼 악보 생성 준비 중...', 'original_name': stem
    }
    t = threading.Thread(target=run_transcribe_direct,
                         args=(input_path, out_dir, task_id, 'sheet_direct', prompt, api_key))
    t.daemon = True
    t.start()
    return jsonify({'task_id': task_id})


@app.route('/download/<task_id>/midi')
def download_midi(task_id):
    if task_id not in tasks:
        return jsonify({'error': '없는 작업입니다.'}), 404
    fpath = tasks[task_id].get('midi_path')
    if not fpath or not Path(fpath).exists():
        return jsonify({'error': 'MIDI 파일이 없습니다.'}), 404
    name = tasks[task_id].get('original_name', 'sheet')
    return send_file(fpath, as_attachment=True, download_name=f'{name}.mid')


@app.route('/download/<task_id>/pdf')
def download_pdf(task_id):
    if task_id not in tasks:
        return jsonify({'error': '없는 작업입니다.'}), 404
    fpath = tasks[task_id].get('pdf_path')
    if not fpath or not Path(fpath).exists():
        return jsonify({'error': 'PDF 파일이 없습니다.'}), 404
    name = tasks[task_id].get('original_name', 'sheet')
    return send_file(fpath, as_attachment=True, download_name=f'{name}.pdf')


@app.route('/view/<task_id>/pdf')
def view_pdf(task_id):
    """PDF를 브라우저 인라인으로 표시"""
    if task_id not in tasks:
        return jsonify({'error': '없는 작업입니다.'}), 404
    fpath = tasks[task_id].get('pdf_path')
    if not fpath or not Path(fpath).exists():
        return jsonify({'error': 'PDF 파일이 없습니다.'}), 404
    return send_file(fpath, mimetype='application/pdf')


if __name__ == '__main__':
    print("🎵 MR 분리기 서버 시작!")
    print("👉 http://localhost:5001 을 브라우저에서 열어주세요")
    app.run(debug=False, host='0.0.0.0', port=5001)
