#!/bin/sh
# macOS(Apple Silicon)에서 게임을 빌드하고 테스트까지 실행한다. 하나라도 실패하면 멈춘다.
set -eu
# 스크립트가 있는 폴더의 한 단계 위(프로젝트 폴더)로 이동한다.
cd "$(dirname "$0")/.."
cmake --preset macos-arm64
cmake --build --preset macos-arm64
ctest --preset macos-arm64
echo "실행: ./build/macos-arm64/bin/BubbleBobble"
