#include "vulkan-utils.hpp"


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

    auto extensions = get_required_extensions();
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

std::vector<const char*> get_required_extensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
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

    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(PhysicalDevice, &deviceProps);
    std::cout << "Using device: " << deviceProps.deviceName << std::endl;

    if (PhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error(deviceCount + "device(s) found but none meet requirements");
    }
}
bool is_device_suitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(device, &deviceProps);
    std::cout << "Testing suitability of device: " << deviceProps.deviceName << std::endl;

    return get_queue_family_index(device) >= 0;
}

uint32_t get_queue_family_index(VkPhysicalDevice device) {
    uint32_t queueIndex = -1;

    uint32_t queueFamiliesAvailable = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesAvailable, nullptr);
    std::cout << "Number of queue families: " << queueFamiliesAvailable << std::endl;

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesAvailable);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesAvailable, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            queueIndex = i;
            break;
        }

        i++;
    }
    
    return queueIndex;
}

void create_logical_device()
{
	uint32_t queueIndex = get_queue_family_index(PhysicalDevice);

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = queueIndex;
	queueCreateInfo.queueCount = 1;
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = 0;

	if (EnableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
		createInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else {
		createInfo.enabledExtensionCount = 0;
	}

	if (vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS) {
		throw std::runtime_error("Could not instantiate logical device");
	}
}

void vulkan_cleanup() {
    destroy_debug_report_callback_EXT(Instance, Callback);
	vkDestroyDevice(Device, nullptr);
    vkDestroyInstance(Instance, nullptr);
}

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

