CFLAGS = -ggdb -Wall -Werror -Lextern/raylib/src/ -Iextern/raylib/src/
LIBS = -lm -l:libraylib.a

main.c: src/main.c raylib player
	gcc src/main.c -o build/persona $(CFLAGS) $(LIBS) build/player.o

player: src/player.c src/player.h
	gcc src/player.c -c -o build/player.o $(CFLAGS) $(LIBS)

raylib:
	make -C extern/raylib/src

