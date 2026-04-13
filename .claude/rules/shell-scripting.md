# Shell Scripting — zsh/bash Portability

The user's shell is **zsh** on macOS. Scripts and Bash tool invocations that assume bash semantics will silently misbehave in zsh because of word-splitting differences.

## The Rule

When iterating a shell variable containing space-separated values, **DO NOT** rely on bash's implicit word splitting of unquoted variable expansion. In zsh, `$VAR` does not split on whitespace by default; the entire variable is treated as a single value.

### Broken (silent failure in zsh)

```bash
EXCLUDE_FILES='clone.c fetch.c remote.c'
for f in $EXCLUDE_FILES; do    # zsh: one iteration with f='clone.c fetch.c remote.c'
    rm "$f"                     # zsh: fails, "no such file" -- whole string is one name
done
```

In bash this works because `$EXCLUDE_FILES` is subject to word splitting on unquoted expansion. In zsh it does not. The loop runs once with the whole string as a single value, every test fails, and **no output is produced** — the script completes silently with zero visible errors.

### Correct — use an array

```bash
EXCLUDE_FILES=(clone.c fetch.c remote.c)
for f in "${EXCLUDE_FILES[@]}"; do
    rm "$f"
done
```

Arrays work identically in bash and zsh, and the `"${VAR[@]}"` expansion is safe under every IFS and shell-option combination.

### Also correct — zsh word-split operator

```bash
EXCLUDE_FILES='clone.c fetch.c remote.c'
for f in ${=EXCLUDE_FILES}; do  # zsh: ${=VAR} forces word splitting
    rm "$f"
done
```

This works in zsh but is **not portable** to bash (bash treats `${=VAR}` as a parameter expansion error or as a default-value operator depending on context). Prefer arrays unless you are certain the script only runs under zsh.

### Also correct — force IFS word-split (portable)

```bash
EXCLUDE_FILES='clone.c fetch.c remote.c'
set -- $EXCLUDE_FILES           # relies on IFS, which includes space by default
for f; do
    rm "$f"
done
```

Works in both shells, but `set --` clobbers positional parameters so the rest of the function loses `$1`...`$N`. Only use inside a subshell or in simple scripts.

## Why This Rule Exists

**2026-04-13, porting libgit2 Phase 2:** A source-copy script used `for ex in $EXCLUDE_FILES` to skip network files during the libgit2 upstream tree copy. The script "succeeded" under zsh without errors, but **every single file in the exclude list was copied anyway** because the loop iterated once with the entire string as a single non-matching filename. The failure surfaced 30+ minutes later when enumerating objects for the Makefile — at which point 13 files that should have been pruned (clone.c, fetch.c, remote.c, transport.c, and their headers) had to be manually deleted. The bug is invisible to the eye: the loop runs, no errors occur, and the script appears correct.

This is bash-vs-zsh's single most common portability trap. It does not produce warnings, does not fail fast, and is only detectable by verifying side effects.

## How to Apply

- **When writing Bash tool invocations** that involve iterating a list of filenames, flags, or args from a variable:
  - Default to using an array (`VAR=(a b c)` + `"${VAR[@]}"`).
  - If the input is a single string you cannot reformat, use `${=VAR}` (zsh only) or `set -- $VAR` (portable).
  - Never use `for x in $VAR` with an unquoted space-separated string variable.

- **When verifying the script ran correctly**, check the actual side effects (`ls`, `wc -l`, `find`), never just "no error reported". A silently-broken loop produces no error but also produces no work. Always count the output.

- **Prefer `rsync --exclude` or explicit `cp` lists** over loop-based file filtering when copying a subset of a directory tree. `rsync -av --exclude='clone.*' --exclude='fetch.*' src/ dst/` is shell-agnostic and visible at a glance.

## Related

- `.claude/rules/use-pipeline-agents.md` — when scripted file manipulation becomes complex, dispatch an agent instead of rolling your own loop.
- Project environment notes in CLAUDE.md and at session start confirm the shell is zsh on Darwin.
