 # Makefile for Discretion2 
 
CC=g++
LN=g++

CFLAGS = -Iinclude -Wall `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`

# use regular or debug cflags
CFLAGS += -O3 -std=c++0x
#CFLAGS += -O0 -ggdb

RELEASE_DIR=DiscretionRelease
RELEASE_FILE=Discretion2

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
#################################
# Linux specific

LDFLAGS += -s

else ifeq ($(UNAME),Darwin)
#################################
# Mac OS X specific

else
#################################
# Windows specific macros
RELEASE_FILE=Discretion2.exe

endif
#################################

LDFLAGS += -lSDL2 -lSDL2_image

.PHONY: all clean releaseDir directories release dllDeps
 
all: releaseDir $(RELEASE_DIR)/$(RELEASE_FILE) directories

releaseDir: $(RELEASE_DIR)

release: $(RELEASE_DIR)/$(RELEASE_ARCHIVE)

directories: releaseDir $(RELEASE_DIR)/graphics

clean:
	rm -vrf $(RELEASE_DIR)
	rm -vf src/*.o
	rm -vf src/*.d

CPP_FILES =  $(wildcard src/*.cpp)
#CPP_FILES += $(wildcard lib/inih/*.cpp)

OBJS := $(CPP_FILES:.cpp=.o)

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)
	
$(RELEASE_DIR)/graphics:
	cp -rv graphics $(RELEASE_DIR)/graphics

# link
$(RELEASE_DIR)/$(RELEASE_FILE): $(OBJS)
	$(LN) $(OBJS) $(LDFLAGS) -o $(RELEASE_DIR)/$(RELEASE_FILE)


# pull in dependency info for existing .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info
%.o: %.cpp
	$(CC) -MMD -c $(CFLAGS) $< -o $@



