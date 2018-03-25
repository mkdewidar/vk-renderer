#pragma once

#include <vector>
#include <iostream>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"

#ifdef NDEBUG
    const bool EnableValidationLayers = false;
#else
    const bool EnableValidationLayers = true;
#endif


static VkInstance Instance;
static VkDebugReportCallbackEXT Callback;

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

void init_vulkan();

void create_instance();

bool check_validation_layers();

std::vector<const char*> get_required_extensions();

void setup_debug_callback();

VkResult create_debug_report_callback_EXT(VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT* createInfo,
    VkDebugReportCallbackEXT* callback);
void destroy_debug_report_callback_EXT(VkInstance instance, VkDebugReportCallbackEXT callback);
