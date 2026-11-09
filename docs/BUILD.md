# 빌드 안내

필요한 것은 C 컴파일러와 CMake뿐입니다. 처음 빌드할 때 raylib 5.5 소스를 인터넷에서 내려받습니다(SHA-256으로 확인).

## macOS (Apple Silicon)

Apple Command Line Tools와 CMake 3.24 이상이 필요합니다.

```sh
./scripts/build-macos.sh
./build/macos-arm64/bin/BubbleBobble
```

스크립트는 설정 → 빌드 → 테스트 순서로 실행하고, 하나라도 실패하면 멈춥니다.
경로에 `:` 같은 특수문자가 들어간 폴더 안에서는 빌드가 실패할 수 있으니 폴더 이름을 단순하게 두세요.

## Windows (Visual Studio 2026)

Desktop development with C++ 워크로드와 CMake 4.2 이상이 필요합니다. PowerShell에서 실행합니다.

```powershell
./scripts/build-windows.ps1
./build/windows-x64/bin/Release/BubbleBobble.exe
```

## 폴더 구조

| 위치 | 내용 |
|---|---|
| `src/` | 게임 코드. `bubble.h`와 `.c` 10개 (README 참고) |
| `tests/` | 창 없이 돌리는 규칙 테스트 4개 (`game`, `levels`, `viewport`, `score`) |
| `Assets/` | 그림·소리 파일. 빌드하면 실행 파일 옆 `assets` 폴더로 복사됩니다 |
| `scripts/` | 빌드 스크립트 |

게임은 실행 파일 옆의 `assets` 폴더에서 그림과 소리를 읽고, 순위표를 같은 위치의 `Scores.txt`에 저장합니다.
실행 파일을 다른 곳으로 옮길 때는 `assets` 폴더도 함께 옮기세요.

## 테스트만 다시 실행

```sh
ctest --preset macos-arm64
```
