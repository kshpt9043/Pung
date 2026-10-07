@echo off
chcp 65001 >nul
setlocal

REM ============================================================
REM  Pung - 자동 대전 (봇끼리 매치 반복 + 기록 CSV)
REM
REM  화면 없이(-nullrhi) 봇끼리 정해진 판 수만큼 매치를 하고 스스로 종료한다.
REM  기록: Saved\Telemetry\[실행 시각]\ (knockbacks / falls / items / matches .csv)
REM  진행 상황은 같이 뜨는 로그 창에서 [자동 대전], [기록] 으로 확인한다.
REM  먼저 Build-Editor.bat 으로 빌드해 두어야 한다.
REM
REM  사용법:
REM    Run-AutoMatch.bat                              기본: Default 2 + Smart 2, 10판
REM    Run-AutoMatch.bat "Default:3,Smart:3" 20       봇 구성과 판 수 지정
REM    Run-AutoMatch.bat "Smart:4" 50 120 2           + 매치 시간(초, 0=기본), 게임 속도 배율
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

REM --- 자동 대전 옵션 (인자가 없으면 기본값) ---
set "BOTS=Default:2,Smart:2"
set "MATCHES=10"
set "DURATION=0"
set "TIMESCALE=1"
if not "%~1"=="" set "BOTS=%~1"
if not "%~2"=="" set "MATCHES=%~2"
if not "%~3"=="" set "DURATION=%~3"
if not "%~4"=="" set "TIMESCALE=%~4"

REM 프로젝트 경로는 이 배치파일이 있는 폴더에서 자동으로 찾는다
set "PROJECT=%~dp0Pung.uproject"

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

echo Pung 자동 대전: 봇 %BOTS% / %MATCHES%판 / 매치 %DURATION%초(0=기본) / 속도 x%TIMESCALE%
start "" "%ENGINE%" "%PROJECT%" -game -nullrhi -nosound -nosteam -unattended -log -PungAutoMatch -PungBots=%BOTS% -PungMatches=%MATCHES% -PungMatchDuration=%DURATION% -PungTimeScale=%TIMESCALE%

endlocal
