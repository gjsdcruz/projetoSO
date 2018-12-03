CC = gcc
CFLAGS = -D_REENTRANT -pthread -Wall -g
LDFLAGS = -lm
OBJS = main.o drone_movement.o central_proc.o warehouse.o log.o sim_manager.o
PROG = out

# ------------------------------------------------

all:							${PROG}

clean_all:
									rm -f ${OBJS} ${PROG}

clean:
									rm -f ${OBJS}

# ------------------------------------------------

${PROG}:					${OBJS}
									${CC} ${CFLAGS} -o ${PROG} ${OBJS} ${LDFLAGS}
