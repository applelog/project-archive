#!/bin/bash
# MR 분리기 설치 스크립트

echo "🎵 MR 분리기 설치를 시작합니다..."
echo ""

# Python 버전 확인
python3 --version || { echo "❌ Python3이 필요합니다."; exit 1; }

# 가상환경 생성
echo "📦 가상환경 생성 중..."
python3 -m venv venv

# 가상환경 활성화
source venv/bin/activate

# pip 업그레이드
pip install --upgrade pip -q

echo "📥 패키지 설치 중 (시간이 걸릴 수 있습니다)..."
echo ""

# PyTorch 설치 (CPU 버전 - Mac 호환)
pip install torch torchaudio --index-url https://download.pytorch.org/whl/cpu -q

# 나머지 패키지 설치
pip install flask demucs librosa soundfile numpy werkzeug pydub pedalboard basic-pitch music21 google-generativeai -q

echo ""
echo "✅ 설치 완료!"
echo ""
echo "👉 서버를 시작하려면: ./start.sh"
