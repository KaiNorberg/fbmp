SRC = fbmp.c
OBJ = ${SRC:.c=.o}

CC = gcc

CFLAGS := $(shell pkg-config --cflags gimp-2.0)
LIBS := $(shell pkg-config --libs gimp-2.0)

all: build fbmp

fbmp: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS}

build:
	${CC} ${CFLAGS} ${LIBS} -o $@ -c $<

clean:
	rm -f fbmp ${OBJ}

install: all
	gimptool-2.0 --install-bin fbmp

uninstall:
	gimptool-2.0 --uninstall-bin fbmp
