OUT = build/bindings.so
SRC = $(wildcard src/*.c)
OBJ = $(subst src/,work/,$(subst .c,.o,$(SRC)))
LDFLAGS =
CFLAGS = -O2
CFLAGS += -fPIC

$(OUT): $(OBJ)
	$(CC) -shared $(OBJ) -o $@

work/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@
