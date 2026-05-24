@echo off
setlocal
cd /d "%~dp0..\src\ORIScore.ESPsim.Runner"
dotnet run -- --port 18088
