/*
 * NO_UPSTREAM.c -- Placeholder file.
 *
 * amigit is an amiport-native CLI written from scratch on top of
 * lib/libgit2/libgit2.a. There is no upstream git source to preserve
 * in this directory because upstream git's command dispatch is built
 * on fork()/execvp() with 53 call sites in run-command.c alone --
 * structurally infeasible on 68k AmigaOS 3.x.
 *
 * See docs/pdr/010-amigit-on-libgit2.md for the full rationale, and
 * ports/amigit/PORT.md for the implementation history.
 *
 * This file exists only to satisfy the port-directory hygiene rule
 * that every port has an original/ directory with at least one .c
 * file. It is never compiled (see Makefile OBJECTS list) and is not
 * part of the amigit binary.
 */

/* Intentionally empty -- see block comment above. */
