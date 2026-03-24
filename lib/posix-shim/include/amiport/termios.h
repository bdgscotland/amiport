/*
 * amiport/termios.h — Minimal termios shim for AmigaOS
 *
 * Maps tcgetattr/tcsetattr/cfmakeraw to AmigaOS SetMode() for
 * raw/cooked console mode switching. Sufficient for terminal programs
 * (less, nano, vim) that need character-at-a-time input.
 *
 * AmigaOS has no termios — console mode is binary (raw or cooked)
 * controlled via SetMode(fh, 0|1). This shim provides the POSIX API
 * so ported programs compile without source changes.
 */

#ifndef AMIPORT_TERMIOS_H
#define AMIPORT_TERMIOS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal termios struct — only c_lflag is meaningful */
struct termios {
    unsigned long c_iflag;   /* input modes (unused) */
    unsigned long c_oflag;   /* output modes (unused) */
    unsigned long c_cflag;   /* control modes (unused) */
    unsigned long c_lflag;   /* local modes — ECHO and ICANON */
    unsigned char c_cc[20];  /* control characters (unused) */
};

/* c_lflag bits */
#define ECHO    0x0001
#define ICANON  0x0002
#define ISIG    0x0004
#define IEXTEN  0x0008
#define ECHONL  0x0010

/* c_iflag bits (stubs — not used on AmigaOS) */
#define IGNBRK  0x0001
#define BRKINT  0x0002
#define PARMRK  0x0004
#define ISTRIP  0x0008
#define INLCR   0x0010
#define IGNCR   0x0020
#define ICRNL   0x0040
#define IXON    0x0080

/* c_oflag bits (stubs) */
#define OPOST   0x0001

/* c_cflag bits (stubs) */
#define CSIZE   0x0030
#define CS8     0x0030
#define PARENB  0x0040

/* tcsetattr 'when' argument */
#define TCSANOW     0
#define TCSADRAIN   1
#define TCSAFLUSH   2

/* VMIN/VTIME indices into c_cc */
#define VMIN    0
#define VTIME   1

/* Control character indices (stub values — AmigaOS has no termios) */
#define VERASE  2
#define VKILL   3
#define VWERASE 4
#define VLNEXT  5
#define VDSUSP  6
#define VSTOP   7
#define VSTART  8
#define VDISCARD 9
#define VERASE2 10

/* Speed type (unused but referenced by some programs) */
typedef unsigned long speed_t;
typedef unsigned long tcflag_t;
typedef unsigned char cc_t;

/* Get current terminal attributes */
int tcgetattr(int fd, struct termios *t);

/* Set terminal attributes — maps to SetMode() */
int tcsetattr(int fd, int optional_actions, const struct termios *t);

/* Configure struct for raw mode (clear ECHO, ICANON, etc.) */
void cfmakeraw(struct termios *t);

/* Speed functions (stubs — no serial port control) */
speed_t cfgetispeed(const struct termios *t);
speed_t cfgetospeed(const struct termios *t);
int cfsetispeed(struct termios *t, speed_t speed);
int cfsetospeed(struct termios *t, speed_t speed);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_TERMIOS_H */
