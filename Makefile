# Makefile for DiscretionTwo
# -*- indent-tabs-mode:t; -*-

CC=g++
LN=g++

CFLAGS = -Iinclude -Wall `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`

# use regular or debug cflags
#CFLAGS += -O3 -std=c++0x
CFLAGS += -O0 -g -std=c++0x

RELEASE_DIR=DiscretionRelease
RELEASE_FILE=DiscretionTwo

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
#################################
# Linux specific

#LDFLAGS += -s

else ifeq ($(UNAME),Darwin)
#################################
# Mac OS X specific

else
#################################
# Windows specific macros
RELEASE_FILE=DiscretionTwo.exe

endif
#################################

LDFLAGS += -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_net

.PHONY: all clean directories release dllDeps

all: $(RELEASE_DIR) $(RELEASE_DIR)/$(RELEASE_FILE) directories deps

release: $(RELEASE_DIR)/$(RELEASE_ARCHIVE)

directories: $(RELEASE_DIR) $(RELEASE_DIR)/resources

deps: $(RELEASE_DIR)/config.ini

$(RELEASE_DIR)/config.ini: config.ini
	cp -rv config.ini $(RELEASE_DIR)/config.ini

clean:
	rm -vrf $(RELEASE_DIR)
	rm -vf src/*.o
	rm -vf src/*.d

CPP_FILES =  $(wildcard src/*.cpp)
#CPP_FILES += $(wildcard lib/inih/*.cpp)

OBJS := $(CPP_FILES:.cpp=.o)

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

$(RELEASE_DIR)/resources:
	cp -rv resources $(RELEASE_DIR)/resources

# link
$(RELEASE_DIR)/$(RELEASE_FILE): $(OBJS)
	$(LN) $(OBJS) $(LDFLAGS) -o $(RELEASE_DIR)/$(RELEASE_FILE)


# pull in dependency info for existing .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info
%.o: %.cpp
	$(CC) -MMD -c $(CFLAGS) $< -o $@

