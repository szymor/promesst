TARGET=promesst

all: $(TARGET)
.PHONY: all

$(TARGET): main.c
	gcc -o $(TARGET) main.c $(LFLAGS)

clean:
	-rm -f $(TARGET)
.PHONY: clean
