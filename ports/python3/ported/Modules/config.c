/* Module configuration — CPython 3.11 for AmigaOS
 *
 * amiport: Hand-crafted static module table.
 * All modules are built-in (no dynamic loading on AmigaOS).
 */

#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mandatory core modules */
extern PyObject* PyMarshal_Init(void);
extern PyObject* PyInit__imp(void);
extern PyObject* PyInit_gc(void);
extern PyObject* PyInit__ast(void);
extern PyObject* PyInit__tokenize(void);
extern PyObject* _PyWarnings_Init(void);
extern PyObject* PyInit__string(void);

/* I/O */
extern PyObject* PyInit__io(void);

/* Essential modules for import/startup */
extern PyObject* PyInit_posix(void);
extern PyObject* PyInit_errno(void);
extern PyObject* PyInit__signal(void);
extern PyObject* PyInit__codecs(void);
extern PyObject* PyInit__weakref(void);
extern PyObject* PyInit__collections(void);
extern PyObject* PyInit__sre(void);
extern PyObject* PyInit__functools(void);
extern PyObject* PyInit__operator(void);
extern PyObject* PyInit_itertools(void);
extern PyObject* PyInit_atexit(void);
extern PyObject* PyInit__stat(void);
extern PyObject* PyInit__thread(void);
extern PyObject* PyInit__tracemalloc(void);
extern PyObject* PyInit_faulthandler(void);
extern PyObject* PyInit__abc(void);
extern PyObject* PyInit_time(void);
extern PyObject* PyInit__symtable(void);

/* Useful stdlib modules (built-in for performance) */
extern PyObject* PyInit_math(void);
extern PyObject* PyInit_cmath(void);
extern PyObject* PyInit__struct(void);
extern PyObject* PyInit_binascii(void);
extern PyObject* PyInit__json(void);
extern PyObject* PyInit__bisect(void);
extern PyObject* PyInit__heapq(void);

struct _inittab _PyImport_Inittab[] = {
    /* Core */
    {"marshal", PyMarshal_Init},
    {"_imp", PyInit__imp},
    {"_ast", PyInit__ast},
    {"_tokenize", PyInit__tokenize},
    {"builtins", NULL},
    {"sys", NULL},
    {"gc", PyInit_gc},
    {"_warnings", _PyWarnings_Init},
    {"_string", PyInit__string},

    /* I/O — mandatory for print(), open(), file ops */
    {"_io", PyInit__io},

    /* OS interface */
    {"posix", PyInit_posix},
    {"errno", PyInit_errno},
    {"_signal", PyInit__signal},
    {"time", PyInit_time},

    /* Import machinery */
    {"_codecs", PyInit__codecs},
    {"_weakref", PyInit__weakref},
    {"_collections", PyInit__collections},
    {"_sre", PyInit__sre},
    {"_functools", PyInit__functools},
    {"_operator", PyInit__operator},
    {"itertools", PyInit_itertools},
    {"_stat", PyInit__stat},
    {"_abc", PyInit__abc},
    {"_symtable", PyInit__symtable},

    /* Runtime */
    {"atexit", PyInit_atexit},
    {"_thread", PyInit__thread},
    /* amiport: disabled -- may corrupt memory list on 68k */
    /* {"_tracemalloc", PyInit__tracemalloc}, */
    /* {"faulthandler", PyInit_faulthandler}, */

    /* Useful builtins */
    {"math", PyInit_math},
    {"cmath", PyInit_cmath},
    {"_struct", PyInit__struct},
    {"binascii", PyInit_binascii},
    {"_json", PyInit__json},
    {"_bisect", PyInit__bisect},
    {"_heapq", PyInit__heapq},

    /* Sentinel */
    {0, 0}
};

#ifdef __cplusplus
}
#endif
