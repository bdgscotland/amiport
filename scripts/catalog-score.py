#!/usr/bin/env python3
"""
catalog-score.py — Porting Tech Tree scoring, validation, and status tool.

Commands:
  --validate          Validate catalog.json against schema
  --score             Compute readiness scores for all candidates
  --status            Print catalog dashboard (tier distribution, top unlocks, next batch)
  --next N            Print top N ready candidates for dispatch
  --diff BEFORE.json  Show changes between before and current catalog
  --hardware          Show hardware fit matrix for all candidates/ported

Filters (combine with --next or --status):
  --category CAT      Filter by amiport_category (cli, scripting, console_ui, network)
  --profile PROF      Filter by hardware profile (stock_a1200, a1200_accel, a4000_030)

Usage:
  python3 scripts/catalog-score.py --status
  python3 scripts/catalog-score.py --next 5 --category cli --profile a1200_accel
  python3 scripts/catalog-score.py --score
  python3 scripts/catalog-score.py --validate
"""

import json
import sys
import os
import argparse
from pathlib import Path

CATALOG_PATH = Path(__file__).parent.parent / "data" / "catalog.json"

# Readiness score weights
W_SHIM_COVERAGE = 0.30
W_SOURCE_SIMPLICITY = 0.15
W_TIER1_RATIO = 0.25
W_BLOCKER_FACTOR = 0.20
W_NO_EXT_DEPS = 0.10

# Readiness tier thresholds
TIER_READY = 0.85
TIER_FEASIBLE = 0.60
TIER_BLOCKED = 0.30

# Publishable licenses
PUBLISHABLE_LICENSES = {
    "MIT", "ISC", "BSD-2-Clause", "BSD-3-Clause", "Apache-2.0",
    "GPL-2.0", "GPL-3.0", "LGPL-2.1", "LGPL-3.0",
    "Lucent-1.02", "Public Domain", "Unlicense", "0BSD",
    "GPL-2.0-or-later", "GPL-3.0-or-later",
}


def load_catalog():
    """Load catalog.json, return parsed data."""
    if not CATALOG_PATH.exists():
        print(f"Error: {CATALOG_PATH} not found. Run catalog analysis first.", file=sys.stderr)
        sys.exit(1)
    with open(CATALOG_PATH) as f:
        return json.load(f)


def save_catalog(data):
    """Atomic write: write to .tmp then rename."""
    tmp_path = CATALOG_PATH.with_suffix(".json.tmp")
    with open(tmp_path, "w") as f:
        json.dump(data, f, indent=2)
        f.write("\n")
    tmp_path.rename(CATALOG_PATH)


def compute_readiness(candidate, shim_functions):
    """Compute readiness score for a candidate. Returns (score, tier)."""
    analysis = candidate.get("analysis")
    if not analysis or candidate.get("analysis_status") != "complete":
        return 0.0, "unanalyzed"

    # shim_coverage: % of needed shims already implemented
    missing = analysis.get("missing_shims", [])
    if not missing:
        shim_coverage = 1.0
    else:
        implemented = sum(
            1 for s in missing
            if shim_functions.get(s, {}).get("status") == "implemented"
        )
        shim_coverage = implemented / len(missing)

    # source_simplicity: 0.0-1.0 (higher = simpler)
    source_simplicity = analysis.get("source_complexity", 0.5)

    # tier1_ratio: % of POSIX calls that are Tier 1
    tier_mix = analysis.get("posix_tier_mix", {})
    total_calls = sum(tier_mix.values()) if tier_mix else 0
    tier1_ratio = tier_mix.get("tier1", 0) / total_calls if total_calls > 0 else 1.0

    # blocker_factor: 1.0 if no Tier 3, 0.5 if workaround exists, 0.0 if hard blocker
    blocking = analysis.get("blocking_shims", [])
    if not blocking:
        blocker_factor = 1.0
    elif analysis.get("blocker_workaround"):
        blocker_factor = 0.5
    else:
        blocker_factor = 0.0

    # no_ext_deps: 1.0 if no external library dependencies
    deps = analysis.get("dependencies", [])
    no_ext_deps = 1.0 if not deps else 0.0

    score = (
        W_SHIM_COVERAGE * shim_coverage
        + W_SOURCE_SIMPLICITY * source_simplicity
        + W_TIER1_RATIO * tier1_ratio
        + W_BLOCKER_FACTOR * blocker_factor
        + W_NO_EXT_DEPS * no_ext_deps
    )
    score = max(0.0, min(1.0, score))

    if score >= TIER_READY:
        tier = "ready"
    elif score >= TIER_FEASIBLE:
        tier = "feasible"
    elif score >= TIER_BLOCKED:
        tier = "blocked"
    else:
        tier = "infeasible"

    return round(score, 3), tier


