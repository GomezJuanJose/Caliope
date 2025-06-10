@echo off

echo "Compiling shaders..."

echo "assets/shaders/Builtin.SpriteShader.vert.glsl -> assets/shaders/Builtin.SpriteShader.vert.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/Builtin.SpriteShader.vert.glsl -o assets/shaders/Builtin.SpriteShader.vert.spv


echo "assets/shaders/Builtin.SpriteShader.frag.glsl -> assets/shaders/Builtin.SpriteShader.frag.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/Builtin.SpriteShader.frag.glsl -o assets/shaders/Builtin.SpriteShader.frag.spv


echo "Done."

pause