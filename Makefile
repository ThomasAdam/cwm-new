# cwm makefile for BSD make and GNU make
# uses pkg-config, DESTDIR and PREFIX

PROG=		cwm-new

PREFIX?=	/usr/local

SRCS=		$(wildcard compat/*.c *.c)

OBJS=		$(patsubst %.c,%.o,$(SRCS))

CPPFLAGS+=	$(shell pkg-config --cflags fontconfig x11 xft xrandr libconfuse)

CFLAGS+=	-Wall -Wimplicit-int -Wno-pointer-sign -O0 -ggdb -D_GNU_SOURCE -I.

LDFLAGS+=	$(shell pkg-config --libs fontconfig x11 xft xrandr libconfuse)

MANPREFIX?=	${PREFIX}/share/man

QUIET_CC=	@echo '   ' CC $@;

all: ${PROG}

clean:
	rm -f *.o compat/*.o core* ${PROG}

${PROG}: ${OBJS}
	$(QUIET_CC)${CC} ${OBJS} ${CPPFLAGS} ${LDFLAGS} -o ${PROG}

.c.o:
	$(QUIET_CC)${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ $<

install: ${PROG}
	install -d ${DESTDIR}${PREFIX}/bin ${DESTDIR}${MANPREFIX}/man1 ${DESTDIR}${MANPREFIX}/man5
	install -m 755 cwm-new ${DESTDIR}${PREFIX}/bin
	install -m 644 cwm.1 ${DESTDIR}${MANPREFIX}/man1
	install -m 644 cwmrc.5 ${DESTDIR}${MANPREFIX}/man5
