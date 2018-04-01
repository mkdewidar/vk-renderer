#include "vulkan-utils.hpp"


VkDebugReportCallbackEXT Callback;

VkInstance Instance;
VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
VkDevice Device = VK_NULL_HANDLE;
VkSurfaceKHR Surface;

VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
VkQueue GraphicsQueue = VK_NULL_HANDLE;
VkQueue PresentQueue = VK_NULL_HANDLE;

// Creation

void create_instance() {
    if (EnableValidationLayers && !check_validation_layers()) {
        throw std::runtime_error("Using validation layers, but could not find them all");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vk-renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    auto extensions = get_required_instance_extensions();
    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    if (EnableValidationLayers) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        instanceInfo.ppEnabledLayerNames = ValidationLayers.data();

        std::cout << "Using Validation Layers" << std::endl;
    } else {
        instanceInfo.enabledLayerCount = 0;
    }

    uint32_t availableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

    std::cout << "Using these extensions:\n";
    for (const auto& extension : availableExtensions) {
        std::cout << "\t" << extension.extensionName << " v" << extension.specVersion << "\n";
    }
    std::cout.flush();

    if (vkCreateInstance(&instanceInfo, nullptr, &Instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

void setup_debug_callback() {
    if (!EnableValidationLayers) return;

    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debug_callback;
    
    if (create_debug_report_callback_EXT(Instance, &createInfo, &Callback) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug callback");
    }
}

void pick_physical_device() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("Could not find any devices with Vulkan support");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (is_device_suitable(device)) {
			PhysicalDevice = device;
			break;
		}
	}

	if (PhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error(deviceCount + "device(s) found but none meet requirements");
	}

	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &deviceProps);
	std::cout << "Using device: " << deviceProps.deviceName << std::endl;
}

void create_logical_device()
{
	auto queueFamilyIndices = get_queue_family_indices(PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> requiredQueuesCreateInfo;
	std::set<int> uniqueQueueFamilyIndices = { queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily };

	float queuePriority = 1.0f;
	for (const int& queueFamilyIndex : uniqueQueueFamilyIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		requiredQueuesCreateInfo.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = requiredQueuesCreateInfo.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(requiredQueuesCreateInfo.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = Extensions.size();
	createInfo.ppEnabledExtensionNames = Extensions.data();

	if (EnableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
		createInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS) {
		throw std::runtime_error("Could not instantiate logical device");
	}

	vkGetDeviceQueue(Device, queueFamilyIndices.graphicsFamily, 0, &GraphicsQueue);
	std::cout << "Obtained graphics queue\n";
	vkGetDeviceQueue(Device, queueFamilyIndices.presentFamily, 0, &PresentQueue);
	std::cout << "Obtained present queue\n";

	std::cout.flush();
}

void create_swap_chain(const uint32_t width, const uint32_t height)
{
	auto swapChainDetails = get_swap_chain_support_details(PhysicalDevice, Surface);

	auto selectedFormat = get_best_surface_format(swapChainDetails.formats);
	auto selectedMode = get_best_present_mode(swapChainDetails.presentationModes);
	auto selectedExtent = get_best_swap_extent(swapChainDetails.capabilities, width, height);

	uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
	if ((swapChainDetails.capabilities.maxImageCount != 0) &&
		(imageCount > swapChainDetails.capabilities.maxImageCount)) {
		imageCount = swapChainDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = selectedFormat.format;
	createInfo.imageColorSpace = selectedFormat.colorSpace;
	createInfo.presentMode = selectedMode;
	createInfo.imageExtent = selectedExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto queueFamilies = get_queue_family_indices(PhysicalDevice);
	if (queueFamilies.graphicsFamily != queueFamilies.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		
		uint32_t queueFamilyIndices[] = {
			(uint32_t) queueFamilies.graphicsFamily, (uint32_t) queueFamilies.presentFamily
		};
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(Device, &createInfo, nullptr, &SwapChain) != VK_SUCCESS) {
		throw std::runtime_error("Could not create swapchain");
	}

	std::cout << "Swapchain created successfully" << std::endl;
}

// Queries

bool check_validation_layers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : ValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> get_required_instance_extensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}

bool check_device_extension_support(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	for (auto& requiredExtensionName : Extensions) {
		std::string_view requiredExtensionNameView = requiredExtensionName;

		bool notFound = std::find_if(availableExtensions.begin(), availableExtensions.end(),
			[&requiredExtensionNameView](VkExtensionProperties extensionProperty) {
			return extensionProperty.extensionName == requiredExtensionNameView;
		}) == availableExtensions.end();

		if (notFound) return false;
	}

	return true;
}

bool is_device_suitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(device, &deviceProps);
    std::cout << "Testing suitability of device: " << deviceProps.deviceName << std::endl;

	if (get_queue_family_indices(device).is_valid() && check_device_extension_support(device)) {
		auto swapChainDetails = get_swap_chain_support_details(device, Surface);
		
		return !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
	}

	return false;
}

QueueFamilyIndices get_queue_family_indices(VkPhysicalDevice device) {
	QueueFamilyIndices queueFamilyIndices = {};

    uint32_t availableQueueFamiliesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &availableQueueFamiliesCount, nullptr);
    std::cout << "Number of queue families for device: " << availableQueueFamiliesCount << std::endl;

    std::vector<VkQueueFamilyProperties> availableQueueFamilies(availableQueueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &availableQueueFamiliesCount, availableQueueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : availableQueueFamilies) {
        if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            queueFamilyIndices.graphicsFamily = i;
        }

		VkBool32 presentQueueSupported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, Surface, &presentQueueSupported);
		if (presentQueueSupported) {
			queueFamilyIndices.presentFamily = i;
		}

		if (queueFamilyIndices.is_valid()) break;

        i++;
    }
    
    return queueFamilyIndices;
}

