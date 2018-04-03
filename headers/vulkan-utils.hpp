#pragma once

#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"

#ifdef NDEBUG
    const bool EnableValidationLayers = false;
#else
    const bool EnableValidationLayers = true;
#endif


extern VkDebugReportCallbackEXT Callback;

extern VkInstance Instance;
extern VkPhysicalDevice PhysicalDevice;
extern VkDevice Device;
extern VkSurfaceKHR Surface;

extern VkSurfaceFormatKHR SurfaceFormat;
extern VkExtent2D SurfaceExtent;

extern VkSwapchainKHR SwapChain;
extern VkQueue GraphicsQueue;
extern VkQueue PresentQueue;
extern std::vector<VkImageView> ImageViews;
extern VkRenderPass RenderPass;
extern VkPipeline Pipeline;
extern VkPipelineLayout PipelineLayout;

extern std::vector<VkFramebuffer> Framebuffers;
extern VkCommandPool CommandPool;
extern std::vector<VkCommandBuffer> CommandBuffers;

extern VkSemaphore ImageAvailableSemaphore;
extern VkSemaphore RenderFinishSemaphore;


const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> Extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Creation

void create_instance();

void setup_debug_callback();

void pick_physical_device();

void create_logical_device();

void create_swap_chain(const uint32_t width, const uint32_t height);

void create_image_views();

void create_render_pass();

void create_graphics_pipeline();

VkShaderModule create_shader_module(const std::vector<char>& shaderByteCode);

std::vector<char> read_shader_bytecode(const std::string& filename);

void create_framebuffers();

void create_command_pool();

void create_command_buffers();

void create_semaphores();

// Queries

bool check_validation_layers();

std::vector<const char*> get_required_instance_extensions();

bool check_device_extension_support(VkPhysicalDevice device);

bool is_device_suitable(VkPhysicalDevice device);

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool is_valid() {
		return (graphicsFamily >= 0) && (presentFamily >= 0);
	}
};
QueueFamilyIndices get_queue_family_indices(VkPhysicalDevice device);

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentationModes;
};
SwapChainSupportDetails get_swap_chain_support_details(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

VkSurfaceFormatKHR get_best_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);

VkPresentModeKHR get_best_present_mode(const std::vector<VkPresentModeKHR>& presentModes);

VkExtent2D get_best_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities,
	const uint32_t width, const uint32_t height);

// Cleanup

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
