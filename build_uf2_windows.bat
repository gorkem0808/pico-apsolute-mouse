@echo off
echo GT Arcade Pico Pro V3 local build
if "%PICO_SDK_PATH%"=="" (
  echo HATA: PICO_SDK_PATH ayarli degil. En kolay yol GitHub Actions kullanmak.
  pause
  exit /b 1
)
if not exist build mkdir build
cd build
cmake .. -DPICO_SDK_PATH=%PICO_SDK_PATH%
cmake --build . -j 2
pause
