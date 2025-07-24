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


xcopy /s /y /q /I assets bin\Debug\assets
xcopy /s /y /q /I assets build\assets

echo "Done."

pause
