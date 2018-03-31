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
    create_instance();

    setup_debug_callback();

	create_surface();

    pick_physical_device();

	create_logical_device();
}

void create_surface()
{
	if (glfwCreateWindowSurface(Instance, Window, nullptr, &Surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
}

void main_loop() {
    while(!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
    }
}

void cleanup() {
	vulkan_destory_surface(Surface);

	vulkan_cleanup();

    glfwDestroyWindow(Window);

    glfwTerminate();
}
