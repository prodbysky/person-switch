CC = clang
CFLAGS = -Wall -Werror -Iextern/raylib/src/ -Iextern/ --extra-warnings
LDFLAGS = -Lextern/raylib/src/ 
LIBS = -lm -l:libraylib.a

SRC_DIR = src
BUILD_DIR = build
EXTERN_DIR = extern/raylib/src

TARGET = $(BUILD_DIR)/persona
OBJS = $(BUILD_DIR)/player.o $(BUILD_DIR)/stage.o \
       $(BUILD_DIR)/ecs.o $(BUILD_DIR)/enemy.o $(BUILD_DIR)/game_state.o \
       $(BUILD_DIR)/bullet.o $(BUILD_DIR)/timing_utilities.o $(BUILD_DIR)/wave.o $(BUILD_DIR)/pickup.o

BUILD_CONFIG = debug

ifeq ($(BUILD_CONFIG), debug)
CFLAGS += -ggdb
TARGET := $(TARGET)_debug
endif

ifeq ($(BUILD_CONFIG), release)
CFLAGS += -O3 -DRELEASE
TARGET := $(TARGET)_release
endif

all: $(TARGET)

$(TARGET): $(OBJS) $(SRC_DIR)/main.c
	$(CC) $(SRC_DIR)/main.c $(OBJS) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(LIBS) src/clay.o

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) -c $< -o $@ $(CFLAGS)

raylib:
	$(MAKE) -C $(EXTERN_DIR)

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)_debug $(TARGET)_release

.PHONY: all raylib clean

debug: raylib
	$(MAKE) BUILD_CONFIG=debug

release: raylib
	$(MAKE) BUILD_CONFIG=release
