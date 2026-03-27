/* Minimal main program -- everything is loaded from the library */

#include "Python.h"

#ifdef MS_WINDOWS
int
wmain(int argc, wchar_t **argv)
{
    return Py_Main(argc, argv);
}
#else
#include <stdio.h>
/* amiport: version string for AmigaOS Version command */
static const char *verstag = "$VER: python3 3.11.12 (26.03.2026)";
long __stack = 524288;  /* amiport: 512KB stack for Python eval recursion */
int
main(int argc, char **argv)
{
    return Py_BytesMain(argc, argv);
}
#endif
