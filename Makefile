# Straight forward Makefile to compile all examples in a row

INCLUDES=-I./Common -I./include
LIBS=-lGLESv2 -lEGL -lm -lX11 -lfreeimage -Llib -ltga

CFLAGS=-g -Wall -std=c99 -D_DEBUG

LFLAGS=

NAME=texture

SRCS=./main.c

COMMONSRC=./Common/esShader.c    \
          ./Common/esTransform.c \
          ./Common/esShapes.c    \
          ./Common/esUtil.c
HEADERS=./Common/esUtil.h

SRCS+= ${COMMONSRC}

OBJ=$(SRCS:.c=.o)

default: all

all: texture

clean:
	rm -fv ${NAME}
	rm -fv *.o ./Common/*.o

texture: ${HEADERS} ${OBJ}
	gcc ${OBJ} -o ${NAME} ${LFLAGS} ${INCLUDES} ${LIBS}
	
.c.o:
	gcc ${CFLAGS} ${INCLUDES} -c $<  -o $@