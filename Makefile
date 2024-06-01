TARGET = regvm
#TARGET = regvm.html
LIBS = -lm `pkg-config --cflags --libs sdl2`
CFLAGS= -o regvm -o 'regvm.html' -g -Wall -std=c99 -pedantic -I ./include `pkg-config --cflags --libs sdl2` -D DEBUG
#CFLAGS= -flto -O3 -o regvm.html -sUSE_SDL=2 -I ./include
LDFLAGS =
CC= gcc

.PHONY: default all clean

default: clean $(TARGET)
all: default

#OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
#HEADERS = $(wildcard *.h)

SRC = $(wildcard src/*.c) $(wildcard src/*/*.c)
OBJ = $(addprefix obj/,$(notdir $(SRC:.c=.o)))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c | obj
	$(CC) $< -c $(CFLAGS) -o $@

obj/%.o: src/*/%.c | obj
	$(CC) $< -c $(CFLAGS) -o $@

obj:
	mkdir obj

.PRECIOUS: $(TARGET) $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -Wall $(LIBS) -o $@

clean:
	-rm -f ./obj/*.o
	-rm -f $(TARGET)

