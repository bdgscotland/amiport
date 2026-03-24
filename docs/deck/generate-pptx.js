const pptxgen = require("pptxgenjs");

const pres = new pptxgen();
pres.layout = "LAYOUT_WIDE"; // 13.3" × 7.5"
pres.author = "Duncan Bowring";
pres.title = "The Hard Direction";

// Colors
const BG = "0C0A08";
const CARD = "1C1814";
const BORDER = "2E2A24";
const AMBER = "D4A020";
const AMBER_B = "F0C848";
const CREAM = "EDE6D8";
const GRAY = "C0B8A8";
const DIM = "887E70";
const GREEN = "48A848";
const RED = "D04838";
const YELLOW = "C8A830";

function addSlide(titleText, bodyLines, opts = {}) {
  const slide = pres.addSlide();
  slide.background = { color: BG };

  // Title
  slide.addText(titleText, {
    x: 0.6, y: 0.4, w: 12, h: 1.4,
    fontSize: opts.titleSize || 30,
    fontFace: "Arial",
    color: CREAM,
    bold: true,
    valign: "top",
  });

  // Body
  const textObjs = bodyLines.map((line, i) => {
    if (line.startsWith("##")) {
      return { text: line.replace("##", "").trim(), options: { bold: true, color: AMBER, fontSize: 16, breakLine: true, paraSpaceBefore: 12 } };
    }
    if (line.startsWith("**")) {
      return { text: line.replace(/\*\*/g, ""), options: { bold: true, color: CREAM, fontSize: 14, breakLine: true, paraSpaceBefore: 6 } };
    }
    if (line === "") {
      return { text: " ", options: { fontSize: 8, breakLine: true } };
    }
    return { text: line, options: { color: GRAY, fontSize: 14, breakLine: true, paraSpaceBefore: 2 } };
  });

  slide.addText(textObjs, {
    x: 0.6, y: 1.9, w: 12, h: 5,
    valign: "top",
    fontFace: "Arial",
  });

  return slide;
}

// S1: Title
(() => {
  const s = pres.addSlide();
  s.background = { color: BG };
  s.addText("The Hard Direction", { x: 0.8, y: 1.5, w: 10, h: 2, fontSize: 52, bold: true, color: CREAM, fontFace: "Arial" });
  s.addText("If AI Can Port Code to 1992,\nIt Can Modernize Your Legacy Apps Too.", { x: 0.8, y: 3.5, w: 10, h: 1.5, fontSize: 22, color: GRAY, fontFace: "Arial" });
  s.addText("DUNCAN BOWRING, ACCENTURE", { x: 0.8, y: 5.5, w: 6, h: 0.5, fontSize: 14, color: AMBER, fontFace: "Courier New", charSpacing: 2 });
})();

// S2: Setup
addSlide("What if one person directing an AI could build an entire\ncode migration platform — and then that platform ships\nreal software?", [
  "",
  "Not a tool that suggests code. A production system with 16 specialized agents,",
  "mandatory quality gates, automated testing on real hardware, and a knowledge base",
  "that gets smarter with every migration.",
  "",
  "I built one — then I used it to ship 14 programs to real users on 30-year-old hardware.",
], { titleSize: 34 });

// S3: What I Built
addSlide("I directed one AI to build an entire migration platform.\nThen the platform shipped real software.", [
  "Claude Code wrote 23,000+ lines of production tooling. 16 specialized AI agents.",
  "Four compatibility libraries totalling 6,556 lines of handwritten C.",
  "An 86MB knowledge base — 13,455 machine-readable API reference files.",
  "428 automated tests. 14 programs shipped to real users.",
  "",
  "**Claude built the system. The system uses Claude to ship software. Human directs, AI builds.**",
  "",
  "The question for this room: what happens when we apply this approach",
  "to the migration engagements we sell to clients?",
]);

// S4: Four Principles
addSlide("Four principles. One pipeline.", [
  "The same foundations we apply in Code Archaeology and Discovery Driven Engineering —",
  "automated in an AI pipeline.",
  "",
  "## 1. Current State Truth",
  "Scan every dependency. Classify by difficulty. No assumptions.",
  "",
  "## 2. Progressive Complexity",
  "Automate the mechanical. Escalate the hard. Stop for judgment.",
  "",
  "## 3. Real Target Verification",
  "Test on the actual platform. Not a simulation. Not a proxy.",
  "",
  "## 4. Self-Learning",
  "Every migration makes the next one faster. Bugs become prevention rules.",
]);

