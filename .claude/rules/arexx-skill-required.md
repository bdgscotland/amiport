Paths: **/*.rexx, toolchain/templates/*.rexx

# ARexx Skill Required

**When writing or modifying ANY `.rexx` file, you MUST invoke the `/write-arexx` skill FIRST.**

ARexx has critical gotchas that cause silent failures on AmigaOS:
- Multi-level compound variables (`stem.i.n`) do NOT work -- only single-level (`stem.i`)
- Non-ASCII characters (em-dashes, smart quotes) cause "Error 8: Unrecognized token"
- `\=` may not be recognized -- use `~=` for not-equal
- No short-circuit evaluation in boolean expressions
- `$` in Execute scripts gets expanded as AmigaDOS variables

The `/write-arexx` skill loads the full reference and gotchas checklist. Loading it prevents the class of bugs that caused weeks of misdiagnosed "exit hang" issues (crash-patterns #9).

Do NOT skip this step. Do NOT write ARexx from memory.
