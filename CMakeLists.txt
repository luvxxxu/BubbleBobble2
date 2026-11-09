# 빌드 설정 파일이다. 어떤 소스 파일을 합쳐 실행 파일을 만들지 CMake에게 알려 준다.
cmake_minimum_required(VERSION 3.24)
project(BubbleBobble LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# 실행 파일은 빌드 폴더의 bin 폴더에 모은다.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 게임 화면, 소리, 키 입력을 담당하는 raylib 5.5를 내려받아 함께 빌드한다.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(raylib)

# 게임 규칙 파일(테스트에서도 그대로 쓴다)과 게임 실행 파일
set(CORE_SOURCES src/game.c src/player.c src/enemy.c src/level.c src/world.c src/score.c)
# 화면과 입력을 담당하는 파일(main.c, render.c, scene.c, ui.c)은 실행 파일에만 넣는다.
add_executable(BubbleBobble src/main.c src/render.c src/scene.c src/ui.c ${CORE_SOURCES})
target_include_directories(BubbleBobble PRIVATE src)
target_link_libraries(BubbleBobble PRIVATE raylib)
if(MSVC)
    target_compile_options(BubbleBobble PRIVATE /utf-8)
else()
    target_link_libraries(BubbleBobble PRIVATE m)           # 수학 함수 라이브러리
    target_compile_options(BubbleBobble PRIVATE -Wall -Wextra)
endif()

# 그림과 소리 파일(Assets 폴더)을 실행 파일 옆의 assets 폴더로 복사한다.
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:BubbleBobble>/assets")
add_dependencies(BubbleBobble copy_assets)

# 테스트: 게임 규칙, 맵, 화면 크기 계산, 순위표 파일을 창 없이 검사한다.
include(CTest)
if(BUILD_TESTING)
    foreach(name game levels viewport score)
        add_executable(test_${name} tests/test_${name}.c ${CORE_SOURCES})
        target_include_directories(test_${name} PRIVATE src)
        target_link_libraries(test_${name} PRIVATE raylib)
        if(NOT MSVC)
            target_link_libraries(test_${name} PRIVATE m)
            target_compile_options(test_${name} PRIVATE -UNDEBUG)  # assert를 Release에서도 켠다.
        else()
            target_compile_options(test_${name} PRIVATE /UNDEBUG /utf-8)
        endif()
        add_test(NAME ${name} COMMAND test_${name})
    endforeach()
endif()

install(TARGETS BubbleBobble RUNTIME DESTINATION bin)
install(DIRECTORY Assets/ DESTINATION bin/assets)
