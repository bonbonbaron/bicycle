DBG=-g
STD=-std=c++20

BICREPO := $(shell git rev-parse --show-toplevel)
BICSRCS := main bicycle Config m/Entity c/Trigger c/Timer c/SshInput v/ColorPalette v/Window c/WindowManager v/Dialogue m/Blackboard # m/Body c/Input v/Bar c/Dice v/Menu c/MidiPlayer v/Graph v/Grid 
# BICINCS := bicycle Image ColorPalette Dialogue Menu MidiPlayer Window WindowManager  # TODO fix this
BICINCS := $(BICINCS:%=$(BICREPO)/include/%.h)

BICOBJS := $(BICSRCS:%=$(BICREPO)/build/%.o)
BICTGT := o  # for now
all: $(BICTGT)

# Consider these compiler options when you're ready to hard-core optimize.
# g++ -O3 -march=armv8-a -mcpu=cortex-a72 -mtune=cortex-a72 -mfpu=neon-fp-armv8 -mfloat-abi=hard ...
$(BICTGT): $(BICOBJS)
	g++ -Wall -fvisibility=default $(DBG) -levdev $(STD) $(BICOBJS) $(shell ncursesw6-config --libs) -lfluidsynth -lyaml-cpp -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp $(BICREPO)/include/%.h 
	g++ -Wall -fvisibility=default $(DBG) $(STD) -c $< -I$(BICREPO)/include $(shell ncursesw6-config --cflags ) -o $@

$(BICREPO)/build/%.o: $(BICREPO)/src/%.cpp ${BICREPO}/build/m ${BICREPO}/build/v ${BICREPO}/build/c 
	g++ -Wall -fvisibility=default $(DBG) $(STD) -c $< -I$(BICREPO)/include $(shell ncursesw6-config --cflags ) -o $@

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
