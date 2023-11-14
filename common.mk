TARGET=promesst
CC ?= gcc

all: $(TARGET)
.PHONY: all

$(TARGET): main.c
	$(CC) -o $(TARGET) main.c $(LFLAGS)

clean:
	-rm -f $(TARGET) promesst.zip promesst.opk
.PHONY: clean
