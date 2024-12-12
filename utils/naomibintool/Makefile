# KallistiOS ##version##
#
# utils/naomibintool/Makefile
# Copyright (C) 2020 Lawrence Sebald
#

all: naomibintool

naomibintool: naomibintool.c
#	$(CC) -DNO_LIBELF -o naomibintool naomibintool.c
	$(CC) -o naomibintool naomibintool.c -lelf

distclean:
	-rm -f naomibintool