def classify_hardware_fit(candidate, profile_name, profile):
    """Classify hardware fit for a candidate against a profile."""
    analysis = candidate.get("analysis")
    if not analysis:
        return "unknown"

    est_binary = analysis.get("estimated_binary_kb", 0)
    est_data = analysis.get("estimated_data_kb", 0)
    est_stack = analysis.get("estimated_stack_kb", 0)

    total_needed = est_binary + est_data + est_stack
    total_ram = profile["total_ram_kb"]
    max_stack = profile["max_practical_stack_kb"]

    # Stack risk check
    if est_stack > max_stack:
        return "stack_risk"

    # FPU check (placeholder — candidates would need an fpu_required field)
    # For now, all our candidates are integer-only

    # RAM fit check
    if total_needed > total_ram:
        return "exceeds"

    # Needs Fast RAM?
    if est_binary + est_data > profile["chip_ram_kb"] and profile["fast_ram_kb"] == 0:
        return "exceeds"

    if est_binary + est_data > profile["chip_ram_kb"]:
        return "needs_fast_ram"

    headroom = (total_ram - total_needed) / total_ram
    if headroom < 0.20:
        return "tight"

    # For stock_a1200 with only estimates AND the program uses >25% of RAM,
    # downgrade fits to tight (estimation error could matter at this scale).
    # Tiny programs (<100KB total) obviously fit on 2MB — don't penalize them.
    if profile_name == "stock_a1200" and not candidate.get("_has_measured"):
        if total_needed > total_ram * 0.25:
            return "tight"

    return "fits"


def build_shim_index(catalog):
    """Build reverse index: for each shim, which candidates need it."""
    shim_unlocks = {}
    for candidate in catalog.get("candidates", []):
        analysis = candidate.get("analysis", {})
        for shim in analysis.get("missing_shims", []):
            if shim not in shim_unlocks:
                shim_unlocks[shim] = []
            shim_unlocks[shim].append(candidate["id"])

    # Update catalog shim_functions with computed unlocks
    for shim_name, shim_data in catalog.get("shim_functions", {}).items():
        shim_data["unlocks"] = shim_unlocks.get(shim_name, [])
        shim_data["unlocks_count"] = len(shim_data["unlocks"])

    return shim_unlocks


def cmd_validate(catalog):
    """Validate catalog structure."""
    errors = []

    required_top = ["version", "hardware_profiles", "aminet_category_map", "shim_functions", "candidates", "ported"]
    for field in required_top:
        if field not in catalog:
            errors.append(f"Missing top-level field: {field}")

    for i, c in enumerate(catalog.get("candidates", [])):
        if "id" not in c:
            errors.append(f"Candidate {i}: missing 'id'")
        if "amiport_category" not in c:
            errors.append(f"Candidate {i} ({c.get('id', '?')}): missing 'amiport_category'")

    for i, p in enumerate(catalog.get("ported", [])):
        if "id" not in p:
            errors.append(f"Ported {i}: missing 'id'")

    if errors:
        print(f"VALIDATION FAILED — {len(errors)} errors:")
        for e in errors:
            print(f"  - {e}")
        return 1
    else:
        nc = len(catalog.get("candidates", []))
        np = len(catalog.get("ported", []))
        ns = len(catalog.get("shim_functions", {}))
        print(f"VALID — {nc} candidates, {np} ported, {ns} shim functions")
        return 0


