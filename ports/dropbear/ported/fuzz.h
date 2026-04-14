/* amiport: fuzz.h stub -- fuzzing disabled */
#ifndef DROPBEAR_FUZZ_H_
#define DROPBEAR_FUZZ_H_

#ifndef DROPBEAR_FUZZ
#define DROPBEAR_FUZZ 0
#endif

#if DROPBEAR_FUZZ
/* not building fuzzer */
#error "Fuzzing not supported on AmigaOS"
#endif

#endif
