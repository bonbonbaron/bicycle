DBG=
STD=-std=c++20

BICREPO := $(shell git rev-parse --show-toplevel)
BICSRCS := main bicycle m/Entity m/Rect c/Trigger c/Timer c/SshInput v/ColorPalette v/Window c/WindowManager v/Dialogue v/Menu v/TextMenu v/Image m/Camera m/World v/Scene c/CollisionDetector # c/Input v/Bar c/Dice c/MidiPlayer v/Graph 
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
	g++ -Wall -rdynamic $(DBG) $(STD) $(BICOBJS) -levdev $(LFLAGS) -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp $(BICREPO)/include/%.h 
	g++ -Wall $(DBG) $(STD) -c $< -I$(BICREPO)/include $(CFLAGS)  -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp ${BICREPO}/build/m ${BICREPO}/build/v ${BICREPO}/build/c 
	g++ -Wall $(DBG) $(STD) -c $< -I$(BICREPO)/include $(CFLAGS)  -o $@

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
