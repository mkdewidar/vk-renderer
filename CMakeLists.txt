# Tells CMake how to build the project. Split into the following sections:
#   config: Sets variables and paths needed for the build. Might need to change some values to match your system.
#   flags: Set flags for compiler, linker etc.
#   dependencies: Finds packages required for the project.
#   resources: Manages the shaders and their compilation/inclusion in the output directory.
#   output: Including the source, headers and defining the output targets of the project.
cmake_minimum_required(VERSION 3.10)
project(vk_renderer)

# config

# should be updated to point the Vulkan SDK to be used
set(ENV{VULKAN_SDK} "/usr/local/VulkanSDK/1.1.73.0/x86_64/")
MESSAGE("Using Vulkan SDK at: $ENV{VULKAN_SDK}")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

MESSAGE("")

# flags

set(CMAKE_CXX_STANDARD 17)

# dependencies

find_package(glfw3 3.2 REQUIRED)
find_package(Vulkan REQUIRED)

# resources

find_file(GLSL_VALIDATOR_PATH "*glslangValidator" "$ENV{VULKAN_SDK}" NO_DEFAULT_PATH)
MESSAGE("Found glsl validator at: ${GLSL_VALIDATOR_PATH}")

file(GLOB VERTEX_SHADERS_FILES "shaders/*.vert")
file(GLOB FRAGMENT_SHADERS_FILES "shaders/*.frag")

MESSAGE("Compiling vertex shaders from ./shaders/*.vert...")
execute_process(COMMAND "${GLSL_VALIDATOR_PATH}" "-V" "${VERTEX_SHADERS_FILES}" WORKING_DIRECTORY "shaders/")
MESSAGE("Compiling fragment shaders from ./shaders/*.frag...")
execute_process(COMMAND "${GLSL_VALIDATOR_PATH}" "-V" "${FRAGMENT_SHADERS_FILES}" WORKING_DIRECTORY "shaders/")

file(GLOB SPV_FILES "shaders/*.spv")

file(COPY ${SPV_FILES} DESTINATION "${CMAKE_BINARY_DIR}/shaders")

MESSAGE("")

# output

file(GLOB_RECURSE SOURCE_CPP_FILES "cpp/*.cpp")
add_executable(renderer ${SOURCE_CPP_FILES})

target_include_directories(renderer PUBLIC headers/ ${Vulkan_INCLUDE_DIRS} ${glfw3_INCLUDE_DIRS})

target_link_libraries(renderer ${Vulkan_LIBRARIES} glfw)
