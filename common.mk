TARGET=promesst

all: $(TARGET)
.PHONY: all

$(TARGET): main.c stb_gl.h
	gcc -o $(TARGET) main.c $(LFLAGS)

clean:
	-rm -f $(TARGET)
.PHONY: clean
