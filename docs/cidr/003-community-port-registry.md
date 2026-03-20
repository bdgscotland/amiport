# CIDR-003: Community Port Registry and Sharing

## Status

Exploring

## Date

2026-03-19

## Idea

Create a registry where users can share their successful ports — both the porting configurations (transformation rules, shim extensions) and the resulting binaries. Think npm/crates.io but for Amiga ports.

## Motivation

Each successful port generates knowledge: which transformations worked, what shim extensions were needed, what compiler flags were required. Sharing this accelerates future ports of similar software and avoids duplicate effort.

## Open Questions

- What format should port configurations be shared in?
- How do we handle versioning (same source, different Amiga targets)?
- Where does it live — GitHub releases, Aminet, a custom registry?
- Quality control — how do we verify shared ports actually work?

## Early Thinking

A `port.json` manifest in each port directory could capture: source URL, version, target OS, transformations applied, build flags, test results. A GitHub-based registry (separate repo or branch) could aggregate these manifests. Aminet integration would reach the existing Amiga community.

## Next Steps

- Define the port manifest format
- Get several successful ports to understand what metadata matters
- Survey existing Amiga software distribution channels (Aminet, OS4Depot)
