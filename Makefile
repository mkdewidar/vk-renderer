OUTPUT_NAME = vk-renderer

COMPILER=g++
SOURCES=src/*.cpp

all:
	$(COMPILER) $(SOURCES) -o build/$(OUTPUT_NAME)

clean:
	rm -f build/*
