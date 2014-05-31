# Straight forward Makefile to compile all examples in a row

INCLUDES=-I./Common -I./include
LIBS=-lGLESv2 -lEGL -lm -Llib -ltga -lX11 -lfreeimage 

CFLAGS=-g -Wall -std=gnu99 -D_DEBUG
#CFLAGS=-g -Wall -std=gnu99

LFLAGS=

NAME=texture

SRCS=./main.c getTime.c

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
