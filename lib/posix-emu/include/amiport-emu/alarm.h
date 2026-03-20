/*
 * amiport-emu/alarm.h — alarm()/setitimer() emulation for AmigaOS
 *
 * EMULATION NOTICE:
 * - Not asynchronous — requires cooperative checking via amiport_emu_check_alarm()
 * - No signal delivery interrupting blocking calls
 * - Resolution limited to timer.device granularity (~1ms on 68020+, ~20ms on 68000)
 * - Only one alarm can be active at a time
 *
 * See docs/posix-tiers.md "Tier 2 — alarm()/setitimer()" for full details.
 */

#ifndef AMIPORT_EMU_ALARM_H
#define AMIPORT_EMU_ALARM_H

#ifndef AMIPORT_EMU_ALARM_ENABLED
#define AMIPORT_EMU_ALARM_ENABLED 1
#endif

#if AMIPORT_EMU_ALARM_ENABLED

/*
 * amiport_emu_alarm() — Set a timer that fires after `seconds` seconds.
 *
 * Unlike POSIX alarm(), this does NOT deliver SIGALRM asynchronously.
 * The program must call amiport_emu_check_alarm() periodically to
 * check whether the timer has expired and invoke the handler.
 *
 * seconds: Time until alarm fires (0 to cancel)
 * Returns: Seconds remaining on previous alarm, or 0
 */
unsigned int amiport_emu_alarm(unsigned int seconds);

/*
 * amiport_emu_check_alarm() — Check if the alarm has fired.
 *
 * Call this from the main loop. If the alarm has expired, the
 * SIGALRM handler (if set via amiport_signal()) is invoked.
 *
 * Returns: 1 if alarm fired, 0 otherwise
 */
int amiport_emu_check_alarm(void);

/*
 * amiport_emu_alarm_init() — Initialize the timer.device for alarm support.
 * Call once at program startup if using alarm().
 *
 * Returns: 0 on success, -1 on error
 */
int amiport_emu_alarm_init(void);

/*
 * amiport_emu_alarm_cleanup() — Release timer.device resources.
 * Call at program exit.
 */
void amiport_emu_alarm_cleanup(void);

#endif /* AMIPORT_EMU_ALARM_ENABLED */

#endif /* AMIPORT_EMU_ALARM_H */
