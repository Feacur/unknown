@echo off
setlocal
cd /d "%~dp0"
chcp 65001 > nul

rem unpack arguments
for %%a in (%*) do (
	for /f "tokens=1,2 delims=:" %%b IN ("%%a") do (
		if [%%c] == [] (set %%b=true) else (set %%b=%%c)
	)
)

rem prepare
if [%toolset%] == [] set toolset=remedybg
if [%target%]  == [] set target=main.exe
if not exist "build/%target%" (exit /b 1)

rem debug
call :debug_%toolset%

endlocal

rem declare functions
goto :eof

:debug_remedybg
	where -q "remedybg.exe" || (
		echo.please, install "RemedyBG", https://remedybg.itch.io/remedybg
		exit /b 1
	)

	set dbg=start /d "build" /b /wait remedybg -g -q

	echo.[debug with remedybg]
	%dbg% "%target%"
	echo.[remedybg debug ended]
goto :eof

:debug_raddbg
	where -q "raddbg.exe" || (
		echo.please, install "RAD Debugger", https://github.com/EpicGamesExt/raddebugger/releases
		exit /b 1
	)

	set dbg=start /d "build" /b /wait raddbg --auto_run

	echo.[debug with raddbg]
	%dbg% "%target%"
	echo.[raddbg debug ended]
goto :eof
