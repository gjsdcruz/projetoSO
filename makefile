CC = gcc
CFLAGS = -pthread -Wall
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