// S5: Classification
(() => {
  const s = pres.addSlide();
  s.background = { color: BG };
  s.addText("Classification is what made it possible.", { x: 0.6, y: 0.4, w: 12, h: 0.8, fontSize: 30, bold: true, color: CREAM, fontFace: "Arial" });
  s.addText("Classify every API dependency by difficulty before writing code.\nThis let the AI handle 90 functions autonomously while stopping for the 3 that needed a human.", { x: 0.6, y: 1.3, w: 12, h: 0.8, fontSize: 14, color: GRAY, fontFace: "Arial" });

  // Green tier
  s.addShape(pres.ShapeType.rect, { x: 0.6, y: 2.4, w: 4, h: 1.3, fill: { color: "0A1A0A" }, line: { color: GREEN, width: 2 } });
  s.addText("90", { x: 0.8, y: 2.5, w: 1.2, h: 1, fontSize: 48, bold: true, color: GREEN, fontFace: "Arial", valign: "middle" });
  s.addText("Tier 1: Wrap — automated\nDirect equivalent. AI handles it.", { x: 2.2, y: 2.5, w: 2.2, h: 1, fontSize: 13, color: GRAY, fontFace: "Arial", valign: "middle" });

  // Yellow tier
  s.addShape(pres.ShapeType.rect, { x: 4.9, y: 2.4, w: 3.5, h: 1.3, fill: { color: "1A1A0A" }, line: { color: YELLOW, width: 2 } });
  s.addText("~10", { x: 5.1, y: 2.5, w: 1.2, h: 1, fontSize: 48, bold: true, color: YELLOW, fontFace: "Arial", valign: "middle" });
  s.addText("Tier 2: Emulate — approximate\nCaveats documented. Human reviews.", { x: 6.5, y: 2.5, w: 1.8, h: 1, fontSize: 13, color: GRAY, fontFace: "Arial", valign: "middle" });

  // Red tier
  s.addShape(pres.ShapeType.rect, { x: 8.7, y: 2.4, w: 4, h: 1.3, fill: { color: "1A0A0A" }, line: { color: RED, width: 2 } });
  s.addText("✗", { x: 8.9, y: 2.5, w: 1.2, h: 1, fontSize: 48, bold: true, color: RED, fontFace: "Arial", valign: "middle" });
  s.addText("Tier 3: Redesign — pipeline stops\nHuman architectural decision required.", { x: 10.3, y: 2.5, w: 2.2, h: 1, fontSize: 13, color: GRAY, fontFace: "Arial", valign: "middle" });

  // Callout
  s.addShape(pres.ShapeType.rect, { x: 0.5, y: 4.2, w: 12.3, h: 1.2, fill: { color: CARD }, line: { color: AMBER, width: 2 } });
  s.addText("Code Archaeology + Discovery Driven Engineering + Classification", { x: 0.8, y: 4.3, w: 11.7, h: 0.5, fontSize: 16, bold: true, color: AMBER_B, fontFace: "Arial" });
  s.addText("We automated all three. Each migration makes the next one faster.", { x: 0.8, y: 4.85, w: 11.7, h: 0.4, fontSize: 14, color: GRAY, fontFace: "Arial" });
})();