def cmd_status(catalog, category=None, profile=None):
    """Print catalog dashboard."""
    candidates = catalog.get("candidates", [])
    ported = catalog.get("ported", [])
    shim_fns = catalog.get("shim_functions", {})

    if category:
        candidates = [c for c in candidates if c.get("amiport_category") == category]

    # Tier distribution
    tiers = {"ready": 0, "feasible": 0, "blocked": 0, "infeasible": 0, "unanalyzed": 0}
    for c in candidates:
        tier = c.get("readiness", {}).get("tier", "unanalyzed")
        tiers[tier] = tiers.get(tier, 0) + 1

    print("=" * 60)
    print("  PORTING TECH TREE — CATALOG STATUS")
    print("=" * 60)
    print()
    cat_label = f" (category: {category})" if category else ""
    print(f"  Candidates: {len(candidates)}{cat_label}")
    print(f"  Ported:     {len(ported)}")
    print()
    print(f"  Ready:      {tiers['ready']:3d}  ████████")
    print(f"  Feasible:   {tiers['feasible']:3d}  ████░░░░")
    print(f"  Blocked:    {tiers['blocked']:3d}  ██░░░░░░")
    print(f"  Infeasible: {tiers['infeasible']:3d}  ░░░░░░░░")
    if tiers["unanalyzed"]:
        print(f"  Unanalyzed: {tiers['unanalyzed']:3d}")
    print()

    # Top shim unlock opportunities
    unimplemented = [
        (name, data) for name, data in shim_fns.items()
        if data.get("status") != "implemented" and data.get("unlocks_count", 0) > 0
    ]
    unimplemented.sort(key=lambda x: x[1].get("unlocks_count", 0), reverse=True)

    if unimplemented:
        print("  Top shim unlock opportunities:")
        for name, data in unimplemented[:5]:
            count = data.get("unlocks_count", 0)
            tier = data.get("tier", "?")
            complexity = data.get("complexity", "?")
            print(f"    {name:15s}  T{tier}  {complexity:8s}  unlocks {count} programs")
        print()

    # Shim coverage
    total_shims = len(shim_fns)
    implemented = sum(1 for s in shim_fns.values() if s.get("status") == "implemented")
    if total_shims > 0:
        pct = implemented * 100 // total_shims
        print(f"  Shim coverage: {implemented}/{total_shims} ({pct}%)")
    print()

    # Next batch recommendation
    ready = [c for c in candidates if c.get("readiness", {}).get("tier") == "ready"]
    if profile:
        ready = [c for c in ready if c.get("hardware_fit", {}).get(profile) in ("fits", "tight")]
    ready.sort(key=lambda c: c.get("readiness", {}).get("score", 0), reverse=True)
    if ready:
        prof_label = f" for {profile}" if profile else ""
        print(f"  Next batch recommendation{prof_label}:")
        for c in ready[:5]:
            score = c.get("readiness", {}).get("score", 0)
            print(f"    {c['id']:15s}  score={score:.3f}  {c.get('description', '')[:40]}")
    elif candidates:
        print("  No ready candidates. Top unlock opportunities above.")
    else:
        print("  No candidates analyzed yet. Run /catalog-analyze to populate.")

    print()
    print("=" * 60)


def cmd_next(catalog, n, category=None, profile=None):
    """Print top N ready candidates for dispatch."""
    candidates = catalog.get("candidates", [])

    # Filter
    ready = [
        c for c in candidates
        if c.get("readiness", {}).get("tier") == "ready"
        and c.get("status") == "candidate"
        and c.get("publishable", True)
    ]
    if category:
        ready = [c for c in ready if c.get("amiport_category") == category]
    if profile:
        ready = [c for c in ready if c.get("hardware_fit", {}).get(profile) in ("fits", "tight")]

    # Sort by score descending
    ready.sort(key=lambda c: c.get("readiness", {}).get("score", 0), reverse=True)

    if not ready:
        print("No ready candidates match the criteria.")
        # Show what would unlock candidates
        blocked = [c for c in candidates if c.get("readiness", {}).get("tier") in ("blocked", "feasible")]
        if blocked:
            all_missing = {}
            for c in blocked:
                for s in c.get("analysis", {}).get("missing_shims", []):
                    all_missing[s] = all_missing.get(s, 0) + 1
            top = sorted(all_missing.items(), key=lambda x: x[1], reverse=True)[:3]
            if top:
                print("Top shims that would unlock candidates:")
                for s, count in top:
                    print(f"  {s}: would help {count} candidates")
        return 1

    for c in ready[:n]:
        score = c.get("readiness", {}).get("score", 0)
        hw = c.get("hardware_fit", {})
        hw_str = " ".join(f"{k}={v}" for k, v in hw.items()) if hw else "unknown"
        print(f"{c['id']:15s}  score={score:.3f}  cat={c.get('amiport_category', '?'):10s}  {c.get('description', '')[:35]}")
        print(f"{'':15s}  source={c.get('upstream', {}).get('source', '?')}  {hw_str}")

    return 0


