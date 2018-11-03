CC = gcc
OBJS = main.o drone_movement.o central_proc.o log.o sim_manager.o
PROG = out

# ------------------------------------------------

all:							${PROG}

clean:
									rm ${OBJS} ${PROG}

# ------------------------------------------------
log.o:						log.c log.h

drone_movement.o:	drone_movement.c	drone_movement.h
									${CC}	drone_movement.c -c -o drone_movement.o

central_proc.o:		central_proc.c central_proc.h
									${CC} central_proc.c -c -o central_proc.o

main.o:						main.c central_proc.h
									${CC} main.c -c -o main.o

sim_manager.o:		sim_manager.c sim_manager.h

out:							${OBJS}
									${CC} ${OBJS} -pthread -Wall -o out -lm
									rm ${OBJS}
