CFLAGS=-g

check_istatd:	check_istatd.o dump.o checks.o socket.o xml.o
	gcc -o check_istatd check_istatd.o dump.o checks.o socket.o xml.o 

.c.o:
	gcc ${CFLAGS} -c $<