def cmd_score(catalog):
    """Compute and update readiness scores for all candidates."""
    shim_fns = catalog.get("shim_functions", {})
    candidates = catalog.get("candidates", [])
    updated = 0

    for c in candidates:
        score, tier = compute_readiness(c, shim_fns)
        if "readiness" not in c:
            c["readiness"] = {}
        c["readiness"]["score"] = score
        c["readiness"]["tier"] = tier
        updated += 1

    # Also compute hardware fit
    profiles = catalog.get("hardware_profiles", {})
    for c in candidates:
        if "hardware_fit" not in c:
            c["hardware_fit"] = {}
        for prof_name, prof_data in profiles.items():
            c["hardware_fit"][prof_name] = classify_hardware_fit(c, prof_name, prof_data)

    # Build shim reverse index
    build_shim_index(catalog)

    save_catalog(catalog)
    print(f"Scored {updated} candidates. Catalog saved.")


def cmd_diff(catalog, before_path):
    """Show changes between before and current catalog."""
    with open(before_path) as f:
        before = json.load(f)

    before_ported = {p["id"] for p in before.get("ported", [])}
    after_ported = {p["id"] for p in catalog.get("ported", [])}
    new_ports = after_ported - before_ported

    before_candidates = {c["id"]: c for c in before.get("candidates", [])}
    after_candidates = {c["id"]: c for c in catalog.get("candidates", [])}

    tier_changes = []
    for cid, after in after_candidates.items():
        before_c = before_candidates.get(cid)
        if before_c:
            bt = before_c.get("readiness", {}).get("tier", "?")
            at = after.get("readiness", {}).get("tier", "?")
            if bt != at:
                tier_changes.append((cid, bt, at))

    print("=" * 50)
    print("  CATALOG DIFF REPORT")
    print("=" * 50)
    if new_ports:
        print(f"\n  New ports: {', '.join(sorted(new_ports))}")
    if tier_changes:
        print(f"\n  Tier changes:")
        for cid, bt, at in tier_changes:
            print(f"    {cid}: {bt} -> {at}")
    if not new_ports and not tier_changes:
        print("\n  No changes.")
    print()


def main():
    parser = argparse.ArgumentParser(description="Porting Tech Tree catalog tool")
    parser.add_argument("--validate", action="store_true", help="Validate catalog")
    parser.add_argument("--score", action="store_true", help="Compute readiness scores")
    parser.add_argument("--status", action="store_true", help="Print dashboard")
    parser.add_argument("--next", type=int, metavar="N", help="Top N ready candidates")
    parser.add_argument("--diff", metavar="BEFORE", help="Diff against previous catalog")
    parser.add_argument("--hardware", action="store_true", help="Hardware fit matrix")
    parser.add_argument("--category", help="Filter by amiport_category")
    parser.add_argument("--profile", help="Filter by hardware profile")
    parser.add_argument("--catalog", help="Path to catalog.json (default: data/catalog.json)")
    args = parser.parse_args()

    global CATALOG_PATH
    if args.catalog:
        CATALOG_PATH = Path(args.catalog)

    catalog = load_catalog()

    if args.validate:
        sys.exit(cmd_validate(catalog))
    elif args.score:
        cmd_score(catalog)
    elif args.status:
        cmd_status(catalog, category=args.category, profile=args.profile)
    elif args.next is not None:
        sys.exit(cmd_next(catalog, args.next, category=args.category, profile=args.profile))
    elif args.diff:
        cmd_diff(catalog, args.diff)
    elif args.hardware:
        # Hardware matrix
        profiles = catalog.get("hardware_profiles", {})
        print(f"{'Name':15s}", end="")
        for p in profiles:
            print(f"  {p:15s}", end="")
        print()
        for c in catalog.get("candidates", []):
            hw = c.get("hardware_fit", {})
            print(f"{c['id']:15s}", end="")
            for p in profiles:
                print(f"  {hw.get(p, '?'):15s}", end="")
            print()
        for p in catalog.get("ported", []):
            print(f"{p['id']:15s}", end="")
            for prof in profiles:
                if p.get("measured_binary_kb"):
                    print(f"  {'verified':15s}", end="")
                else:
                    print(f"  {'ported':15s}", end="")
            print()
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
