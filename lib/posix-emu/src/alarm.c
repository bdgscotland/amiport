/*
 * alarm.c — alarm()/setitimer() emulation via timer.device
 *
 * Tier 2 emulation: approximate POSIX semantics with documented differences.
 * See amiport-emu/alarm.h for the emulation notice.
 */

#include <amiport-emu/alarm.h>

#if AMIPORT_EMU_ALARM_ENABLED

#include <amiport/signal.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/timer.h>

static struct MsgPort *timer_port = NULL;
static struct timerequest *timer_req = NULL;
static int timer_open = 0;
static int alarm_pending = 0;
static unsigned long alarm_secs = 0;

int amiport_emu_alarm_init(void)
{
    if (timer_open) {
        return 0; /* already initialized */
    }

    timer_port = CreateMsgPort();
    if (!timer_port) {
        return -1;
    }

    timer_req = (struct timerequest *)CreateIORequest(timer_port,
                    sizeof(struct timerequest));
    if (!timer_req) {
        DeleteMsgPort(timer_port);
        timer_port = NULL;
        return -1;
    }

    if (OpenDevice("timer.device", UNIT_VBLANK,
                   (struct IORequest *)timer_req, 0)) {
        DeleteIORequest((struct IORequest *)timer_req);
        DeleteMsgPort(timer_port);
        timer_req = NULL;
        timer_port = NULL;
        return -1;
    }

    timer_open = 1;
    return 0;
}

void amiport_emu_alarm_cleanup(void)
{
    if (!timer_open) {
        return;
    }

    if (alarm_pending) {
        AbortIO((struct IORequest *)timer_req);
        WaitIO((struct IORequest *)timer_req);
        alarm_pending = 0;
    }

    CloseDevice((struct IORequest *)timer_req);
    DeleteIORequest((struct IORequest *)timer_req);
    DeleteMsgPort(timer_port);
    timer_req = NULL;
    timer_port = NULL;
    timer_open = 0;
}

unsigned int amiport_emu_alarm(unsigned int seconds)
{
    unsigned int remaining = 0;

    if (!timer_open) {
        if (amiport_emu_alarm_init() != 0) {
            return 0;
        }
    }

    /* Cancel any pending alarm */
    if (alarm_pending) {
        AbortIO((struct IORequest *)timer_req);
        WaitIO((struct IORequest *)timer_req);
        remaining = (unsigned int)alarm_secs; /* approximate */
        alarm_pending = 0;
    }

    if (seconds == 0) {
        return remaining;
    }

    /* Set new alarm */
    alarm_secs = seconds;
    timer_req->tr_node.io_Command = TR_ADDREQUEST;
    timer_req->tr_time.tv_secs = seconds;
    timer_req->tr_time.tv_micro = 0;
    SendIO((struct IORequest *)timer_req);
    alarm_pending = 1;

    return remaining;
}

int amiport_emu_check_alarm(void)
{
    struct Message *msg;

    if (!alarm_pending || !timer_port) {
        return 0;
    }

    msg = GetMsg(timer_port);
    if (msg) {
        alarm_pending = 0;

        /* Fire the SIGALRM handler if one is set */
        amiport_raise(14); /* 14 = SIGALRM equivalent */

        return 1;
    }

    return 0;
}

#endif /* AMIPORT_EMU_ALARM_ENABLED */
