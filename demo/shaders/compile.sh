# compiles frag.glsl and vert.glsl into frag.spv and vert.spv

set -e
set -x

glslc -fshader-stage=vertex vert.glsl -o vert.spv 
glslc -fshader-stage=fragment frag.glsl -o frag.spv 
