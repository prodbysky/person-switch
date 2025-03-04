CFLAGS = -ggdb -Wall -Werror -Lextern/raylib/src/ -Iextern/raylib/src/
LIBS = -lm -l:libraylib.a


main.c: src/main.c raylib player arena stage ecs enemy game_state bullet
	gcc src/main.c -o build/persona $(CFLAGS) $(LIBS) build/player.o build/arena.o build/stage.o build/ecs.o build/enemy.o build/game_state.o build/bullet.o

game_state: src/game_state.c src/game_state.h
	gcc src/game_state.c -c -o build/game_state.o $(CFLAGS) $(LIBS)

player: src/player.c src/player.h
	gcc src/player.c -c -o build/player.o $(CFLAGS) $(LIBS)

stage: src/stage.c src/stage.h
	gcc src/stage.c -c -o build/stage.o $(CFLAGS)

arena: src/arena.c src/arena.h
	gcc src/arena.c -c -o build/arena.o $(CFLAGS)

ecs: src/ecs.c src/ecs.h
	gcc src/ecs.c -c -o build/ecs.o $(CFLAGS)

enemy: src/enemy.c src/enemy.h
	gcc src/enemy.c -c -o build/enemy.o $(CFLAGS)

bullet: src/bullet.c src/bullet.h
	gcc src/bullet.c -c -o build/bullet.o $(CFLAGS)

raylib:
	make -C extern/raylib/src

