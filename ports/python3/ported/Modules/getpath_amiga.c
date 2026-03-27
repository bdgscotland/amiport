/*
 * getpath_amiga.c -- Path configuration for CPython on AmigaOS
 *
 * amiport: Minimal path config that sets Python's search paths
 * to AmigaOS-style locations. No frozen getpath.py needed.
 *
 * Path layout on AmigaOS:
 *   PYTHON:         -- prefix (assign or directory)
 *   PYTHON:lib      -- stdlib .py files (Phase 8)
 *   PROGDIR:        -- program directory (AmigaOS built-in)
 *
 * For Phases 3-7 (no stdlib on disk), the interpreter works with
 * frozen/builtin modules only. module_search_paths can be empty.
 */

#include "Python.h"
#include "pycore_initconfig.h"
#include "pycore_pathconfig.h"

PyStatus
_PyConfig_InitPathConfig(PyConfig *config, int compute_path_config)
{
    PyStatus status;

    if (!compute_path_config) {
        return _PyStatus_OK();
    }

    /* Set prefix and exec_prefix to PYTHON: (Amiga assign) */
    status = PyConfig_SetString(config, &config->prefix, L"PYTHON:");
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = PyConfig_SetString(config, &config->exec_prefix, L"PYTHON:");
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = PyConfig_SetString(config, &config->base_prefix, L"PYTHON:");
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = PyConfig_SetString(config, &config->base_exec_prefix, L"PYTHON:");
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* Set module search paths */
    if (!config->module_search_paths_set) {
        config->module_search_paths_set = 1;

        /* Add PYTHON:lib as the stdlib search path */
        status = PyWideStringList_Append(&config->module_search_paths,
                                         L"PYTHON:lib");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        /* Add current directory */
        status = PyWideStringList_Append(&config->module_search_paths,
                                         L"");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    /* stdlib_dir -- where the standard library lives */
    status = PyConfig_SetString(config, &config->stdlib_dir, L"PYTHON:lib");
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* amiport: Set sys.executable so site module doesn't crash with
     * AttributeError: module 'sys' has no attribute 'executable' */
    if (config->executable == NULL || config->executable[0] == L'\0') {
        status = PyConfig_SetString(config, &config->executable,
                                     L"WORK:python3");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    /* Set platlibdir for sys.platlibdir */
    status = PyConfig_SetString(config, &config->platlibdir, L"lib");
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}
