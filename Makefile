TARGET=promesst

all: $(TARGET)
.PHONY: all

$(TARGET): main.c stb_gl.h stb_image.c stb_sdl2graph.h
	gcc -o $(TARGET) main.c -lopengl32 -lglu32 -lmingw32 -lSDL2main -lSDL2

clean:
	-rm -f $(TARGET)
.PHONY: clean
