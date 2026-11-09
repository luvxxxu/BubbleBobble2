# Windows에서 게임을 빌드하고 테스트까지 실행한다. 하나라도 실패하면 멈춘다.
$ErrorActionPreference = "Stop"
# 스크립트가 있는 폴더의 한 단계 위(프로젝트 폴더)에서 실행한다.
Set-Location (Split-Path -Parent $PSScriptRoot)
cmake --preset windows-x64
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }
cmake --build --preset windows-x64
if ($LASTEXITCODE -ne 0) { throw "Compilation failed" }
ctest --preset windows-x64
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
Write-Host "실행: build/windows-x64/bin/Release/BubbleBobble.exe"
