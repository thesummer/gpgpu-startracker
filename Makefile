# Straight forward Makefile to compile all examples in a row

INCLUDES=-I./Common -I./include
LIBS=-lGLESv2 -lEGL -lm -Llib -ltga

CFLAGS=-g -Wall -D_DEBUG -std=gnu++11
#CFLAGS=-Wall -std=gnu++11

LFLAGS=

NAME=texture

SRCS=./main.cpp ogles.cpp phase.cpp labelPhase.cpp reductionPhase.cpp getTime.c

OBJ=$(SRCS:.cpp=.o)


default: all

all: texture

clean:
	rm -fv ${NAME}
	rm -fv *.o ./Common/*.o

texture: ${HEADERS} ${OBJ}
	g++ ${OBJ} -o ${NAME} ${LFLAGS} ${INCLUDES} ${LIBS}
	
.cpp.o:
	g++ ${CFLAGS} ${INCLUDES} -c $<  -o $@