// S6-S14: Content slides
const contentSlides = [
  ["The stress test: two platforms that share zero APIs.", [
    "Source: POSIX/Linux 2024 — 64GB+ RAM, virtual memory, protected processes",
    "Target: AmigaOS 1992 — 2MB RAM, 7MHz 68EC020, no memory protection",
    "", "Motorola 68EC020: 24-bit address bus only, no MMU.",
    "The crash debugger can't even run on stock hardware.",
    "", "Most problems clients face in legacy modernization exist here in a more extreme form.",
    "The engineering patterns that solve them transfer directly.",
  ]],
  ["30 years of manual porting. We matched it in a weekend.", [
    "The Commodore Amiga (1985). 500K–800K active users today.",
    "85,191 packages on Aminet built over three decades.",
    "38 uploads in 2 weeks from the entire global community.",
    "", "**We submitted 10 packages in 3 days — 26% of Aminet's two-week volume.**",
    "", "Each went through a 7-stage pipeline: analyzed across 57 transformation rules,",
    "cross-compiled in Docker, tested on vamos + FS-UAE with real AmigaOS 3.1,",
    "memory-audited, performance-optimized, and published with full source.",
    "", "423+ downloads in 3 days. The previous best grep was from 2006.",
    "Ours is from this week — with 53 tests and full source documentation.",
  ]],
  ["14 programs. 428 tests. All passing on real hardware.", [
    "grep: 53 tests, 7 source files, regex, recursive directory walking",
    "Lua: 65 tests, coroutines, metatables, 100KB strings",
    "jq: full JSON processor with custom memory allocator",
    "diff: unified output, recursive comparison, context formatting",
    "patch: apply and reverse unified diffs",
    "", "7 live on Aminet where real users run them on 30-year-old hardware.",
    "", "cal 90 · cut 77 · grep 57 · sed 54 · head 53 · tee 47 · yes 45 downloads",
  ]],
  ["Every failure has a system-level fix.", [
    "## 1. Classify Before You Code",
    "Tier 1/2/3. 90 automated, ~10 approximated, pipeline stops for redesign.",
    "", "## 2. Inject Domain Knowledge",
    "86MB: 13,455 API files, 646-page CPU manual, 700+ runtime functions.",
    "", "## 3. Verify Every Output",
    "Docker → vamos (100ms/test) → FS-UAE (real AmigaOS 3.1) → mandatory audits.",
    "", "## 4. Accumulate Learning",
    "19 crash patterns growing. fclose(stdin) caught once → flagged at analysis forever.",
  ]],
  ["Classify. Bridge. Pipeline.", [
    "**Adapter Layer:** 6,556 lines across 4 libraries. One wrapper per function.",
    "posix-shim 3,413 · posix-emu 1,285 · console-shim 1,146 · bsdsocket-shim 712",
    "Started at 20 functions → now 90 after 14 ports.",
    "", "**7-Stage Pipeline with 3 feedback loops:**",
    "Loop 1: Build fails → Transform (extend-shim writes missing function)",
    "Loop 2: Crash → Debug agent → Build (Enforcer → addr2line → KB → fix)",
    "Loop 3: Review changes → Build (memory/perf fixes re-verified)",
    "", "57 transformation rules · 21 CI steps · Every change re-verified",
  ]],
  ["Why it doesn't fall apart at scale.", [
    "**16 Constrained Agents**",
    "source-analyzer r-- · code-transformer rw- · memory-checker r-- (mandatory)",
    "debug-agent rwx (5 iter cap) · aminet-publisher HUMAN APPROVAL",
    "Enforced by infrastructure hooks, not prompt instructions.",
    "", "**3-Layer Enforcement**",
    "Runtime: 8 hook scripts on every tool call",
    "Git: doc consistency, metadata, test coverage at commit time",
    "CI: 21-step pipeline builds all 14 ports on every push",
    "", "**Living Knowledge Base**",
    "19 crash patterns + 20+ pitfalls. getopt_long → Pattern #17 → auto-fixed.",
  ]],
  ["When the system gets it wrong, it marks its own mistakes.", [
    "**The Debunked Pattern** — Pipeline identified exit() as hanging. Built _exit() workaround.",
    "Actual cause: non-ASCII byte in ARexx test script. Pattern #9 marked DEBUNKED.",
    "The system self-corrects.",
    "", "**The grep Re-Port** — FS-UAE testing caught two critical bugs before publication:",
    "exit codes always 0 (missing braces) and grep -R skipping subdirectories",
    "(d_type never populated on AmigaOS). Both now caught at analysis.",
    "", "**The Desktop Killer** — fclose(stdin) crashes the entire OS.",
    "Now flagged before any code is written.",
  ]],
  ["grep went from OpenBSD source to live on Aminet.\nI made one decision.", [
    "Analyze (25 issues) → Transform (57 rules) → Build (3 iterations) → Test (53/53)",
    "→ Memory (clean) → Perf (3-5x speedup) → Re-port (2 bugs fixed) → Re-test (53/53)",
    "→ Package (LHA) → HUMAN approve → LIVE (57 downloads)",
    "", "All automated. No human intervention. One decision.",
    "", "amiport.platesteel.net — package browser, CLI installer (amiget install grep),",
    "stats dashboard, Amiga HTML 3.2 page. All built by the same AI.",
    "", "**Human on the loop. Everything else moves at machine speed.**",
  ]],
  ["The patterns are proven. The target was harder than yours.", [
    "**What we overcame:** fewer APIs (invented 90 shims), 2MB RAM, no memory protection,",
    "custom crash detection from scratch, 30-year-old manuals → 13,455 markdown files.",
    "", "**What your clients have:** rich Java/Spring APIs, 64GB+ RAM, GC, virtual memory,",
    "ASAN/Valgrind/JVM debuggers, modern structured documentation.",
    "", "**Every pattern transfers:**",
    "Classification → COBOL triage · Adapter → CICS/DB2/VSAM mapping",
    "Crash KB → incident knowledge base · Hooks → compliance gates",
    "", "**Constrain the agents. Inject the domain knowledge. Enforce the quality gates.**",
    "", "Build the pipeline.",
    "", "Duncan Bowring, Accenture",
  ]],
];

for (const [title, lines] of contentSlides) {
  addSlide(title, lines);
}

pres.writeFile({ fileName: "/Users/duncan/Downloads/The_Hard_Direction.pptx" })
  .then(() => console.log("Saved: ~/Downloads/The_Hard_Direction.pptx — 14 slides"))
  .catch(err => console.error(err));
