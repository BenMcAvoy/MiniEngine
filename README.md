# MiniEngine

MiniEngine is an extremely basic Vulkan game engine wrote in C++ purely for educational purposes. It is not intended to go further than this, and is not intended to be used in any serious project. It is instead, my first ever attempt at using Vulkan (I am only 15 years old at the time of writing this) and I am using it to learn the basics of Vulkan and game engine development.

## Compiling
> [!NOTE]
> You must have CMake, a compiler, VCPKG and the Vulkan SDK along with the GLFW system dependencies (listed on their site [here](https://www.glfw.org/docs/latest/compile_guide.html#compile_deps)) installed on your system to compile this project.
```bash
cmake --preset=linux -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
If you are on Windows, simply substitute `linux` with `windows`.
