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
set data=%root%/data
set project=%root%/project
set temp=%root%/temp

rem prepare directories
if not exist "build" mkdir "build"
if not exist "build/data" mkdir "build/data"
if not exist "temp"  mkdir "temp"

del "build\*" /s /q > nul
if "%clean%" == "true" (
	del "temp\*" /s /q > nul
)

rem prepare common tools
rem @note there's also "glslangValidator.exe"
where -q "glslc.exe" || (
	if not "%VULKAN_SDK%" == "" (
		set "PATH=%PATH%;%VULKAN_SDK%/Bin"
	)
	where -q "glslc.exe" || (
		echo.please, install "Vulkan SDK", https://vulkan.lunarg.com/sdk/home
		exit /b 1
	)
)

rem shader compiler flags
set SHADERC=start /d "build" /b glslc -Werror
if "%optimize%" == "inspect" set SHADERC=%SHADERC% -O0 -g -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_INSPECT
if "%optimize%" == "develop" set SHADERC=%SHADERC% -O  -g -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_DEVELOP
if "%optimize%" == "release" set SHADERC=%SHADERC% -O     -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_RELEASE

set VTXC=%SHADERC% -DBUILD_STAGE=BUILD_STAGE_VERTEX
set FRGC=%SHADERC% -DBUILD_STAGE=BUILD_STAGE_FRAGMENT

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

	rem @info batch
	rem https://ss64.com/nt/
	rem https://learn.microsoft.com/windows-server/administration/windows-commands

	rem @info clang / gcc
	rem https://gcc.gnu.org/onlinedocs/
	rem https://clang.llvm.org/docs/CommandGuide/clang.html

	rem @info clang's lld-link / MSVC's link
	rem https://learn.microsoft.com/cpp/build/reference/linking
	rem https://learn.microsoft.com/cpp/c-runtime-library/crt-library-features

	rem C compiler flags
	set CC=start /d "temp" /b clang -ansi -c -std=c99 -pedantic-errors -fno-exceptions -fno-rtti -ffp-contract=off -flto=thin
	set CC=%CC% -I"%code%" -I"%project%"
	set CC=%CC% -I"%code%/_external"
	set CC=%CC% -I"%VULKAN_SDK%/Include"
	if "%optimize%" == "inspect" set CC=%CC% -O0 -g -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_INSPECT -DBUILD_TARGET=BUILD_TARGET_TERMINAL
	if "%optimize%" == "develop" set CC=%CC% -Og -g -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_DEVELOP -DBUILD_TARGET=BUILD_TARGET_TERMINAL
	if "%optimize%" == "release" set CC=%CC% -O2    -DBUILD_OPTIMIZE=BUILD_OPTIMIZE_RELEASE -DBUILD_TARGET=BUILD_TARGET_GRAPHICAL
	if "%arch%" == "32" set CC=%CC% -m32
	if "%arch%" == "64" set CC=%CC% -m64

	rem resource compiler flags
	set RESC=start /d "temp" /b llvm-rc
	set RESC=%RESC% -nologo

	rem linker flags
	set LD=start /d "build" /b /wait lld-link -nologo -incremental:no -noimplib -WX
	set LD=%LD% kernel32.lib user32.lib
	if "%CRT%" == "static"  set LD=%LD% -nodefaultlib libucrt.lib libvcruntime.lib libcmt.lib
	if "%CRT%" == "dynamic" set LD=%LD% -nodefaultlib    ucrt.lib    vcruntime.lib msvcrt.lib
	if "%optimize%" == "inspect" set LD=%LD% -debug:full -subsystem:console
	if "%optimize%" == "develop" set LD=%LD% -debug:full -subsystem:console
	if "%optimize%" == "release" set LD=%LD% -debug:none -subsystem:windows
	if "%arch%" == "32" set LD=%LD% -machine:x86
	if "%arch%" == "64" set LD=%LD% -machine:x64

	rem compile the whole group asynchronously by piping
	rem it into a silent pause command
	echo.[ compile  async ] %time%
	(
		%CC% "%code%/base.c" -o "base.o"
		%CC% "%code%/os_windows.c" -o "os_windows.o"
		%CC% "%code%/rhi_vulkan.c" -o "rhi_vulkan.o"
		%CC% "%code%/unknown.c"    -o "unknown.o"

		%RESC% "%project%/windows_main.rc" -fo "windows_main.res"

		%VTXC% "%data%/shader.glsl" -o data/shader_vert.spirv
		%FRGC% "%data%/shader.glsl" -o data/shader_frag.spirv
	) | pause > nul

	rem link
	echo.[link unknown.exe] %time%
	%LD% ^
		"%temp%/base.o" ^
		"%temp%/os_windows.o" ^
		"%temp%/rhi_vulkan.o" "%VULKAN_SDK%/Lib/vulkan-1.lib" ^
		"%temp%/unknown.o"    "%temp%/windows_main.res" ^
		-out:"unknown.exe"

	echo.[    complete    ] %time%
goto :eof
