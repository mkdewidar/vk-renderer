OUTPUT_NAME = vk-renderer

COMPILER = g++
SOURCES = cpp/*
INCLUDES = headers/

VULKAN_SDK_PATH = /c/VulkanSDK/1.1.70.0
GLM_PATH = /c/Libs/glm-0.9.8.5
GLFW3_PATH = /c/Libs/glfw-3.2.1.bin.WIN64

COMPILER_INCLUDES = -I$(INCLUDES) -I$(VULKAN_SDK_PATH)/Include -I$(GLFW3_PATH)/include -I$(GLM_PATH)
COMPILER_FLAGS = -std=c++14 -Wall
LINKER_FLAGS = -L$(VULKAN_SDK_PATH)/Lib -L$(GLFW3_PATH)/lib-mingw-w64 -lvulkan-1 -lglfw3 -mwindows

all:
	echo "Available targets are: windows, clean"
	
windows:
	$(COMPILER) $(COMPILER_FLAGS) $(SOURCES) -o build/$(OUTPUT_NAME) $(COMPILER_INCLUDES) $(LINKER_FLAGS)

clean:
	rm -f build/*

