cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# enable all warnings (but not as errors)
if (WIN32)
    add_compile_options(/W4 /utf-8)

	# Statically link the runtime libraries
	# /MtD if debug, /Mt if release
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

project(MiniEngine)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)

# MiniEngine
file (GLOB_RECURSE RIVET_SOURCES "src/*.cpp")
# add_library(MiniEngine STATIC ${RIVET_SOURCES})
add_executable(MiniEngine ${RIVET_SOURCES})
target_include_directories(MiniEngine PRIVATE include ${Stb_INCLUDE_DIR})

# link to vulkan
target_link_libraries(MiniEngine PRIVATE fmt::fmt glfw spdlog::spdlog glm::glm Vulkan::Vulkan imgui::imgui)
