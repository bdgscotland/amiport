/*
 * transports/smart.h -- stub for AmigaOS port (PDR-010, no network)
 * amiport: git_smart__ofs_delta_enabled is a global flag used by the
 * pack negotiation code. Define it here as an extern; it will be
 * provided by a stub in the archive.
 */
#ifndef INCLUDE_transports_smart_h__
#define INCLUDE_transports_smart_h__

/* Global flag: whether to use OFS_DELTA objects in pack negotiation.
 * Referenced by settings.c SET_OPTION handler for
 * GIT_OPT_ENABLE_OFS_DELTA. Default is 1 (enabled). */
extern int git_smart__ofs_delta_enabled;

#endif /* INCLUDE_transports_smart_h__ */
