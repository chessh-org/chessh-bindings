OUT = build/libchessh.a
SRC = $(wildcard src/*.c)
OBJ = $(subst src/,work/,$(subst .c,.o,$(SRC)))
LDFLAGS =
CFLAGS = -O2
CFLAGS += -fPIC -Wall -Wextra -Wpedantic -Wimplicit-fallthrough=2 -Werror

$(OUT): $(OBJ)
	$(AR) cvq $@ $(OBJ)

work/%.o: src/%.c src/chessh.h
	$(CC) -c $(CFLAGS) $< -o $@
