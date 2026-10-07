@echo off
chcp 65001 >nul
REM ============================================================
REM  Pung - 자동 대전 실행기 (GUI)
REM  봇 등급별 수, 판 수, 매치 시간, 게임 속도, 동시 실행 수를 창에서 고른다.
REM  Windows 기본 PowerShell 로 동작하므로 따로 설치할 것은 없다.
REM  먼저 Build-Editor.bat 으로 빌드해 두어야 한다 (또는 패키징한 exe).
REM ============================================================
start "" /min powershell -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0Tools\AutoMatchLauncher.ps1"
