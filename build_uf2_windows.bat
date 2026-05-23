@echo off
setlocal
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake bulunamadi. GitHub Actions ile derlemek daha kolay.
  pause
  exit /b 1
)
if "%PICO_SDK_PATH%"=="" (
  echo PICO_SDK_PATH ayari yok. GitHub Actions ile derlemek daha kolay.
  pause
  exit /b 1
)
mkdir build 2>nul
cd build
cmake .. -G "NMake Makefiles"
cmake --build .
echo.
echo UF2 burada olmali: build\pico_absolute_mouse.uf2
pause
