@echo off
chcp 65001 >nul
setlocal

REM ============================================================
REM  Pung - Standalone 실행 (Terminus 에서 가져옴)
REM
REM  PIE(에디터 안에서 재생)에서는 Steam OSS 가 초기화되지 않는다.
REM  세션 테스트(PungHost / PungFind / PungJoin)는 반드시 이 배치로 실행할 것.
REM  먼저 Build-Editor.bat 으로 빌드해 두어야 한다.
REM
REM  사용법:
REM    Run-Standalone.bat            일반 실행 (Steam 사용)
REM    Run-Standalone.bat nosteam    Steam 끄고 실행 (로컬 2인 접속 테스트용)
REM ============================================================

REM --- 엔진 위치. 아래 순서로 처음 찾은 곳을 쓴다 ---
REM 1. 환경 변수 UE_ENGINE_DIR (예: E:\UE_5.8\Engine)
REM 2. 에픽 런처 기본 설치 경로
REM 3. E:\UE_5.8
set "ENGINE_DIR="
if defined UE_ENGINE_DIR if exist "%UE_ENGINE_DIR%\Build\BatchFiles\Build.bat" set "ENGINE_DIR=%UE_ENGINE_DIR%"
if not defined ENGINE_DIR if exist "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "ENGINE_DIR=C:\Program Files\Epic Games\UE_5.8\Engine"
if not defined ENGINE_DIR if exist "E:\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "ENGINE_DIR=E:\UE_5.8\Engine"

if not defined ENGINE_DIR (
    echo [오류] UE 5.8 엔진을 찾을 수 없습니다.
    echo        환경 변수 UE_ENGINE_DIR 에 엔진 폴더를 넣어 주세요. 예: E:\UE_5.8\Engine
    pause
    exit /b 1
)

set "ENGINE=%ENGINE_DIR%\Binaries\Win64\UnrealEditor.exe"

REM --- 실행 옵션 ---
set "RESX=1280"
set "RESY=720"
set "MAXFPS=60"

REM 프로젝트 경로는 이 배치파일이 있는 폴더에서 자동으로 찾는다
set "PROJECT=%~dp0Pung.uproject"

REM nosteam 인자를 주면 Steam OSS 를 끄고 IP 넷드라이버로 떨어진다
set "EXTRA="
if /I "%~1"=="nosteam" set "EXTRA=-nosteam"

if not exist "%ENGINE%" (
    echo [오류] 에디터 실행 파일을 찾을 수 없습니다.
    echo        %ENGINE%
    pause
    exit /b 1
)

if not exist "%PROJECT%" (
    echo [오류] 프로젝트를 찾을 수 없습니다: %PROJECT%
    pause
    exit /b 1
)

echo Pung 실행: %RESX%x%RESY% / %MAXFPS%fps / 창모드 %EXTRA%
start "" "%ENGINE%" "%PROJECT%" -game -log -windowed -ResX=%RESX% -ResY=%RESY% -ExecCmds="t.MaxFPS %MAXFPS%" %EXTRA%

endlocal
