/*
 * streams/registry.h -- stub for AmigaOS port (PDR-010, no TLS/streams)
 * amiport: stream registry global init is a no-op stub.
 */
#ifndef INCLUDE_streams_registry_h__
#define INCLUDE_streams_registry_h__
/* git_stream_registry_global_init is a runtime init fn (int (*)(void)).
 * Provide it as a no-op function that returns 0 (success). */
static int git_stream_registry_global_init(void) { return 0; }
#endif
