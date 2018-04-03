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

	create_swap_chain(WIDTH, HEIGHT);

	create_image_views();

	create_render_pass();

	create_graphics_pipeline();

	create_framebuffers();

	create_command_pool();

	create_command_buffers();

	create_semaphores();
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

		draw_frame();
    }

	vkDeviceWaitIdle(Device);
}

void draw_frame() {
	uint32_t imageIndex = 0;
	vkAcquireNextImageKHR(Device, SwapChain, std::numeric_limits<uint64_t>::max(), ImageAvailableSemaphore,
		VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &ImageAvailableSemaphore;

	VkPipelineStageFlags waitForStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	submitInfo.pWaitDstStageMask = waitForStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CommandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &RenderFinishSemaphore;

	vkQueueSubmit(GraphicsQueue, 1, &submitInfo, NULL);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &SwapChain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &RenderFinishSemaphore;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(PresentQueue, &presentInfo);
}

void cleanup() {
	vulkan_cleanup();

    glfwDestroyWindow(Window);

    glfwTerminate();
}
