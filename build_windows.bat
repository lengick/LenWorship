@echo off
REM ============================================================
REM  Compila Len-Worship (VST3) en Windows.
REM  Requisitos: Visual Studio 2022 (carga de trabajo "Desarrollo de
REM  escritorio con C++"), CMake y Git instalados.
REM  Ejecutalo con doble clic desde esta carpeta.
REM ============================================================

cmake -B build -A x64
if errorlevel 1 goto error

cmake --build build --config Release --target LenWorship_VST3 --parallel
if errorlevel 1 goto error

echo.
echo ============================================================
echo  LISTO. Tu plugin esta en:
echo  %cd%\build\LenWorship_artefacts\Release\VST3\Len-Worship.vst3
echo.
echo  Copia la carpeta "Len-Worship.vst3" a:
echo  C:\Program Files\Common Files\VST3
echo  y en FL Studio busca plugins nuevos.
echo ============================================================
pause
exit /b 0

:error
echo.
echo Algo fallo durante la compilacion. Revisa los mensajes de arriba.
pause
exit /b 1
