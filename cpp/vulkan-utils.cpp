#include "vulkan-utils.hpp"


VkInstance Instance;
VkDebugReportCallbackEXT Callback;
VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
VkDevice Device = VK_NULL_HANDLE;
VkSurfaceKHR Surface;

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

    return get_queue_family_indices(device).is_valid() && check_device_extension_support(device);
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

void vulkan_destory_surface(VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(Instance, surface, nullptr);
}

void vulkan_cleanup() {
    destroy_debug_report_callback_EXT(Instance, Callback);
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
