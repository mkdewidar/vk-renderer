#include "vulkan-utils.hpp"


VkDebugReportCallbackEXT Callback;

VkInstance Instance;
VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
VkDevice Device = VK_NULL_HANDLE;
VkSurfaceKHR Surface;

VkSurfaceFormatKHR SurfaceFormat;
VkExtent2D SurfaceExtent;

VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
VkQueue GraphicsQueue = VK_NULL_HANDLE;
VkQueue PresentQueue = VK_NULL_HANDLE;
std::vector<VkImageView> ImageViews;
VkRenderPass RenderPass;
VkPipeline Pipeline;

VkPipelineLayout PipelineLayout;

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
	SurfaceFormat = selectedFormat;
	SurfaceExtent = selectedExtent;
}

void create_image_views()
{
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(Device, SwapChain, &imageCount, nullptr);

	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(Device, SwapChain, &imageCount, images.data());

	ImageViews.resize(imageCount);

	uint32_t i = 0;
	for (auto& image : images) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.format = SurfaceFormat.format;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.levelCount = 1;

		if (vkCreateImageView(Device, &createInfo, nullptr, &ImageViews[i])) {
			throw std::runtime_error("Could not create image view");
		}

		i++;
	}

	std::cout << "Created " << ImageViews.size() << " image views" << std::endl;
}

void create_render_pass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = SurfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(Device, &createInfo, nullptr, &RenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}

	std::cout << "Created render pass" << std::endl;
}

void create_graphics_pipeline()
{
	auto vertexShader = read_shader_bytecode("shaders/vert.spv");
	auto fragmentShader = read_shader_bytecode("shaders/frag.spv");

	VkShaderModule vertexShaderModule = create_shader_module(vertexShader);
	VkShaderModule fragmentShaderModule = create_shader_module(fragmentShader);

	VkPipelineShaderStageCreateInfo vertexShaderPipelineStage = {};
	vertexShaderPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderPipelineStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderPipelineStage.module = vertexShaderModule;
	vertexShaderPipelineStage.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderPipelineStage = {};
	fragmentShaderPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderPipelineStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderPipelineStage.module = fragmentShaderModule;
	fragmentShaderPipelineStage.pName = "main";

	VkPipelineShaderStageCreateInfo shaderPipelineStages[] = {
		vertexShaderPipelineStage, fragmentShaderPipelineStage
	};

	VkPipelineVertexInputStateCreateInfo vertexInputPipelineStage = {};
	vertexInputPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputPipelineStage.vertexBindingDescriptionCount = 0;
	vertexInputPipelineStage.pVertexBindingDescriptions = nullptr;
	vertexInputPipelineStage.vertexAttributeDescriptionCount = 0;
	vertexInputPipelineStage.pVertexBindingDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyPipelineStage = {};
	inputAssemblyPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyPipelineStage.primitiveRestartEnable = VK_FALSE;
	inputAssemblyPipelineStage.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportPipelineStage = {};
	viewportPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float) SurfaceExtent.width;
	viewport.height = (float) SurfaceExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportPipelineStage.viewportCount = 1;
	viewportPipelineStage.pViewports = &viewport;

	VkRect2D viewportScissor = {};
	viewportScissor.extent = SurfaceExtent;
	viewportScissor.offset = { 0, 0 };

	viewportPipelineStage.scissorCount = 1;
	viewportPipelineStage.pScissors = &viewportScissor;

	VkPipelineRasterizationStateCreateInfo rasterizationPipelineStage = {};
	rasterizationPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationPipelineStage.depthClampEnable = VK_FALSE;
	rasterizationPipelineStage.rasterizerDiscardEnable = VK_FALSE;
	rasterizationPipelineStage.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationPipelineStage.lineWidth = 1.0f;
	rasterizationPipelineStage.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationPipelineStage.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationPipelineStage.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisamplingPipelineStage = {};
	multisamplingPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingPipelineStage.sampleShadingEnable = VK_FALSE;
	multisamplingPipelineStage.alphaToOneEnable = VK_FALSE;
	multisamplingPipelineStage.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendPipelineStage = {};
	colorBlendPipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendPipelineStage.logicOpEnable = VK_FALSE;
	colorBlendPipelineStage.attachmentCount = 1;
	colorBlendPipelineStage.pAttachments = &colorBlendAttachment;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	if (vkCreatePipelineLayout(Device, &pipelineLayoutCreateInfo, nullptr, &PipelineLayout)) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderPipelineStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputPipelineStage;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyPipelineStage;
	pipelineCreateInfo.pViewportState = &viewportPipelineStage;
	pipelineCreateInfo.pRasterizationState = &rasterizationPipelineStage;
	pipelineCreateInfo.pMultisampleState = &multisamplingPipelineStage;
	pipelineCreateInfo.pColorBlendState = &colorBlendPipelineStage;
	pipelineCreateInfo.layout = PipelineLayout;
	pipelineCreateInfo.renderPass = RenderPass;
	pipelineCreateInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &Pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	std::cout << "Created graphics pipeline" << std::endl;

	vkDestroyShaderModule(Device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(Device, fragmentShaderModule, nullptr);
}

VkShaderModule create_shader_module(const std::vector<char>& shaderByteCode)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderByteCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderByteCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module from bytcode");
	}

	return shaderModule;
}

std::vector<char> read_shader_bytecode(const std::string & filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open shader " + filename);
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> fileContents(fileSize);

	file.seekg(0);
	file.read(fileContents.data(), fileSize);

	file.close();

	std::cout << "Loaded shader " << filename << " with size " << fileContents.size() << std::endl;

	return fileContents;
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

void vulkan_cleanup() {
    destroy_debug_report_callback_EXT(Instance, Callback);

	for (auto imageView : ImageViews) {
		vkDestroyImageView(Device, imageView, nullptr);
	}

	vkDestroyPipeline(Device, Pipeline, nullptr);
	vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	vkDestroyRenderPass(Device, RenderPass, nullptr);

	vkDestroySwapchainKHR(Device, SwapChain, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);

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
