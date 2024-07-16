

# DIR for build files
BUILD_DIR = build
# create build/obj/ directory
$(shell mkdir -p $(BUILD_DIR)/obj)

CUCXX = nvcc
CXX = g++

CFLAGS = -Wall -fPIC
LDFLAGS = -lpthread -libverbs -lcudart -L/usr/local/cuda/lib64

# match all .cpp file under src/
CSRC = $(wildcard src/*.cpp)
CUSRC = $(wildcard src/*.cu)
# all .o files
COBJ = $(SRC:.cpp=.o)
CUOBJ = $(CUSRC:.cu=.o)

# create obj build list, replace src with build/obj
COBJ_BUILD = $(patsubst src/%.cpp, $(BUILD_DIR)/obj/%.o, $(CSRC))
CUOBJ_BUILD = $(patsubst src/%.cu, $(BUILD_DIR)/obj/%.o, $(CUSRC))


# build libcora.so
cora: $(COBJ_BUILD) $(CUOBJ_BUILD)
	$(CXX) -shared -o libcora.so $(COBJ_BUILD) $(CUOBJ_BUILD) $(LDFLAGS)

all: cora

# build .o files
$(BUILD_DIR)/obj/%.o: src/%.cu
	$(CUCXX) -c $< -Xcompiler -fPIC -o $@

$(BUILD_DIR)/obj/%.o: src/%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@


.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/obj/*.o
	rm -f libcora.so

