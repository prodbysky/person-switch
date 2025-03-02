main.c: src/main.c raylib
	gcc src/main.c -o persona -lm -Lextern/raylib/src/ -l:libraylib.a -Iextern/raylib/src/

raylib:
	make -C extern/raylib/src

