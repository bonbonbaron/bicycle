DBG=-g
STD=-std=c++20

BICREPO := $(shell git rev-parse --show-toplevel)
BICSRCS := main bicycle Bar Blackboard Body ColorPalette ConditionRegistry Config Dialogue Dice Entity Menu MidiPlayer Graph Grid Timer Personality Window WindowManager 
BICINCS := bicycle ColorPalette Dialogue Menu Constellation MidiPlayer Window WindowManager  # TODO fix this
BICINCS := $(BICINCS:%=$(BICREPO)/include/%.h)

BICOBJS := $(BICSRCS:%=$(BICREPO)/build/%.o)
BICTGT := o  # for now
all: $(BICTGT)

$(BICTGT): $(BICOBJS)
	g++ $(DBG) $(STD) $(BICOBJS) $(shell ncursesw6-config --libs) -lfluidsynth -lyaml-cpp -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp $(BICREPO)/include/%.h 
	g++ $(DBG) $(STD) -c $< -I$(BICREPO)/include $(shell ncursesw6-config --cflags ) -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp 
	g++ $(DBG) $(STD) -c $< -I$(BICREPO)/include $(shell ncursesw6-config --cflags ) -o $@

$(BICREPO)/build/:
	mkdir -p $(BICREPO)/build

install: $(BICTGT)
	sudo rm -rf /usr/local/include/bicycle
	sudo mkdir -p /usr/local/include/bicycle
	sudo cp include/*.h /usr/local/include/bicycle
	sudo cp $(BICTGT) /usr/local/bin


.PHONY: clean
clean:
	rm -rf $(BICREPO)/build/*
