/* ============================================================================
   gaming-detail.js -- per-game detail page renderer (vanilla JS)
   Reads the port id from <html data-game-id="...">, looks up port data,
   merges per-port overrides with generic makeDetails(), renders page.
   ============================================================================ */
(function () {
    'use strict';

    // ── Port data (kept in sync with gaming.js) ──────────────────────────────
    var PORTS = [
        { id: 'julius', name: 'Julius', game: 'Caesar III', year: 1998, originalPlatform: 'Windows', genre: 'City-builder', status: 'playable', fps: 42, targetFps: 60, audio: false, version: '1.7.0-ami3', size: '3.2 MB', featured: true, files: 360, sourceMods: 1, lastBuild: '2026-04-11', upstream: 'https://github.com/bvschaik/julius', palette: { bg: '#3a2f1c', fg: '#c8965a' }, blurb: "The 1998 classic city-builder -- set your citizens' wages, watch Mars smite heretics, delete your own aqueducts by accident. Never ran on a classic Amiga. Now it does." },
        { id: 'chocolate-doom', name: 'Chocolate Doom', game: 'Doom', year: 1993, originalPlatform: 'DOS', genre: 'FPS', status: 'playable', fps: 38, targetFps: 35, audio: true, version: '3.0.1-ami2', size: '2.1 MB', featured: true, files: 214, sourceMods: 3, lastBuild: '2026-03-28', upstream: 'https://github.com/bdgscotland/chocolate-doom', palette: { bg: '#2a0f0f', fg: '#cc4444' }, blurb: 'Historically-accurate Doom port. WAV sound effects through SDL2_mixer. E1M1 loads in 2.8 seconds on a stock A1200.' },
        { id: 'ccleste', name: 'Celeste Classic', game: 'Celeste Classic', year: 2018, originalPlatform: 'PICO-8', genre: 'Platformer', status: 'playable', fps: 60, targetFps: 60, audio: false, version: '1.0.1-ami1', size: '0.4 MB', featured: false, files: 12, sourceMods: 0, lastBuild: '2026-02-14', upstream: 'https://github.com/lemon32767/ccleste', palette: { bg: '#1a1a3a', fg: '#88aacc' }, blurb: 'First new-era game ever running on a classic 68k Amiga. PICO-8 to 68030 is a 2018 to 1990 time-warp.' },
        { id: '1oom', name: '1oom', game: 'Master of Orion', year: 1993, originalPlatform: 'DOS', genre: '4X Strategy', status: 'playable', fps: 43, targetFps: 60, audio: false, version: '1.0.2-ami1', size: '1.8 MB', featured: false, files: 187, sourceMods: 0, lastBuild: '2026-03-05', upstream: 'https://gitlab.com/KrzysztofBS/1oom', palette: { bg: '#0f0f2a', fg: '#8866cc' }, blurb: 'The original 4X. Galaxy generation in 480ms, ~43 FPS on a Vampire V4. Zero source modifications -- pure stdlib port.' },
        { id: 'sdlpop', name: 'SDLPoP', game: 'Prince of Persia', year: 1989, originalPlatform: 'DOS', genre: 'Platformer', status: 'wip', fps: 35, targetFps: 60, audio: false, version: '1.24-ami0', size: '1.1 MB', featured: false, files: 98, sourceMods: 2, lastBuild: '2026-04-02', upstream: 'https://github.com/NagyD/SDLPoP', palette: { bg: '#2a1a0a', fg: '#ddaa66' }, blurb: "Renders. Doesn't play. Keyboard input routes through OS4-style event pipe that 3.x doesn't have. Fix planned." },
        { id: 'vanilla-conquer', name: 'Vanilla Conquer', game: 'Command & Conquer', year: 1995, originalPlatform: 'Windows', genre: 'RTS', status: 'wip', fps: 22, targetFps: 30, audio: false, version: '0.3-ami0', size: '8.4 MB', featured: false, files: 1804, sourceMods: 11, lastBuild: '2026-04-09', upstream: 'https://github.com/TheAssemblyArmada/Vanilla-Conquer', palette: { bg: '#1a2a0a', fg: '#88aa44' }, blurb: '430,000 lines of C++11 cross-compiled with bebbo-gcc. Main menu loads. GDI Mission 1 loads. Second mission crashes on briefing. You can watch the intro though.' },
        { id: 'fheroes2', name: 'fheroes2', game: 'Heroes of Might and Magic II', year: 1996, originalPlatform: 'DOS/Windows', genre: 'Turn-based Strategy', status: 'wip', fps: 30, targetFps: 60, audio: false, version: '1.1.3-ami0', size: '4.5 MB', featured: false, files: 892, sourceMods: 5, lastBuild: '2026-03-22', upstream: 'https://github.com/ihhub/fheroes2', palette: { bg: '#1a2a1a', fg: '#88cc88' }, blurb: 'The greatest fantasy-TBS ever made, re-implemented from scratch in modern C++. Asset loading works. Combat UI renders. Turns crash after move 7.' },
        { id: 'reminiscence', name: 'REminiscence', game: 'Flashback', year: 1992, originalPlatform: 'Amiga/DOS', genre: 'Cinematic Platformer', status: 'planned', fps: null, targetFps: 60, audio: false, version: '--', size: '--', featured: false, palette: { bg: '#2a0a1a', fg: '#cc6688' }, blurb: 'The coming-home port. Originally ran on the A500.' },
        { id: 'another-world', name: 'Another World', game: 'Another World', year: 1991, originalPlatform: 'Amiga', genre: 'Cinematic Platformer', status: 'planned', fps: null, targetFps: 60, audio: false, version: '--', size: '--', featured: false, palette: { bg: '#0a0a2a', fg: '#6688cc' }, blurb: 'Another coming-home port. Original ran on the A500 in 550KB.' },
        { id: 'opentyrian', name: 'OpenTyrian', game: 'Tyrian', year: 1995, originalPlatform: 'DOS', genre: "Shoot 'em up", status: 'planned', fps: null, targetFps: 60, audio: false, version: '--', size: '--', featured: false, palette: { bg: '#2a1a2a', fg: '#cc88cc' }, blurb: 'Vertical scroller with a fully reverse-engineered engine. Portable, asset-loading, should be straightforward.' }
    ];
    var PORTS_BY_ID = {};
    PORTS.forEach(function (p) { PORTS_BY_ID[p.id] = p; });

    // ── Julius hand-crafted overrides ────────────────────────────────────────
    var JULIUS_DETAILS = {
        about: [
            "Julius is a cross-platform re-implementation of Caesar III -- the 1998 Impressions/Sierra Roman city-builder. The original shipped for Windows 95 and Mac only, never touched the Amiga, and was widely considered impossible to port without source access. bvschaik's Julius project rebuilt the engine in clean C99 from scratch in 2017. This amiport build is that engine, cross-compiled for 68030 with a single source modification (see Patches).",
            "On a 68030/50MHz + RTG you'll see ~42 FPS on 160x160 maps. Vampire V4 pushes ~55. Save files round-trip with PC Julius 1.7 -- start a campaign on your desktop, continue on the Amiga. Audio is still missing; the MIDI bank is there but AHI routing isn't wired yet (planned for 1.8-ami0).",
            "Julius does not ship game data. You need a legitimate copy of Caesar III (the 2017 GOG.com release works great) and to copy c3.eng, c3.sg2, and the smk intro files to SYS:Games/Julius/data/ -- the launcher auto-detects the first install it finds."
        ],
        toolchain: 'bebbo-gcc 13.1.0 (m68k-amigaos)',
        sdl2: 'libSDL2-amigaos3 0.7.0',
        buildTime: '4m 12s (Ryzen 7 via Docker)',
        dataFiles: 'Caesar III data files (c3.eng, c3.sg2 + SG3 folder) -- copy from a legitimate install to SYS:Games/Julius/data/',
        requirements: [
            { k: 'CPU',      v: '68030 @ 50MHz' },
            { k: 'RAM',      v: '8MB Fast (recommend 32MB)' },
            { k: 'OS',       v: 'AmigaOS 3.0+' },
            { k: 'Graphics', v: 'RTG (CGX/P96) * 800x600 min' },
            { k: 'Disk',     v: '420MB (engine 3MB + data)' },
            { k: 'TCP',      v: 'Not required' }
        ],
        screenshots: [
            { caption: 'main menu',     ornamentTop: 'JULIUS * caesar iii' },
            { caption: 'city overview', ornamentTop: 'ROMA 190 BC * pop 2,840' },
            { caption: 'trade panel',   ornamentTop: 'TRADE * 12 routes' },
            { caption: 'overlay: food' }
        ],
        controls: [
            { action: 'Pan map',       keys: ['Arrow keys'] },
            { action: 'Fast pan',      keys: ['Shift', 'Arrow'] },
            { action: 'Rotate view',   keys: ['Home', 'End'] },
            { action: 'Advisors',      keys: ['F1'] },
            { action: 'Empire map',    keys: ['F2'] },
            { action: 'Messages',      keys: ['F3'] },
            { action: 'City overlays', keys: ['1', '-', '9'] },
            { action: 'Pause',         keys: ['P'] },
            { action: 'Game speed',    keys: ['[', ']'] },
            { action: 'Save / Load',   keys: ['Ctrl', 'S'] },
            { action: 'Screenshot',    keys: ['PrtSc'] },
            { action: 'Quit to WB',    keys: ['Ctrl', 'Q'] }
        ],
        issues: [
            { id: '#AMI-011', severity: 'medium', text: 'Aqueduct pathing stalls on maps exceeding 200x200 tiles. Workaround: keep maps <= 160x160.' },
            { id: '#AMI-019', severity: 'medium', text: 'No MIDI music -- AHI routing deferred until Julius 1.8-ami0.' },
            { id: '#AMI-024', severity: 'low',    text: 'Mouse cursor flickers during scroll on Picasso96 2.2e. Works on 2.4b+.' },
            { id: '#AMI-027', severity: 'low',    text: 'Smk intro video plays silently; upstream smacker decoder hardcodes 22050 Hz audio.' },
            { id: '#AMI-031', severity: 'high',   text: 'Memory leak on campaign end -- ~180KB per mission. Relaunch between campaigns.' },
            { id: '#AMI-034', severity: 'low',    text: 'F12 (quicksave) conflicts with Workbench hotkey. Rebind via config.txt.' }
        ],
        compat: [
            { machine: 'A1200 * stock * 2MB',           boot: 'no',  menu: 'no',      gameplay: 'no',      save: 'no',      audio: 'no' },
            { machine: 'A1200 * 030/50 * 32MB * RTG',   boot: 'yes', menu: 'yes',     gameplay: 'yes',     save: 'yes',     audio: 'no' },
            { machine: 'A4000/040 * 32MB * CGX',        boot: 'yes', menu: 'yes',     gameplay: 'yes',     save: 'yes',     audio: 'no' },
            { machine: 'A4000/060 * 128MB * CGX 3',     boot: 'yes', menu: 'yes',     gameplay: 'yes',     save: 'yes',     audio: 'no' },
            { machine: 'Vampire V4 standalone * 512MB', boot: 'yes', menu: 'yes',     gameplay: 'yes',     save: 'yes',     audio: 'no' },
            { machine: 'FS-UAE * A1200 030 config',     boot: 'yes', menu: 'yes',     gameplay: 'yes',     save: 'yes',     audio: 'no' },
            { machine: 'WinUAE * A4000 060 config',     boot: 'yes', menu: 'yes',     gameplay: 'partial', save: 'partial', audio: 'no' }
        ],
        changelog: [
            { date: '2026-04-11', version: '1.7.0-ami3', notes: [
                'Fixed #AMI-011: aqueduct pathing stalls on maps > 200x200 (workaround still required at > 220).',
                'Mouse sprite no longer corrupts under Picasso96 2.4b.',
                'Save files now fully round-trip with PC Julius 1.7 -- bitwise identical.',
                'Boot-time cut from 4.2s to 2.8s on stock A1200 via lazy asset loading.'
            ] },
            { date: '2026-03-04', version: '1.7.0-ami2', notes: [
                'Rebuilt against libSDL2-amigaos3 0.7.0 -- texture path rewritten for CyberGraphX.',
                'Campaign mission 8 no longer crashes on Cleopatra event trigger.'
            ] },
            { date: '2026-01-22', version: '1.7.0-ami1', notes: [
                'Initial port. 360 source files, 1 upstream patch (platform/file.c: Amiga path canonicalisation).',
                '~42 FPS on A1200/030/50. 2MB binary. No audio.'
            ] },
            { date: '2025-12-10', version: '1.6.2-ami0', notes: [
                'Tech preview. Main menu loads. City does not yet render.'
            ] }
        ],
        patches: [
            {
                file: 'src/platform/file.c  (1 of 1)',
                lines: [
                    { type: 'hunk', n: '',    text: '@@ -142,7 +142,11 @@ static void canonicalize_path(char *path)' },
                    { type: 'ctx',  n: '142', text: '{' },
                    { type: 'ctx',  n: '143', text: '    char *p = path;' },
                    { type: 'ctx',  n: '144', text: '    while (*p) {' },
                    { type: 'del',  n: '145', text: "        if (*p == '\\\\') *p = '/';" },
                    { type: 'add',  n: '145', text: '#ifdef __amigaos__' },
                    { type: 'add',  n: '146', text: "        /* Amiga uses volume:path/file -- preserve the colon. */" },
                    { type: 'add',  n: '147', text: "        if (*p == '\\\\' && p != path && *(p-1) != ':') *p = '/';" },
                    { type: 'add',  n: '148', text: '#else' },
                    { type: 'add',  n: '149', text: "        if (*p == '\\\\') *p = '/';" },
                    { type: 'add',  n: '150', text: '#endif' },
                    { type: 'ctx',  n: '151', text: '        p++;' },
                    { type: 'ctx',  n: '152', text: '    }' },
                    { type: 'ctx',  n: '153', text: '}' }
                ]
            }
        ]
    };

    // ── Generic details generator (fallback for all non-Julius ports) ────────
    function makeDetails(port) {
        var isPlanned = port.status === 'planned';
        var isWip = port.status === 'wip';
        return {
            about: [
                port.blurb,
                'Cross-compiled from ' + port.originalPlatform + ' sources via bebbo-gcc 13.1 and libSDL2-amigaos3 0.7.0. Built against ' +
                    (port.files || '--') + ' source files with ' + (port.sourceMods || 0) +
                    ' upstream modifications -- see the Patches tab for the diff.',
                isPlanned
                    ? 'Not yet started. Listed on the PORT_CANDIDATES roadmap -- contribute via the GitHub repo or submit your interest below.'
                    : isWip
                        ? 'Actively in development -- current version is ' + port.version + '. Build notes and the latest ' + port.lastBuild + ' artefact are on the GitHub releases page.'
                        : 'Shipping. ' + port.size + ' compressed, installs to SYS:Games/' + port.name + '/ by default.'
            ],
            toolchain: 'bebbo-gcc 13.1.0 (m68k-amigaos)',
            sdl2: 'libSDL2-amigaos3 0.7.0',
            buildTime: isPlanned ? '--' : Math.max(1, Math.round(port.files / 70)) + 'm ' + String(port.files % 60).padStart(2, '0') + 's',
            dataFiles: isPlanned ? null : 'Original ' + port.game + ' game data required -- copy to SYS:Games/' + port.name + '/data/',
            requirements: [
                { k: 'CPU',      v: isPlanned ? 'TBD' : '68030 @ 50MHz' },
                { k: 'RAM',      v: isPlanned ? 'TBD' : '8MB Fast' },
                { k: 'OS',       v: 'AmigaOS 3.0+' },
                { k: 'Graphics', v: 'RTG (CGX/P96)' },
                { k: 'Disk',     v: port.size && port.size !== '--' ? port.size + ' engine + data' : 'TBD' },
                { k: 'TCP',      v: 'Not required' }
            ],
            screenshots: [
                { caption: 'title screen', ornamentTop: port.game.toUpperCase() + ' * ' + port.year },
                { caption: 'gameplay',     ornamentTop: port.genre.toUpperCase() },
                { caption: 'hud overlay' },
                { caption: 'options screen' }
            ],
            controls: [
                { action: 'Move',       keys: ['Arrows'] },
                { action: 'Action',     keys: ['Space'] },
                { action: 'Menu',       keys: ['Esc'] },
                { action: 'Pause',      keys: ['P'] },
                { action: 'Save',       keys: ['Ctrl', 'S'] },
                { action: 'Load',       keys: ['Ctrl', 'L'] },
                { action: 'Screenshot', keys: ['PrtSc'] },
                { action: 'Quit',       keys: ['Ctrl', 'Q'] }
            ],
            issues: isPlanned ? [] : [
                { id: '#AMI-101', severity: isWip ? 'high' : 'low',
                  text: isWip ? 'Crashes on second mission briefing -- null deref in intro decoder.'
                              : 'No MIDI music -- AHI routing pending.' },
                { id: '#AMI-102', severity: 'medium', text: 'Mouse cursor flickers on Picasso96 versions prior to 2.4b.' },
                { id: '#AMI-103', severity: 'low',    text: port.audio ? 'Occasional audio pop on first SFX trigger.' : 'No audio -- SDL2_mixer integration in progress.' }
            ],
            compat: [
                { machine: 'A1200 * stock * 2MB',           boot: port.status === 'playable' ? 'partial' : 'no', menu: 'no', gameplay: 'no', save: 'no', audio: 'no' },
                { machine: 'A1200 * 030/50 * 32MB * RTG',   boot: isPlanned ? 'no' : 'yes', menu: isPlanned ? 'no' : 'yes', gameplay: port.status === 'playable' ? 'yes' : (isWip ? 'partial' : 'no'), save: port.status === 'playable' ? 'yes' : 'partial', audio: port.audio ? 'yes' : 'no' },
                { machine: 'A4000/060 * 128MB',             boot: isPlanned ? 'no' : 'yes', menu: isPlanned ? 'no' : 'yes', gameplay: port.status === 'playable' ? 'yes' : (isWip ? 'partial' : 'no'), save: port.status === 'playable' ? 'yes' : 'partial', audio: port.audio ? 'yes' : 'no' },
                { machine: 'Vampire V4 * 512MB',            boot: isPlanned ? 'no' : 'yes', menu: isPlanned ? 'no' : 'yes', gameplay: isPlanned ? 'no' : 'yes', save: isPlanned ? 'no' : 'yes', audio: port.audio ? 'yes' : 'no' },
                { machine: 'FS-UAE * 030 config',           boot: isPlanned ? 'no' : 'yes', menu: isPlanned ? 'no' : 'yes', gameplay: port.status === 'playable' ? 'yes' : (isWip ? 'partial' : 'no'), save: port.status === 'playable' ? 'yes' : 'partial', audio: port.audio ? 'yes' : 'no' }
            ],
            changelog: isPlanned
                ? [{ date: '--', version: 'not started', notes: ['Tracked on the PORT_CANDIDATES roadmap. ' + port.blurb] }]
                : [
                    { date: port.lastBuild, version: port.version, notes: [
                        isWip ? 'Work in progress build -- see issues tab.' : 'Stable release -- shipping build.',
                        'Built against libSDL2-amigaos3 0.7.0 and bebbo-gcc 13.1.',
                        port.files + ' source files, ' + port.sourceMods + ' upstream patches.'
                    ]},
                    { date: '2025-12-01', version: 'initial', notes: [
                        'First build attempt -- upstream source compiles cleanly after path canonicalisation patch.'
                    ]}
                ],
            patches: (port.sourceMods || 0) === 0 ? [] : (function () {
                var count = Math.min(port.sourceMods, 2);
                var out = [];
                for (var i = 0; i < count; i++) {
                    out.push({
                        file: 'src/platform/amiga' + (i > 0 ? '_input' : '') + '.c  (' + (i + 1) + ' of ' + count + ')',
                        lines: [
                            { type: 'hunk', n: '',   text: '@@ -42,5 +42,9 @@' },
                            { type: 'ctx',  n: '42', text: 'void platform_init(void)' },
                            { type: 'ctx',  n: '43', text: '{' },
                            { type: 'del',  n: '44', text: "    set_path_sep('/');" },
                            { type: 'add',  n: '44', text: '#ifdef __amigaos__' },
                            { type: 'add',  n: '45', text: '    /* Amiga uses volume:path -- different canonicalisation. */' },
                            { type: 'add',  n: '46', text: '    amiga_path_init();' },
                            { type: 'add',  n: '47', text: '#else' },
                            { type: 'add',  n: '48', text: "    set_path_sep('/');" },
                            { type: 'add',  n: '49', text: '#endif' },
                            { type: 'ctx',  n: '50', text: '}' }
                        ]
                    });
                }
                return out;
            })()
        };
    }

    // ── DOM helpers ──────────────────────────────────────────────────────────
    function el(tag, attrs, children) {
        var n = document.createElement(tag);
        if (attrs) {
            Object.keys(attrs).forEach(function (k) {
                if (k === 'class') n.className = attrs[k];
                else if (k === 'html') n.innerHTML = attrs[k];
                else if (k === 'text') n.textContent = attrs[k];
                else if (k.slice(0, 2) === 'on' && typeof attrs[k] === 'function') {
                    n.addEventListener(k.slice(2).toLowerCase(), attrs[k]);
                } else if (k === 'style' && typeof attrs[k] === 'object') {
                    Object.keys(attrs[k]).forEach(function (sk) { n.style[sk] = attrs[k][sk]; });
                } else {
                    n.setAttribute(k, attrs[k]);
                }
            });
        }
        (children || []).forEach(function (c) {
            if (c == null || c === false) return;
            if (typeof c === 'string') n.appendChild(document.createTextNode(c));
            else n.appendChild(c);
        });
        return n;
    }
    function statusLabel(s) {
        return s === 'playable' ? 'Playable' : s === 'wip' ? 'WIP' : 'Planned';
    }

    function screenshotPlaceholder(port, shot, idx) {
        var wrap = el('div', {
            class: 'screenshot-placeholder screenshot-scanline',
            style: '--ph-bg: ' + port.palette.bg + '; --ph-fg: ' + port.palette.fg
        });
        if (shot && shot.ornamentTop) {
            wrap.appendChild(el('div', { class: 'screenshot-placeholder__ornament', style: 'top: 6px; left: 8px;', text: shot.ornamentTop }));
        }
        var bottomText = (shot && shot.caption) ? shot.caption : ('#' + (idx + 1));
        wrap.appendChild(el('div', { class: 'screenshot-placeholder__ornament', style: 'bottom: 6px; right: 8px;', text: bottomText }));
        wrap.appendChild(el('div', { class: 'screenshot-placeholder__title', text: port.game }));
        return wrap;
    }

    // ── Renderers ────────────────────────────────────────────────────────────
    function renderHero(port, details, root) {
        var state = { shot: 0 };

        var hero = el('section', { class: 'detail-hero' });
        var left = el('div');
        var big = el('div', { class: 'detail-hero__screenshot screenshot-scanline' });
        big.appendChild(screenshotPlaceholder(port, details.screenshots[0], 0));
        left.appendChild(big);

        var strip = el('div', { class: 'shot-strip' });
        details.screenshots.forEach(function (s, i) {
            var item = el('div', {
                class: 'shot-strip__item' + (i === 0 ? ' active' : ''),
                onClick: function () {
                    state.shot = i;
                    big.innerHTML = '';
                    big.appendChild(screenshotPlaceholder(port, details.screenshots[i], i));
                    strip.querySelectorAll('.shot-strip__item').forEach(function (x, j) {
                        x.classList.toggle('active', j === i);
                    });
                }
            });
            item.appendChild(screenshotPlaceholder(port, s, i));
            strip.appendChild(item);
        });
        left.appendChild(strip);
        hero.appendChild(left);

        // Info column
        var info = el('div', { class: 'detail-hero__info' });
        var badges = el('div', { style: 'display:flex; gap: 6px;' });
        badges.appendChild(el('span', { class: 'status-pill status-pill--' + port.status, text: statusLabel(port.status) }));
        if (port.audio) {
            badges.appendChild(el('span', { class: 'badge badge--new', style: 'border-color:#444;', text: 'AUDIO' }));
        }
        info.appendChild(badges);

        info.appendChild(el('h1', { class: 'detail-hero__title', text: port.name }));
        info.appendChild(el('p', {
            class: 'detail-hero__subtitle',
            text: port.game + ' * ' + port.originalPlatform + ' * ' + port.year + ' * ' + port.genre
        }));
        info.appendChild(el('p', { class: 'detail-hero__blurb', text: port.blurb }));

        var stats = el('div', { class: 'detail-hero__stats' });
        [
            { label: 'Current FPS', value: (port.fps != null ? port.fps : '--'), extra: port.targetFps ? '/' + port.targetFps : '' },
            { label: 'Version',     value: port.version },
            { label: 'Binary size', value: port.size },
            { label: 'Last build',  value: port.lastBuild || '--' }
        ].forEach(function (s) {
            var box = el('div', { class: 'detail-hero__stat' });
            box.appendChild(el('div', { class: 'detail-hero__stat-label', text: s.label }));
            var val = el('div', { class: 'detail-hero__stat-value', text: String(s.value) });
            if (s.extra) {
                val.appendChild(el('span', { style: 'color:#7a6d50; font-size:10px; margin-left:4px;', text: s.extra }));
            }
            box.appendChild(val);
            stats.appendChild(box);
        });
        info.appendChild(stats);

        var actions = el('div', { class: 'detail-hero__actions' });
        actions.appendChild(el('a', { class: 'btn btn--primary', href: '#install', text: 'Install' }));
        if (port.upstream) {
            actions.appendChild(el('a', {
                class: 'btn btn--default', href: port.upstream, target: '_blank', rel: 'noopener',
                text: 'Upstream \u2197'
            }));
        }
        info.appendChild(actions);
        hero.appendChild(info);

        root.appendChild(hero);
    }

    function renderOverview(port, details, container) {
        container.innerHTML = '';
        var about = el('div');
        about.appendChild(el('h3', {
            style: 'margin:0; font-size:13px; text-transform:uppercase; letter-spacing:1.5px; color:var(--brown);',
            text: 'About this port'
        }));
        var aboutBody = el('div', { style: 'margin-top:8px; font-size:14px; line-height:1.6;' });
        details.about.forEach(function (p) {
            aboutBody.appendChild(el('p', { style: 'margin:0 0 8px;', text: p }));
        });
        about.appendChild(aboutBody);
        container.appendChild(about);

        var build = el('div', { style: 'margin-top:16px;' });
        build.appendChild(el('h3', {
            style: 'margin:0; font-size:13px; text-transform:uppercase; letter-spacing:1.5px; color:var(--brown);',
            text: 'Build details'
        }));
        var kv = el('div', { class: 'kv-list', style: 'margin-top:8px;' });
        var buildRows = [
            ['Source files', (port.files != null ? port.files.toLocaleString() : '--')],
            ['Source mods',  (port.sourceMods != null ? port.sourceMods + ' upstream patches' : '--')],
            ['Toolchain',    details.toolchain],
            ['SDL2 version', details.sdl2],
            ['Build time',   details.buildTime],
            ['Upstream',     port.upstream || '--']
        ];
        buildRows.forEach(function (r) {
            var row = el('div', { class: 'kv-list__row' });
            row.appendChild(el('span', { class: 'kv-list__key', text: r[0] }));
            var val = el('span', { class: 'kv-list__val' });
            if (r[0] === 'Upstream' && port.upstream) {
                val.appendChild(el('a', {
                    href: port.upstream, target: '_blank', rel: 'noopener',
                    text: port.upstream.replace(/^https?:\/\//, '')
                }));
            } else {
                val.textContent = String(r[1]);
            }
            row.appendChild(val);
            kv.appendChild(row);
        });
        build.appendChild(kv);
        container.appendChild(build);
    }

    function renderControls(details, container) {
        container.innerHTML = '';
        container.appendChild(el('h3', {
            style: 'margin:0 0 10px; font-size:13px; text-transform:uppercase; letter-spacing:1.5px; color:var(--brown);',
            text: 'Keyboard controls'
        }));
        var km = el('div', { class: 'keymap' });
        details.controls.forEach(function (c) {
            var row = el('div', { class: 'keymap__row' });
            row.appendChild(el('span', { style: 'font-size:13px;', text: c.action }));
            var keys = el('span', { class: 'keymap__keys' });
            c.keys.forEach(function (k) { keys.appendChild(el('span', { class: 'key', text: k })); });
            row.appendChild(keys);
            km.appendChild(row);
        });
        container.appendChild(km);
    }

    function renderIssues(details, container) {
        container.innerHTML = '';
        if (!details.issues.length) {
            container.appendChild(el('div', { class: 'alert alert--info', text: 'No known issues. Port has not started yet -- status: planned.' }));
            return;
        }
        details.issues.forEach(function (i) {
            var row = el('div', { class: 'issue' });
            row.appendChild(el('span', { class: 'issue__id', text: i.id }));
            row.appendChild(el('span', { text: i.text }));
            row.appendChild(el('span', { class: 'issue__severity issue__severity--' + i.severity, text: i.severity }));
            container.appendChild(row);
        });
    }

    function renderCompat(details, container) {
        container.innerHTML = '';
        var table = el('table', { class: 'compat' });
        var thead = el('thead');
        var headRow = el('tr');
        ['Machine', 'Boot', 'Menu', 'Gameplay', 'Save/Load', 'Audio'].forEach(function (h) {
            headRow.appendChild(el('th', { text: h }));
        });
        thead.appendChild(headRow);
        table.appendChild(thead);
        var tbody = el('tbody');
        details.compat.forEach(function (r) {
            var tr = el('tr');
            tr.appendChild(el('td', { text: r.machine }));
            ['boot', 'menu', 'gameplay', 'save', 'audio'].forEach(function (k) {
                var v = r[k];
                var cls = v === 'yes' ? 'yes' : v === 'no' ? 'no' : 'partial';
                var mark = v === 'yes' ? '\u2713' : v === 'no' ? '\u2717' : '~';
                tr.appendChild(el('td', {}, [el('span', { class: 'compat__mark compat__mark--' + cls, text: mark })]));
            });
            tbody.appendChild(tr);
        });
        table.appendChild(tbody);
        container.appendChild(table);
    }

    function renderChangelog(details, container) {
        container.innerHTML = '';
        var cl = el('div', { class: 'changelog' });
        details.changelog.forEach(function (e) {
            var entry = el('div', { class: 'changelog__entry' });
            var date = el('div', { class: 'changelog__date' });
            date.appendChild(document.createTextNode(e.date));
            date.appendChild(el('div', { class: 'changelog__version', text: e.version }));
            entry.appendChild(date);
            var what = el('div', { class: 'changelog__what' });
            var ul = el('ul');
            e.notes.forEach(function (n) { ul.appendChild(el('li', { text: n })); });
            what.appendChild(ul);
            entry.appendChild(what);
            cl.appendChild(entry);
        });
        container.appendChild(cl);
    }

    function renderPatches(details, container) {
        container.innerHTML = '';
        if (!details.patches.length) {
            container.appendChild(el('div', { class: 'alert alert--info', text: 'No source modifications -- this port builds on vanilla upstream.' }));
            return;
        }
        details.patches.forEach(function (p) {
            var viewer = el('div', { class: 'patch-viewer' });
            var header = el('div', { class: 'patch-viewer__header' });
            header.appendChild(el('span', { text: p.file }));
            var adds = p.lines.filter(function (l) { return l.type === 'add'; }).length;
            var dels = p.lines.filter(function (l) { return l.type === 'del'; }).length;
            header.appendChild(el('span', { text: adds + ' additions * ' + dels + ' deletions' }));
            viewer.appendChild(header);
            var body = el('div', { class: 'patch-viewer__body' });
            p.lines.forEach(function (l) {
                var line = el('div', { class: 'patch-viewer__line patch-viewer__line--' + l.type });
                line.appendChild(el('span', { class: 'ln', text: l.n || '' }));
                var prefix = l.type === 'add' ? '+ ' : l.type === 'del' ? '- ' : '  ';
                line.appendChild(el('span', { text: prefix + l.text }));
                body.appendChild(line);
            });
            viewer.appendChild(body);
            container.appendChild(viewer);
            viewer.style.marginBottom = '12px';
        });
    }

    function renderTabs(port, details, root) {
        var tabs = [
            { id: 'overview',  label: 'Overview' },
            { id: 'controls',  label: 'Controls' },
            { id: 'issues',    label: 'Issues (' + details.issues.length + ')' },
            { id: 'compat',    label: 'Compat' },
            { id: 'changelog', label: 'Changelog' },
            { id: 'patches',   label: 'Patches (' + details.patches.length + ')' }
        ];
        var bar = el('div', { class: 'tabs' });
        var panel = el('div', { class: 'tab-panel' });
        var active = 'overview';

        function show(id) {
            active = id;
            bar.querySelectorAll('.tab').forEach(function (t) {
                t.classList.toggle('tab--active', t.getAttribute('data-id') === id);
            });
            if (id === 'overview')       renderOverview(port, details, panel);
            else if (id === 'controls')  renderControls(details, panel);
            else if (id === 'issues')    renderIssues(details, panel);
            else if (id === 'compat')    renderCompat(details, panel);
            else if (id === 'changelog') renderChangelog(details, panel);
            else if (id === 'patches')   renderPatches(details, panel);
        }

        tabs.forEach(function (t) {
            var btn = el('button', {
                class: 'tab' + (t.id === active ? ' tab--active' : ''),
                type: 'button',
                'data-id': t.id,
                text: t.label,
                onClick: function () { show(t.id); }
            });
            bar.appendChild(btn);
        });
        root.appendChild(bar);
        root.appendChild(panel);
        show('overview');
    }

    function renderSidebar(port, details, root) {
        // Install
        var install = el('section', { class: 'group-frame', id: 'install' });
        install.appendChild(el('div', { class: 'group-frame__title', text: 'Install' }));
        var installBody = el('div', { class: 'group-frame__body' });
        var stack = el('div', { style: 'display:flex; flex-direction:column; gap: 8px;' });

        var ib1 = el('div', { class: 'install-block' });
        ib1.appendChild(el('span', { class: 'install-block__prompt', text: '1.SYS:>' }));
        ib1.appendChild(el('span', { class: 'install-block__cmd', text: 'amiport install ' + port.id }));
        stack.appendChild(ib1);

        stack.appendChild(el('p', {
            style: 'margin:0; font-size:11px; color:var(--text-muted); font-family:"SF Mono", Consolas, monospace;',
            text: 'or download .lha manually:'
        }));
        var ib2 = el('div', { class: 'install-block' });
        ib2.appendChild(el('span', { class: 'install-block__prompt', text: '$' }));
        ib2.appendChild(el('span', {
            class: 'install-block__cmd',
            text: 'curl -O amiport.platesteel.net/packages/' + port.id + '-' + port.version + '.lha'
        }));
        stack.appendChild(ib2);

        if (details.dataFiles) {
            var warn = el('div', { class: 'alert alert--warning' });
            warn.appendChild(el('strong', { text: 'Game data required. ' }));
            warn.appendChild(document.createTextNode(details.dataFiles));
            stack.appendChild(warn);
        }
        installBody.appendChild(stack);
        install.appendChild(installBody);
        root.appendChild(install);

        // Requirements
        var reqs = el('section', { class: 'group-frame' });
        reqs.appendChild(el('div', { class: 'group-frame__title', text: 'Requirements' }));
        var reqsBody = el('div', { class: 'group-frame__body' });
        var kv = el('div', { class: 'kv-list' });
        details.requirements.forEach(function (r) {
            var row = el('div', { class: 'kv-list__row' });
            row.appendChild(el('span', { class: 'kv-list__key', text: r.k }));
            row.appendChild(el('span', { class: 'kv-list__val', text: r.v }));
            kv.appendChild(row);
        });
        reqsBody.appendChild(kv);
        reqs.appendChild(reqsBody);
        root.appendChild(reqs);

        // See also
        var others = PORTS.filter(function (p) { return p.id !== port.id && p.status === 'playable'; }).slice(0, 3);
        if (others.length) {
            var see = el('section', { class: 'group-frame' });
            see.appendChild(el('div', { class: 'group-frame__title', text: 'See also' }));
            var seeBody = el('div', { class: 'group-frame__body' });
            var list = el('div');
            others.forEach(function (o) {
                var link = el('a', { class: 'see-also__link', href: o.id + '.html' });
                var left = el('span');
                left.appendChild(el('strong', { text: o.name }));
                left.appendChild(document.createTextNode(' * ' + o.game));
                link.appendChild(left);
                if (o.fps != null) {
                    link.appendChild(el('span', { class: 'see-also__fps', text: o.fps + ' fps' }));
                }
                list.appendChild(link);
            });
            seeBody.appendChild(list);
            see.appendChild(seeBody);
            root.appendChild(see);
        }
    }

    // ── Boot ─────────────────────────────────────────────────────────────────
    function init() {
        var gameId = document.documentElement.getAttribute('data-game-id');
        var port = PORTS_BY_ID[gameId];
        var mount = document.getElementById('detail-root');
        if (!port || !mount) {
            if (mount) mount.textContent = 'Unknown port: ' + gameId;
            return;
        }

        document.title = port.name + ' -- amiport gaming';

        var details = gameId === 'julius' ? JULIUS_DETAILS : makeDetails(port);

        // Breadcrumb
        var crumb = el('nav', { class: 'breadcrumb', 'aria-label': 'Breadcrumb' });
        crumb.appendChild(el('a', { href: '../gaming.html', text: 'amiport/gaming' }));
        crumb.appendChild(el('span', { class: 'breadcrumb__sep', text: '/' }));
        crumb.appendChild(el('a', { href: '../gaming.html#ports', text: 'ports' }));
        crumb.appendChild(el('span', { class: 'breadcrumb__sep', text: '/' }));
        crumb.appendChild(el('span', { text: port.id }));
        mount.appendChild(crumb);

        renderHero(port, details, mount);

        // Main grid: tabs + sidebar
        var grid = el('div', { class: 'detail-grid' });
        var main = el('div');
        renderTabs(port, details, main);
        grid.appendChild(main);

        var side = el('div', { class: 'col-side' });
        renderSidebar(port, details, side);
        grid.appendChild(side);

        mount.appendChild(grid);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
