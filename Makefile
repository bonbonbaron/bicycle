# Convenience wrapper around CMake (CMakeLists.txt is the build). `make` builds ./o, `make clean` cleans,
# `make install` puts the headers in /usr/local/include/bicycle and o in /usr/local/bin.
BUILD := build

all:
	cmake -S . -B $(BUILD)
	cmake --build $(BUILD) -j

install: all
	sudo rm -rf /usr/local/include/bicycle
	sudo mkdir -p /usr/local/include/bicycle
	sudo cp include/*.h /usr/local/include/bicycle
	sudo cp o /usr/local/bin

.PHONY: all clean install
clean:
	rm -rf $(BUILD) o o.exe
