# Makefile for factor (amiport)

TARGET      = factor
SOURCES     = ported/factor.c ported/pattern.c ported/pr_tbl.c
VERSION     = 1.30
DESCRIPTION = Factor a number (OpenBSD)
AUTHOR      = The Regents of the University of California

CFLAGS += -Iported

AMINET_CAT = util/misc

include ../common.mk

.PHONY: all clean test package compare
