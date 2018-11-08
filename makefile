CC = gcc
OBJS = main.o drone_movement.o central_proc.o warehouse.o log.o sim_manager.o
PROG = out

# ------------------------------------------------

all:							${PROG}

clean:
									rm ${OBJS} ${PROG}

# ------------------------------------------------
log.o:						log.c log.h
									${CC} log.c -c -o log.o

drone_movement.o:	drone_movement.c	drone_movement.h
									${CC} drone_movement.c -c -o drone_movement.o

central_proc.o:		central_proc.c central_proc.h
									${CC} central_proc.c -c -o central_proc.o

warehouse.o:			warehouse.c warehouse.h
									${CC} warehouse.c -c -o warehouse.o

main.o:						main.c
									${CC} main.c -c -o main.o

sim_manager.o:		sim_manager.c sim_manager.h
									${CC} sim_manager.c -c -o sim_manager.o

out:							${OBJS}
									${CC} ${OBJS} -pthread -Wall -o out -lm
									rm ${OBJS}
