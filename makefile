CC = gcc
OBJS = main.o drone_movement.o central_proc.o
PROG = out


all:							${PROG}

clean:
									rm ${OBJS} ${PROG}

drone_movement.o:	drone_movement.h	drone_movement.c
									${CC}	drone_movement.c -c -o drone_movement.o

central_proc.o:		central_proc.c central_proc.h
									${CC} central_proc.c -c -o central_proc.o

main.o:						main.c central_proc.h
									${CC} main.c -c -o main.o

out:							${OBJS}
									${CC} ${OBJS} -pthread -Wall -o out -lm
									rm ${OBJS}
