#!/bin/bash
# '나의 불편함' TUI 대시보드 정리 및 삭제 스크립트

echo "🧹 TUI 대시보드 로컬 찌꺼기 파일을 정리합니다..."

# 1. 샌드박스 플러그인 폴더 삭제
if [ -d "./.sandbox_plugins" ]; then
    rm -rf "./.sandbox_plugins"
    echo "  - 로컬 샌드박스 플러그인 폴더(.sandbox_plugins) 삭제 완료"
fi

# 2. 빌드된 실행 파일 삭제
if [ -f "./my-dashboard" ]; then
    rm -f "./my-dashboard"
    echo "  - TUI 대시보드 실행 파일(my-dashboard) 삭제 완료"
fi

# 3. Go 빌드 캐시 청소
go clean
echo "  - Go 빌드 캐시 정리 완료"

echo "✨ 모든 로컬 임시 파일 및 실행 파일이 성공적으로 삭제되었습니다."
echo "프로그램을 완전히 삭제하려면 이 폴더(나의 불편함) 전체를 쓰레기통에 버리시면 됩니다."
