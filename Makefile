CC = gcc
CFLAGS = -O3 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -g \
         -I./include `pkg-config --cflags vterm termkey`
LDFLAGS = `pkg-config --libs vterm termkey`

SRC = $(wildcard src/*.c)
OUT = inline

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)
	rm -rf $(OUT).dSYM

clean:
	rm -f $(OUT)

.PHONY: all clean
