#pragma once

#include <vector>
#include <set>
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


extern VkInstance Instance;
extern VkDebugReportCallbackEXT Callback;
extern VkPhysicalDevice PhysicalDevice;
extern VkDevice Device;
extern VkSurfaceKHR Surface;

extern VkQueue GraphicsQueue;
extern VkQueue PresentQueue;

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

// Creation

void create_instance();

void setup_debug_callback();

void pick_physical_device();

void create_logical_device();

// Queries

bool check_validation_layers();

std::vector<const char*> get_required_extensions();

bool is_device_suitable(VkPhysicalDevice device);

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool is_valid() {
		return (graphicsFamily >= 0) && (presentFamily >= 0);
	}
};
QueueFamilyIndices get_queue_family_indices(VkPhysicalDevice device);

// Cleanup

void vulkan_destory_surface(VkSurfaceKHR surface);

void vulkan_cleanup();

// Extensions

VkResult create_debug_report_callback_EXT(VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT* createInfo,
    VkDebugReportCallbackEXT* callback);

void destroy_debug_report_callback_EXT(VkInstance instance, VkDebugReportCallbackEXT callback);

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData);
