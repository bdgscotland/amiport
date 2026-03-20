# examples/common.mk — Shared build settings for amiport example ports
#
# Include from each example's Makefile:
#   include ../common.mk

CC = m68k-amigaos-gcc
CFLAGS = -O2 -noixemul -m68020 -Wall -I../../lib/posix-shim/include
LDFLAGS = -L../../lib/posix-shim -lamiport

SHIM_LIB = ../../lib/posix-shim/libamiport.a

# Native compiler for comparison builds
NATIVE_CC = cc
NATIVE_CFLAGS = -O2 -Wall
