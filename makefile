prog: main.o entity.o
	gcc main.o entity.o -o prog -lSDL -lSDL_image -lSDL_mixer -lSDL_ttf -lm -g

main.o: main.c entity.h
	gcc -c main.c -g `sdl-config --cflags`

entity.o: entity.c entity.h
	gcc -c entity.c -g `sdl-config --cflags`

clean:
	rm -f *.o prog
