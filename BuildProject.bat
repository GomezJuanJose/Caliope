cmake -S . -B build

move build\Caliope.sln .

@echo off
setlocal enableextensions disabledelayedexpansion


REM TODO: Make it to search for project names to not hardcode it or in the future create a more robust build system using other language for crossplatform

call :modifysln "ALL_BUILD.vcxproj" "build/ALL_BUILD.vcxproj" "Caliope.sln"
call :modifysln "ZERO_CHECK.vcxproj" "build/ZERO_CHECK.vcxproj" "Caliope.sln"


call :modifysln "testbed.vcxproj" "build/testbed.vcxproj" "Caliope.sln"
call :modifysln "engine.vcxproj" "build/engine.vcxproj" "Caliope.sln"

pause

:modifysln
set "search=%~1"
set "replace=%~2"

set "textFile=%~3"

for /f "delims=" %%i in ('type "%textFile%" ^& break ^> "%textFile%" ') do (
    set "line=%%i"
    setlocal enabledelayedexpansion
    >>"%textFile%" echo(!line:%search%=%replace%!
    endlocal
)