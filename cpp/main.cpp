#include "main.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

GLFWwindow* Window;

int main() {
    try {
        init_window();

        init_vulkan();
        
        main_loop();

        cleanup();
    } catch (const std::runtime_error& error) {
        std::cerr << error.what() << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void init_window() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    Window = glfwCreateWindow(WIDTH, HEIGHT, "vk-renderer", nullptr, nullptr);
}

void init_vulkan() {
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

    setup_debug_callback();
}

void main_loop() {
    while(!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
    }
}

void cleanup() {
    destroy_debug_report_callback_EXT(Instance, Callback);
    vkDestroyInstance(Instance, nullptr);

    glfwDestroyWindow(Window);

    glfwTerminate();
}
