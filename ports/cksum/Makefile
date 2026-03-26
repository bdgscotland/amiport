# Makefile for cksum (amiport)
TARGET      = cksum
SOURCES     = ported/cksum.c ported/crc.c ported/crc32.c ported/sum.c
VERSION     = 1.0
DESCRIPTION = Display file checksums and sizes (FreeBSD)
AUTHOR      = The Regents of the University of California

CFLAGS += -Iported
VAMOS_STACK = 256

include ../common.mk

.PHONY: all clean test package compare
