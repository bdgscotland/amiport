/*
 * termios.c — Minimal termios shim for AmigaOS
 *
 * amiport: Maps POSIX terminal mode control to AmigaOS SetMode().
 * AmigaOS console has two modes: CON: (cooked, line-buffered) and
 * RAW: (character-at-a-time, no echo). This is a binary switch —
 * there's no fine-grained control like Unix termios provides.
 *
 * tcgetattr() saves the current mode.
 * tcsetattr() checks c_lflag: if ECHO and ICANON are cleared, sets raw mode.
 * cfmakeraw() clears the flags that indicate cooked mode.
 */

#include <amiport/termios.h>

#include <string.h>

#ifdef __AMIGA__
#include <proto/dos.h>
#endif

/* Track whether we're currently in raw mode */
static int raw_mode_active = 0;
static struct termios saved_termios;
static int saved_valid = 0;

int tcgetattr(int fd, struct termios *t)
{
    (void)fd;
    if (!t) return -1;

    if (saved_valid) {
        *t = saved_termios;
    } else {
        /* Default: cooked mode (ECHO + ICANON set) */
        t->c_iflag = ICRNL | IXON;
        t->c_oflag = OPOST;
        t->c_cflag = CS8;
        t->c_lflag = ECHO | ICANON | ISIG | IEXTEN;
        memset(t->c_cc, 0, sizeof(t->c_cc));
        t->c_cc[VMIN] = 1;
        t->c_cc[VTIME] = 0;
        saved_termios = *t;
        saved_valid = 1;
    }
    return 0;
}

int tcsetattr(int fd, int optional_actions, const struct termios *t)
{
    int want_raw;

    (void)fd;
    (void)optional_actions;
    if (!t) return -1;

    /* Determine mode from c_lflag: if ECHO and ICANON are both cleared,
     * the program wants raw mode. Otherwise cooked. */
    want_raw = !(t->c_lflag & (ECHO | ICANON));

#ifdef __AMIGA__
    if (want_raw && !raw_mode_active) {
        SetMode(Input(), 1);  /* Switch to RAW mode */
        raw_mode_active = 1;
    } else if (!want_raw && raw_mode_active) {
        SetMode(Input(), 0);  /* Switch to CON (cooked) mode */
        raw_mode_active = 0;
    }
#endif

    return 0;
}

void cfmakeraw(struct termios *t)
{
    if (!t) return;
    t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    t->c_oflag &= ~OPOST;
    t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(CSIZE | PARENB);
    t->c_cflag |= CS8;
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}

speed_t cfgetispeed(const struct termios *t)
{
    (void)t;
    return 9600;  /* Stub — no serial control */
}

speed_t cfgetospeed(const struct termios *t)
{
    (void)t;
    return 9600;
}

int cfsetispeed(struct termios *t, speed_t speed)
{
    (void)t;
    (void)speed;
    return 0;
}

int cfsetospeed(struct termios *t, speed_t speed)
{
    (void)t;
    (void)speed;
    return 0;
}
