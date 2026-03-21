@echo off
REM ESP32 Firmware Merge Script
REM Merge bootloader, partition table and application into single bin file

echo Merging firmware...
echo.

REM Set ESP-IDF environment paths
set IDF_PYTHON=D:\JianHao\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe
set IDF_PATH=D:\JianHao\Espressif\frameworks\esp-idf-v5.5.1

REM Check if ESP-IDF Python exists
if not exist "%IDF_PYTHON%" (
    echo ERROR: ESP-IDF Python not found
    echo Path: %IDF_PYTHON%
    echo Please check your ESP-IDF installation
    echo.
    pause
    exit /b 1
)

REM Check if project is built
if not exist ".pio\build\release\.sconsign311.dblite" (
    echo ERROR: Build database not found
    echo Please build the project first: pio run -e release
    echo.
    pause
    exit /b 1
)

REM Auto-detect framework path from build output
echo Detecting framework path from build output...
for /f "delims=" %%i in ('strings .pio\build\release\.sconsign311.dblite 2^>nul ^| findstr /C:"framework-arduinoespressif32" 2^>nul ^| findstr /C:"tools" 2^>nul ^| head -1 2^>nul') do (
    set FRAMEWORK_LINE=%%i
)

if not defined FRAMEWORK_LINE (
    echo ERROR: Could not detect framework path from build output
    echo Please ensure the project is built: pio run -e release
    echo.
    pause
    exit /b 1
)

REM Extract framework directory path
REM Example: C:/Users/admin/.platformio/packages/framework-arduinoespressif32@3.10004.210126/tools/...
REM We need: C:\Users\admin\.platformio\packages\framework-arduinoespressif32@3.10004.210126

REM Use PowerShell to extract the path with regex
for /f "delims=" %%i in ('powershell -Command "$line='%FRAMEWORK_LINE%'; if($line -match 'C:/Users/[^/]+/\.platformio/packages/framework-arduinoespressif32[^/]*') { $matches[0] -replace '/','\' }"') do (
    set FRAMEWORK_DIR=%%i
)

if not defined FRAMEWORK_DIR (
    echo ERROR: Failed to extract framework path
    echo Raw line: %FRAMEWORK_LINE%
    echo.
    pause
    exit /b 1
)

echo Found framework: %FRAMEWORK_DIR%
echo.

REM Set file paths using detected framework
set BOOTLOADER=%FRAMEWORK_DIR%\tools\sdk\bin\bootloader_dio_80m.bin
set PARTITIONS=.pio\build\release\partitions.bin
set BOOT_APP0=%FRAMEWORK_DIR%\tools\partitions\boot_app0.bin
set FIRMWARE=.pio\build\release\firmware.bin
set OUTPUT=.pio\build\release\firmware_merged.bin

REM Check if files exist
if not exist "%BOOTLOADER%" (
    echo ERROR: Bootloader file not found
    echo Path: %BOOTLOADER%
    echo.
    pause
    exit /b 1
)

if not exist "%PARTITIONS%" (
    echo ERROR: Partition table not found, please build project first
    echo.
    pause
    exit /b 1
)

if not exist "%BOOT_APP0%" (
    echo ERROR: boot_app0.bin not found
    echo Path: %BOOT_APP0%
    echo.
    pause
    exit /b 1
)

if not exist "%FIRMWARE%" (
    echo ERROR: Firmware file not found, please build project first
    echo.
    pause
    exit /b 1
)

REM Merge using esptool.py from ESP-IDF
echo Running esptool merge_bin...
echo Using Python: %IDF_PYTHON%
echo.
echo Merging 4 files:
echo   0x1000  %BOOTLOADER%
echo   0x8000  %PARTITIONS%
echo   0xe000  %BOOT_APP0%
echo   0x10000 %FIRMWARE%
echo.
"%IDF_PYTHON%" -m esptool --chip esp32 merge_bin -o "%OUTPUT%" --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 "%BOOTLOADER%" 0x8000 "%PARTITIONS%" 0xe000 "%BOOT_APP0%" 0x10000 "%FIRMWARE%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo SUCCESS! Firmware merged successfully
    echo Output file: %OUTPUT%
    echo ========================================
    echo.
    echo Flash address: 0x0
    echo File size:
    dir "%OUTPUT%" | findstr firmware_merged.bin
    echo.
) else (
    echo.
    echo ========================================
    echo FAILED! Please check if esptool is installed
    echo Install command: pip install esptool
    echo ========================================
    echo.
)

pause
