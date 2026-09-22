@echo off
cd /d "%~dp0"
py -3 -m venv .venv
if errorlevel 1 goto fail
".venv\Scripts\python.exe" -m pip install -r requirements.txt
if errorlevel 1 goto fail
echo Installation complete. Run watch.bat before connecting the Feather.
pause
exit /b 0
:fail
echo Installation failed. Install Python 3.10 or newer with the Python launcher, then retry.
pause
exit /b 1
