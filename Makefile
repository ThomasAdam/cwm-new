# cwm makefile for BSD make and GNU make
# uses pkg-config, DESTDIR and PREFIX

PROG=		cwm-new

PREFIX?=	/usr/local

SRCS=		$(wildcard compat/*.[ch] *.[ch]) y.tab.c

OBJS=		$(patsubst %.c,%.o,$(SRCS))

CPPFLAGS+=	$(shell pkg-config --cflags fontconfig x11 xft xinerama xrandr)

CFLAGS?=	-Wall -O2 -g -D_GNU_SOURCE

LDFLAGS+=	$(shell pkg-config --libs fontconfig x11 xft xinerama xrandr)

MANPREFIX?=	${PREFIX}/share/man

all: ${PROG}

clean:
	rm -f *.o compat/*.o core* ${PROG} y.tab.c

y.tab.c: parse.y
	yacc parse.y

${PROG}: ${OBJS}
	${CC} ${OBJS} ${CPPFLAGS} ${LDFLAGS} -o ${PROG}

.c.o:
	${CC} -c ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@ $<

install: ${PROG}
	install -d ${DESTDIR}${PREFIX}/bin ${DESTDIR}${MANPREFIX}/man1 ${DESTDIR}${MANPREFIX}/man5
	install -m 755 cwm ${DESTDIR}${PREFIX}/bin
	install -m 644 cwm.1 ${DESTDIR}${MANPREFIX}/man1
	install -m 644 cwmrc.5 ${DESTDIR}${MANPREFIX}/man5
