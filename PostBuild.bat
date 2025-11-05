@echo off

echo "Compiling shaders..."

for %%A IN ("assets\shaders\*.vert.glsl") DO (
    echo "assets/shaders/%%~nA.glsl -> assets/shaders/%%~nA.spv"
    %VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/%%~nA.glsl -o assets/shaders/%%~nA.spv
)

for %%A IN ("assets\shaders\*.frag.glsl") DO (
    echo "assets/shaders/%%~nA.glsl -> assets/shaders/%%~nA.spv"
    %VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/%%~nA.glsl -o assets/shaders/%%~nA.spv
)


xcopy /s /y /q /I assets build\testbed\assets
xcopy /s /y /q /I assets build\testbed\Debug\assets

if not exist ".\build\testbed\logs" mkdir .\build\testbed\logs
if not exist ".\build\testbed\Debug\logs" mkdir .\build\testbed\Debug\logs

echo "Done."

pause
