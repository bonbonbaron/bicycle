DBG=-g
STD=-std=c++20

BICREPO := $(shell git rev-parse --show-toplevel)
BICSRCS := main bicycle m/Entity m/Box v/Image c/Trigger c/Timer c/SshInputListener v/ColorPalette v/Window c/WindowManager v/Dialogue v/Menu v/TextMenu m/Camera m/World v/Scene c/CollisionDetector c/Kinematics c/Animator c/MidiPlayer trigLUT # c/Input v/Bar c/Dice v/Graph 
BICINCS := $(BICINCS:%=$(BICREPO)/include/%.h)

BICOBJS := $(BICSRCS:%=$(BICREPO)/build/%.o)
BICTGT := o  # for now
all: $(BICTGT)

PKGS=luajit ncurses fluidsynth 
CFLAGS=$(shell pkg-config --cflags $(PKGS))
LFLAGS=$(shell pkg-config --libs $(PKGS))
# Consider these compiler options when you're ready to hard-core optimize.
# g++ -O3 -march=armv8-a -mcpu=cortex-a72 -mtune=cortex-a72 -mfpu=neon-fp-armv8 -mfloat-abi=hard ...
# TODO figure out how to only avoid removal of symbols in specific files.
$(BICTGT): $(BICOBJS)
	g++ -Wall -Wno-switch -rdynamic $(DBG) $(STD) $(BICOBJS) -levdev $(LFLAGS) -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp $(BICREPO)/include/%.h 
	g++ -Wall -Wno-switch $(DBG) $(STD) -c $< -I$(BICREPO)/include $(CFLAGS)  -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp ${BICREPO}/build/m ${BICREPO}/build/v ${BICREPO}/build/c 
	g++ -Wall -Wno-switch $(DBG) $(STD) -c $< -I$(BICREPO)/include $(CFLAGS)  -o $@

$(BICREPO)/build/m:
	mkdir -p $(BICREPO)/build/m

$(BICREPO)/build/v:
	mkdir -p $(BICREPO)/build/v

$(BICREPO)/build/c:
	mkdir -p $(BICREPO)/build/c

install: $(BICTGT)
	sudo rm -rf /usr/local/include/bicycle
	sudo mkdir -p /usr/local/include/bicycle
	sudo cp include/*.h /usr/local/include/bicycle
	sudo cp $(BICTGT) /usr/local/bin


.PHONY: clean
clean:
	rm -rf $(BICREPO)/build/*/*.o
	rm -rf $(BICREPO)/build/*.o
