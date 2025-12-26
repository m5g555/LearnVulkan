SRCDIR := src
LIBDIR := libs

CFLAGS := -std=c++17 -O2
LDFLAGS := -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# create list of all spv files and set as dependency
vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

TARGET = a.out
$(TARGET): $(vertObjFiles) $(fragObjFiles)
$(TARGET): $(SRCDIR)/*.cpp $(SRCDIR)/*.hpp
	g++ $(CFLAGS) -o $(TARGET) $(SRCDIR)/*.cpp $(LDFLAGS)

# make shader targets
%.spv: %
	glslc $< -o $@

.PHONY: test clean

test: a.out
	./a.out

clean:
	rm -f a.out