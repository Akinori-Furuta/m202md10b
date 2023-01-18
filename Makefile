# SPDX-License-Identifier: BSD-2-Clause
# Futaba M202MD10B control application makefile
#
# Copyright (C) 2023 afuruta@m7.dion.ne.jp

CC ?= gcc
CFLAGS ?= -Wall -O2

all: m202md10b

m202md10b: m202md10b.c
	$(CC) $(CFLAGS) -o $@ $<
