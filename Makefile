CC = gcc
CFLAGS = -ggdb -Wall -Werror -Iextern/raylib/src/
LDFLAGS = -Lextern/raylib/src/
LIBS = -lm -l:libraylib.a

SRC_DIR = src
BUILD_DIR = build
EXTERN_DIR = extern/raylib/src

TARGET = $(BUILD_DIR)/persona
OBJS = $(BUILD_DIR)/player.o $(BUILD_DIR)/arena.o $(BUILD_DIR)/stage.o \
       $(BUILD_DIR)/ecs.o $(BUILD_DIR)/enemy.o $(BUILD_DIR)/game_state.o \
       $(BUILD_DIR)/bullet.o

all: $(TARGET)

$(TARGET): $(OBJS) $(SRC_DIR)/main.c
	$(CC) $(SRC_DIR)/main.c $(OBJS) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) -c $< -o $@ $(CFLAGS) $(LIBS)

raylib:
	$(MAKE) -C $(EXTERN_DIR)

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)

# Phony targets
.PHONY: all raylib clean
