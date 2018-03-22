#include <iostream>
#include <vector>
#include <cstdlib>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

void init_window();

void init_vulkan();
bool check_validation_layers();
std::vector<const char*> get_required_extensions();
void setup_debug_callback();

void main_loop();

void cleanup();

#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
