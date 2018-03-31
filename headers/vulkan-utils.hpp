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
static VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
static VkDevice Device = VK_NULL_HANDLE;

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

void create_instance();

bool check_validation_layers();

std::vector<const char*> get_required_extensions();

void setup_debug_callback();

void pick_physical_device();
bool is_device_suitable(VkPhysicalDevice device);

uint32_t get_queue_family_index(VkPhysicalDevice device);

void create_logical_device();

void vulkan_cleanup();

VkResult create_debug_report_callback_EXT(VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT* createInfo,
    VkDebugReportCallbackEXT* callback);
void destroy_debug_report_callback_EXT(VkInstance instance, VkDebugReportCallbackEXT callback);

