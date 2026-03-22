---
name: vamos_wrapper_bug
description: Bug in toolchain/scripts/vamos wrapper - skip_next logic added -C arg value to args instead of discarding it
type: feedback
---

The vamos wrapper at `toolchain/scripts/vamos` had an inverted skip_next handler. When `-C ""` was passed, the empty string argument was added to the args array instead of being dropped. This caused vamos to receive `''` as the binary name, producing `AmiPathError: path='': can't derive cmdpaths`.

Fixed 2026-03-22 by removing `args+=("$arg")` from the `skip_next=1` branch — the arg should be silently discarded, not kept.

**Why:** The `-C` flag and its argument are a pair that should be entirely removed. The original code skipped the `-C` but forwarded its value. Classic off-by-one in a skip-next pattern.

**How to apply:** If Gate 2 vamos tests fail with "can't derive cmdpaths" or `path=''`, check whether the wrapper is correctly discarding the `-C` argument pair. Also verify with `bash -x /path/to/vamos` to trace the final invocation.
