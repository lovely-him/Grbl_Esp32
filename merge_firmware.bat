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

REM Set file paths
set BOOTLOADER=%USERPROFILE%\.platformio\packages\framework-arduinoespressif32\tools\sdk\bin\bootloader_dio_80m.bin
set PARTITIONS=.pio\build\release\partitions.bin
set BOOT_APP0=%USERPROFILE%\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin
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
echo   0x1000  bootloader_dio_80m.bin (DIO mode, 80MHz)
echo   0x8000  partitions.bin
echo   0xe000  boot_app0.bin (OTA selector)
echo   0x10000 firmware.bin
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
