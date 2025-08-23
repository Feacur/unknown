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

rem prepare flags
if [%clean%]    == [] set clean=true
if [%toolset%]  == [] set toolset=clang
if [%CRT%]      == [] set CRT=static
if [%optimize%] == [] set optimize=release
if [%arch%]     == [] set arch=64

set root=%cd%
set code=%root%/code
set project=%root%/project
set temp=%root%/temp

rem prepare directories
if not exist "build" mkdir "build"
if not exist "temp"  mkdir "temp"

del "build\*" /s /q > nul
if "%clean%" == "true" (
	del "temp\*" /s /q > nul
)

rem build
call :build_%toolset%

endlocal

rem declare functions
goto :eof

:build_clang
	rem refer to `code/_project.h` for additional info

	where -q "clang.exe" || (
		set "PATH=%PATH%;C:/Program Files/LLVM/bin/"
		where -q "clang.exe" || (
			echo.please, install "Clang", https://github.com/llvm/llvm-project/releases
			exit /b 1
		)
	)

	rem @note batch
	rem https://ss64.com/nt/
	rem https://learn.microsoft.com/windows-server/administration/windows-commands

	rem @note clang / gcc
	rem https://gcc.gnu.org/onlinedocs/
	rem https://clang.llvm.org/docs/CommandGuide/clang.html

	rem @note clang's lld-link / MSVC's link
	rem https://learn.microsoft.com/cpp/build/reference/linking
	rem https://learn.microsoft.com/cpp/c-runtime-library/crt-library-features

	rem C compiler flags
	set cc=start /d "temp" /b clang -c -ansi -pedantic-errors
	set cc=%cc% -std=c99 -fno-exceptions -fno-rtti -ffp-contract=off
	set cc=%cc% -I"%code%" -I"%project%"
	set cc=%cc% -I"%code%/_external"
	set cc=%cc% -flto=thin
	if "%optimize%" == "inspect" set cc=%cc% -O0 -g -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_INSPECT -DBUILD_TARGET=BUILD_TARGET_TERMINAL
	if "%optimize%" == "develop" set cc=%cc% -Og -g -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_DEVELOP -DBUILD_TARGET=BUILD_TARGET_TERMINAL
	if "%optimize%" == "release" set cc=%cc% -O2    -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_RELEASE -DBUILD_TARGET=BUILD_TARGET_GRAPHICAL
	if "%arch%" == "32" set cc=%cc% -m32
	if "%arch%" == "64" set cc=%cc% -m64

	rem resource compiler flags
	set resc=start /d "temp" /b llvm-rc
	set resc=%resc% -nologo

	rem linker flags
	set linkd=start /d "build" /b /wait lld-link
	set linkd=%linkd% -WX
	set linkd=%linkd% -nologo -incremental:no -noimplib
	set linkd=%linkd% kernel32.lib user32.lib
	if "%CRT%" == "static"  set linkd=%linkd% -nodefaultlib libucrt.lib libvcruntime.lib libcmt.lib
	if "%CRT%" == "dynamic" set linkd=%linkd% -nodefaultlib    ucrt.lib    vcruntime.lib msvcrt.lib
	if "%optimize%" == "inspect" set linkd=%linkd% -debug:full -subsystem:console
	if "%optimize%" == "develop" set linkd=%linkd% -debug:full -subsystem:console
	if "%optimize%" == "release" set linkd=%linkd% -debug:none -subsystem:windows
	if "%arch%" == "32" set linkd=%linkd% -machine:x86
	if "%arch%" == "64" set linkd=%linkd% -machine:x64

	rem compile the whole group asynchronously by piping
	rem it into a silent pause command
	echo.[compile async] %time%
	(
		%cc% "%code%/base.c" -o "base.o"
		%cc% "%code%/main.c" -o "main.o"

		%resc% "%project%/windows_main.rc" -fo "windows_main.res"
	) | pause > nul

	rem link
	echo.[link main.exe] %time%
	%linkd% ^
		"%temp%/base.o" ^
		"%temp%/main.o" "%temp%/windows_main.res" ^
		-out:"main.exe"

	echo.[  complete!  ] %time%
goto :eof