// Cleanup

SwapChainSupportDetails get_swap_chain_support_details(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapChainSupportDetails surfaceDetails = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceDetails.capabilities);

	uint32_t formatsCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);
	if (formatsCount != 0) {
		surfaceDetails.formats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, surfaceDetails.formats.data());
	}

	uint32_t modesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modesCount, nullptr);
	if (modesCount != 0) {
		surfaceDetails.presentationModes.resize(modesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
			&modesCount, surfaceDetails.presentationModes.data());
	}

	return surfaceDetails;
}

VkSurfaceFormatKHR get_best_surface_format(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() && (formats[0].format == VK_FORMAT_UNDEFINED)) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	}

	for (auto& format : formats) {
		if ((format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
			(format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)) {
			return format;
		}
	}

	std::cout << "Device does not directly support preferred surface format, picking first one" << std::endl;
	return formats[0];
}

VkPresentModeKHR get_best_present_mode(const std::vector<VkPresentModeKHR>& presentModes)
{
	VkPresentModeKHR chosenMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	for (auto& availableMode : presentModes) {
		switch (availableMode) {
		case VK_PRESENT_MODE_FIFO_KHR:
			chosenMode = VK_PRESENT_MODE_FIFO_KHR;
			continue;

		case VK_PRESENT_MODE_MAILBOX_KHR:
			return availableMode;
		}
	}

	std::cout << "Using present mode " << chosenMode << std::endl;
	return chosenMode;
}

VkExtent2D get_best_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities,
	const uint32_t windowWidth, const uint32_t windowHeight)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D extent = {};

		extent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, windowWidth));

		extent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, windowHeight));

		return extent;
	}
}

void vulkan_destroy_surface(VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(Instance, surface, nullptr);
}

void vulkan_cleanup() {
    destroy_debug_report_callback_EXT(Instance, Callback);
	vkDestroySwapchainKHR(Device, SwapChain, nullptr);
	vkDestroyDevice(Device, nullptr);
    vkDestroyInstance(Instance, nullptr);
}

// Extensions

VkResult create_debug_report_callback_EXT(VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT* createInfo,
    VkDebugReportCallbackEXT* callback) {
    
    auto func =
        (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, createInfo, nullptr, callback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_report_callback_EXT(VkInstance instance, VkDebugReportCallbackEXT callback) {
    auto func =
        (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, nullptr);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData) {

    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}
