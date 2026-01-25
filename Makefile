DBG=-g
STD=-std=c++20

BICREPO := $(shell git rev-parse --show-toplevel)
BICSRCS := Bar  bicycle  Body ColorPalette  Dialogue  Dice  Edge  Menu  MidiPlayer  Node Window  WindowManager Timer Personality Action
BICINCS := bicycle ColorPalette Dialogue Menu Constellation MidiPlayer Window WindowManager
BICINCS := $(BICINCS:%=$(BICREPO)/include/%.h)

BICOBJS := $(BICSRCS:%=$(BICREPO)/build/%.o)
BICTGT := $(BICREPO)/build/libbicycle.a
all: $(BICTGT)

$(BICTGT): $(BICOBJS)
	ar rcs $@ $(BICOBJS) 

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp $(BICREPO)/include/%.h 
	g++ $(DBG) $(STD) -c $< -I$(BICREPO)/include $(shell ncursesw6-config --cflags ) -o $@

$(BICREPO)/build/:
	mkdir -p $(BICREPO)/build

install: build/libbicycle.a
	sudo rm -rf /usr/local/include/bicycle
	sudo mkdir -p /usr/local/include/bicycle
	sudo cp include/*.h /usr/local/include/bicycle
	sudo cp build/libbicycle.a /usr/local/lib


.PHONY: clean
clean:
	rm -rf $(BICREPO)/build/*
