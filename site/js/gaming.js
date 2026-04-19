/* ============================================================================
   gaming.js — amiport gaming portal (vanilla JS, no frameworks)
   Port data + tile rendering + filter/sort + live FPS ticker + submit form.
   ============================================================================ */
(function () {
    'use strict';

    // ── Port data ────────────────────────────────────────────────────────────
    var PORTS = [
        {
            id: 'julius', name: 'Julius', game: 'Caesar III', year: 1998,
            originalPlatform: 'Windows', genre: 'City-builder',
            status: 'playable', fps: 42, targetFps: 60, audio: false,
            version: '1.7.0-ami3', size: '3.2 MB', featured: true,
            files: 360, sourceMods: 1, lastBuild: '2026-04-11',
            upstream: 'https://github.com/bvschaik/julius',
            palette: { bg: '#3a2f1c', fg: '#c8965a' },
            blurb: "The 1998 classic city-builder -- set your citizens' wages, watch Mars smite heretics, delete your own aqueducts by accident. Never ran on a classic Amiga. Now it does."
        },
        {
            id: 'chocolate-doom', name: 'Chocolate Doom', game: 'Doom', year: 1993,
            originalPlatform: 'DOS', genre: 'FPS',
            status: 'playable', fps: 38, targetFps: 35, audio: true,
            version: '3.0.1-ami2', size: '2.1 MB', featured: true,
            files: 214, sourceMods: 3, lastBuild: '2026-03-28',
            upstream: 'https://github.com/bdgscotland/chocolate-doom',
            palette: { bg: '#2a0f0f', fg: '#cc4444' },
            blurb: 'Historically-accurate Doom port. WAV sound effects through SDL2_mixer. E1M1 loads in 2.8 seconds on a stock A1200.'
        },
        {
            id: 'ccleste', name: 'Celeste Classic', game: 'Celeste Classic', year: 2018,
            originalPlatform: 'PICO-8', genre: 'Platformer',
            status: 'playable', fps: 60, targetFps: 60, audio: false,
            version: '1.0.1-ami1', size: '0.4 MB', featured: false,
            files: 12, sourceMods: 0, lastBuild: '2026-02-14',
            upstream: 'https://github.com/lemon32767/ccleste',
            palette: { bg: '#1a1a3a', fg: '#88aacc' },
            blurb: 'First new-era game ever running on a classic 68k Amiga. PICO-8 to 68030 is a 2018 to 1990 time-warp.'
        },
        {
            id: '1oom', name: '1oom', game: 'Master of Orion', year: 1993,
            originalPlatform: 'DOS', genre: '4X Strategy',
            status: 'playable', fps: 43, targetFps: 60, audio: false,
            version: '1.0.2-ami1', size: '1.8 MB', featured: false,
            files: 187, sourceMods: 0, lastBuild: '2026-03-05',
            upstream: 'https://gitlab.com/KrzysztofBS/1oom',
            palette: { bg: '#0f0f2a', fg: '#8866cc' },
            blurb: 'The original 4X. Galaxy generation in 480ms, ~43 FPS on a Vampire V4. Zero source modifications -- pure stdlib port.'
        },
        {
            id: 'sdlpop', name: 'SDLPoP', game: 'Prince of Persia', year: 1989,
            originalPlatform: 'DOS', genre: 'Platformer',
            status: 'wip', fps: 35, targetFps: 60, audio: false,
            version: '1.24-ami0', size: '1.1 MB', featured: false,
            files: 98, sourceMods: 2, lastBuild: '2026-04-02',
            upstream: 'https://github.com/NagyD/SDLPoP',
            palette: { bg: '#2a1a0a', fg: '#ddaa66' },
            blurb: "Renders. Doesn't play. Keyboard input routes through OS4-style event pipe that 3.x doesn't have. Fix planned."
        },
        {
            id: 'vanilla-conquer', name: 'Vanilla Conquer', game: 'Command & Conquer', year: 1995,
            originalPlatform: 'Windows', genre: 'RTS',
            status: 'wip', fps: 22, targetFps: 30, audio: false,
            version: '0.3-ami0', size: '8.4 MB', featured: false,
            files: 1804, sourceMods: 11, lastBuild: '2026-04-09',
            upstream: 'https://github.com/TheAssemblyArmada/Vanilla-Conquer',
            palette: { bg: '#1a2a0a', fg: '#88aa44' },
            blurb: '430,000 lines of C++11 cross-compiled with bebbo-gcc. Main menu loads. GDI Mission 1 loads. Second mission crashes on briefing. You can watch the intro though.'
        },
        {
            id: 'fheroes2', name: 'fheroes2', game: 'Heroes of Might and Magic II', year: 1996,
            originalPlatform: 'DOS/Windows', genre: 'Turn-based Strategy',
            status: 'wip', fps: 30, targetFps: 60, audio: false,
            version: '1.1.3-ami0', size: '4.5 MB', featured: false,
            files: 892, sourceMods: 5, lastBuild: '2026-03-22',
            upstream: 'https://github.com/ihhub/fheroes2',
            palette: { bg: '#1a2a1a', fg: '#88cc88' },
            blurb: 'The greatest fantasy-TBS ever made, re-implemented from scratch in modern C++. Asset loading works. Combat UI renders. Turns crash after move 7.'
        },
        {
            id: 'reminiscence', name: 'REminiscence', game: 'Flashback', year: 1992,
            originalPlatform: 'Amiga/DOS', genre: 'Cinematic Platformer',
            status: 'planned', fps: null, targetFps: 60, audio: false,
            version: '--', size: '--', featured: false,
            palette: { bg: '#2a0a1a', fg: '#cc6688' },
            blurb: 'The coming-home port. Originally ran on the A500.'
        },
        {
            id: 'another-world', name: 'Another World', game: 'Another World', year: 1991,
            originalPlatform: 'Amiga', genre: 'Cinematic Platformer',
            status: 'planned', fps: null, targetFps: 60, audio: false,
            version: '--', size: '--', featured: false,
            palette: { bg: '#0a0a2a', fg: '#6688cc' },
            blurb: 'Another coming-home port. Original ran on the A500 in 550KB.'
        },
        {
            id: 'opentyrian', name: 'OpenTyrian', game: 'Tyrian', year: 1995,
            originalPlatform: 'DOS', genre: "Shoot 'em up",
            status: 'planned', fps: null, targetFps: 60, audio: false,
            version: '--', size: '--', featured: false,
            palette: { bg: '#2a1a2a', fg: '#cc88cc' },
            blurb: 'Vertical scroller with a fully reverse-engineered engine. Portable, asset-loading, should be straightforward.'
        }
    ];

    var STATS = {
        total: PORTS.length,
        playable: PORTS.filter(function (p) { return p.status === 'playable'; }).length,
        wip: PORTS.filter(function (p) { return p.status === 'wip'; }).length,
        planned: PORTS.filter(function (p) { return p.status === 'planned'; }).length,
        totalFiles: PORTS.reduce(function (a, p) { return a + (p.files || 0); }, 0),
        totalSourceMods: PORTS.reduce(function (a, p) { return a + (p.sourceMods || 0); }, 0),
        avgFps: Math.round(
            PORTS.filter(function (p) { return p.fps; }).reduce(function (a, p) { return a + p.fps; }, 0) /
            PORTS.filter(function (p) { return p.fps; }).length
        )
    };

    // ── Helpers ──────────────────────────────────────────────────────────────
    function el(tag, attrs, children) {
        var n = document.createElement(tag);
        if (attrs) {
            Object.keys(attrs).forEach(function (k) {
                if (k === 'class') n.className = attrs[k];
                else if (k === 'html') n.innerHTML = attrs[k];
                else if (k === 'text') n.textContent = attrs[k];
                else if (k === 'style' && typeof attrs[k] === 'object') {
                    Object.keys(attrs[k]).forEach(function (sk) { n.style[sk] = attrs[k][sk]; });
                } else if (k.slice(0, 2) === 'on' && typeof attrs[k] === 'function') {
                    n.addEventListener(k.slice(2).toLowerCase(), attrs[k]);
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
        return s === 'playable' ? 'Playable'
             : s === 'wip'      ? 'WIP'
             :                    'Planned';
    }

    // ── Placeholder screenshot ───────────────────────────────────────────────
    function screenshotPlaceholder(port) {
        var wrap = el('div', {
            class: 'screenshot-placeholder screenshot-scanline',
            style: '--ph-bg: ' + port.palette.bg + '; --ph-fg: ' + port.palette.fg
        });
        wrap.appendChild(el('div', { class: 'screenshot-placeholder__ornament', style: 'top: 6px; left: 8px;', text: port.year + ' -> 2026' }));
        wrap.appendChild(el('div', { class: 'screenshot-placeholder__ornament', style: 'bottom: 6px; right: 8px;', text: port.originalPlatform.toUpperCase() }));
        wrap.appendChild(el('div', { class: 'screenshot-placeholder__title', text: port.game }));
        return wrap;
    }

    // ── Port tile ────────────────────────────────────────────────────────────
    function portTile(port, opts) {
        opts = opts || {};
        var liveFps = opts.liveFps;
        var featured = !!opts.featured;

        var tile = el('a', {
            class: 'port-tile' + (featured ? ' port-tile--featured' : ''),
            href: 'games/' + port.id + '.html',
            'aria-label': port.name + ' -- ' + statusLabel(port.status)
        });

        var shot = el('div', { class: 'port-tile__screenshot' });
        shot.appendChild(screenshotPlaceholder(port));

        var badges = el('div', { class: 'port-tile__badges' });
        badges.appendChild(el('span', {
            class: 'status-pill status-pill--' + port.status,
            text: statusLabel(port.status)
        }));
        if (port.audio) {
            badges.appendChild(el('span', { class: 'status-pill status-pill--wip', text: 'Audio' }));
        }
        shot.appendChild(badges);

        if (port.fps != null && liveFps && liveFps[port.id] != null) {
            shot.appendChild(el('div', {
                class: 'port-tile__screenshot-fps',
                text: liveFps[port.id] + ' FPS'
            }));
        }
        tile.appendChild(shot);

        var body = el('div', { class: 'port-tile__body' });
        body.appendChild(el('h3', { class: 'port-tile__title', text: port.name }));
        body.appendChild(el('p', {
            class: 'port-tile__subtitle',
            text: port.game + ' (' + port.year + ') -- ' + port.genre
        }));

        if (featured && port.blurb) {
            body.appendChild(el('p', {
                style: 'font-size: 13px; color: var(--text); margin: 8px 0 0; line-height: 1.45;',
                text: port.blurb
            }));
        }

        var meta = el('div', { class: 'port-tile__meta' });
        meta.appendChild(el('span', { html: 'v<strong>' + port.version + '</strong>' }));
        if (port.size && port.size !== '--') {
            meta.appendChild(el('span', { html: '<strong>' + port.size + '</strong>' }));
        }
        if (port.files) {
            meta.appendChild(el('span', { html: '<strong>' + port.files.toLocaleString() + '</strong> files' }));
        }
        if (port.sourceMods != null) {
            meta.appendChild(el('span', { html: '<strong>' + port.sourceMods + '</strong> mods' }));
        }
        body.appendChild(meta);

        tile.appendChild(body);
        return tile;
    }

    // ── Filter / sort ────────────────────────────────────────────────────────
    function applyFilterSort(list, filter, sort) {
        var out = list.slice();
        if (filter.status !== 'all') {
            out = out.filter(function (p) { return p.status === filter.status; });
        }
        if (filter.audio !== 'all') {
            out = out.filter(function (p) { return (filter.audio === 'yes') === !!p.audio; });
        }
        if (filter.platform !== 'all') {
            out = out.filter(function (p) { return p.originalPlatform.indexOf(filter.platform) !== -1; });
        }
        if (sort === 'featured') {
            var order = { playable: 0, wip: 1, planned: 2 };
            out.sort(function (a, b) {
                if (a.featured !== b.featured) return a.featured ? -1 : 1;
                if (order[a.status] !== order[b.status]) return order[a.status] - order[b.status];
                return (b.fps || 0) - (a.fps || 0);
            });
        } else if (sort === 'fps') {
            out.sort(function (a, b) { return (b.fps || 0) - (a.fps || 0); });
        } else if (sort === 'year') {
            out.sort(function (a, b) { return a.year - b.year; });
        } else if (sort === 'name') {
            out.sort(function (a, b) { return a.name.localeCompare(b.name); });
        }
        return out;
    }

    // ── Live FPS ticker ──────────────────────────────────────────────────────
    var liveFpsState = {};
    var liveFpsTimer = null;
    function initLiveFps() {
        PORTS.forEach(function (p) { if (p.fps != null) liveFpsState[p.id] = p.fps; });
    }
    function startLiveFps(onTick) {
        stopLiveFps();
        liveFpsTimer = setInterval(function () {
            Object.keys(liveFpsState).forEach(function (id) {
                var base = PORTS.find(function (p) { return p.id === id; }).fps;
                var jitter = Math.round((Math.random() - 0.5) * 4);
                liveFpsState[id] = Math.max(1, base + jitter);
            });
            onTick(liveFpsState);
        }, 1400);
    }
    function stopLiveFps() {
        if (liveFpsTimer) { clearInterval(liveFpsTimer); liveFpsTimer = null; }
    }

    // ── Leaderboard rendering ────────────────────────────────────────────────
    function renderLeaderboard(tbody, liveFps) {
        tbody.innerHTML = '';
        var rows = PORTS.filter(function (p) { return p.fps != null; })
            .slice()
            .sort(function (a, b) {
                return (liveFps[b.id] || 0) - (liveFps[a.id] || 0);
            });
        rows.forEach(function (r) {
            var tr = el('tr');
            tr.appendChild(el('td', {}, [el('a', { class: 'name', href: 'games/' + r.id + '.html', text: r.name })]));
            tr.appendChild(el('td', {}, [el('span', { style: 'font-size: 12px;', text: r.game })]));

            var v = liveFps[r.id];
            var fpsCell = el('td');
            fpsCell.appendChild(el('span', {
                class: 'ver',
                style: 'color: ' + (v >= 30 ? 'var(--success-border)' : 'var(--warning-border)') +
                    '; font-variant-numeric: tabular-nums; font-weight: 700;',
                text: v != null ? String(v) : '--'
            }));
            tr.appendChild(fpsCell);
            tr.appendChild(el('td', {}, [el('span', { class: 'ver', text: String(r.targetFps) })]));

            var pct = Math.min(100, Math.round((liveFps[r.id] || 0) / r.targetFps * 100));
            var progCell = el('td');
            var bar = el('div', { style: 'display:flex; align-items:center; gap:6px;' });
            var track = el('div', {
                style: 'width:100px; height:8px; background: var(--bg); border: 1px solid var(--bevel-shadow);'
            });
            track.appendChild(el('div', {
                style: 'width:' + pct + '%; height:100%; background: ' +
                    (pct >= 100 ? 'var(--success-border)' : 'var(--amber)') + ';'
            }));
            bar.appendChild(track);
            bar.appendChild(el('span', {
                class: 'ver',
                style: 'font-size:10px; width:34px; text-align:right;',
                text: pct + '%'
            }));
            progCell.appendChild(bar);
            tr.appendChild(progCell);

            tr.appendChild(el('td', {}, [
                el('span', { class: 'status-pill status-pill--' + r.status, text: statusLabel(r.status) })
            ]));
            tbody.appendChild(tr);
        });
    }

    // ── Hero leaderboard (the big ticker) ────────────────────────────────────
    function renderHeroLeaderboard(container, liveFps) {
        var active = PORTS.filter(function (p) { return p.fps != null; });
        var best = active.reduce(function (acc, p) {
            var v = liveFps[p.id];
            return v > (acc.v || 0) ? { v: v, name: p.name } : acc;
        }, { v: 0, name: '--' });
        var avg = active.length ? Math.round(
            active.reduce(function (a, p) { return a + (liveFps[p.id] || 0); }, 0) / active.length
        ) : 0;

        var items = [
            { label: 'Top port', value: best.name, unit: '' },
            { label: 'Top FPS', value: best.v || '--', unit: 'fps' },
            { label: 'Avg FPS', value: avg, unit: 'fps' },
            { label: 'Playable', value: STATS.playable + '/' + STATS.total, unit: '' }
        ];

        container.innerHTML = '';
        items.forEach(function (it) {
            var item = el('div', { class: 'leaderboard__item' });
            item.appendChild(el('div', { class: 'leaderboard__label', text: it.label }));
            var val = el('div', { class: 'leaderboard__value ticking' }, [document.createTextNode(String(it.value))]);
            if (it.unit) val.appendChild(el('sub', { text: it.unit }));
            item.appendChild(val);
            container.appendChild(item);
            setTimeout(function () { val.classList.remove('ticking'); }, 180);
        });
    }

    // ── Grid render ──────────────────────────────────────────────────────────
    function renderGrid(container, ports, layout, liveFps) {
        container.innerHTML = '';
        container.className = 'port-grid' + (layout === 'featured' ? ' port-grid--featured' : '');
        if (!ports.length) {
            container.appendChild(el('div', {
                class: 'alert alert--warning',
                html: '<strong>No ports match those filters.</strong> Try loosening the status or platform filter.'
            }));
            return;
        }
        ports.forEach(function (p, i) {
            var featured = layout === 'featured' && i === 0;
            container.appendChild(portTile(p, { featured: featured, liveFps: liveFps }));
        });
    }

    // ── Submit form ──────────────────────────────────────────────────────────
    function bindSubmitForm() {
        var form = document.getElementById('submit-form');
        if (!form) return;
        var successBox = document.getElementById('submit-success');
        var errorBox = document.getElementById('submit-error');

        form.addEventListener('submit', function (e) {
            e.preventDefault();
            successBox.classList.add('hidden');
            errorBox.classList.add('hidden');

            var name = form.game_name.value.trim();
            var upstream = form.upstream.value.trim();
            var why = form.why.value.trim();
            var ok = true;

            [['game_name', name.length >= 2], ['upstream', upstream.length === 0 || /^https?:\/\//i.test(upstream)]]
                .forEach(function (entry) {
                    var input = form[entry[0]];
                    var valid = entry[1];
                    input.setAttribute('aria-invalid', valid ? 'false' : 'true');
                    if (!valid) ok = false;
                });

            if (!ok) {
                errorBox.textContent = 'Please enter a game name (2+ chars) and a valid http(s) URL if provided.';
                errorBox.classList.remove('hidden');
                return;
            }

            successBox.innerHTML = '<strong>Request received.</strong> Thanks -- ' +
                name.replace(/[<>]/g, '') + ' added to the triage queue. (Demo only; not wired to a backend.)';
            successBox.classList.remove('hidden');
            form.reset();
        });
    }

    // ── Tweaks panel ─────────────────────────────────────────────────────────
    var tweaks = {
        accent: 'amber',
        layout: 'featured',
        arcade: false,
        density: 'comfortable',
        leaderboard: true,
        liveFps: true
    };

    function applyAccent(accent) {
        var root = document.documentElement;
        root.removeAttribute('data-accent');
        if (accent && accent !== 'amber') root.setAttribute('data-accent', accent);
    }

    function applyTweaks() {
        applyAccent(tweaks.accent);
        document.body.classList.toggle('arcade', !!tweaks.arcade);
        document.body.classList.toggle('dense', tweaks.density === 'dense');
        var lb = document.getElementById('hero-leaderboard-wrap');
        if (lb) lb.style.display = tweaks.leaderboard ? '' : 'none';
    }

    function bindTweaks(onTweakChange) {
        var toggle = document.getElementById('tweaks-toggle');
        var panel = document.getElementById('tweaks-panel');
        if (toggle && panel) {
            toggle.addEventListener('click', function () { panel.classList.toggle('open'); });
        }

        var swatches = document.querySelectorAll('.swatch');
        swatches.forEach(function (s) {
            s.addEventListener('click', function () {
                swatches.forEach(function (x) { x.classList.remove('active'); });
                s.classList.add('active');
                tweaks.accent = s.getAttribute('data-accent');
                applyTweaks();
                onTweakChange();
            });
        });

        ['layout', 'density'].forEach(function (key) {
            var sel = document.getElementById('tweak-' + key);
            if (!sel) return;
            sel.addEventListener('change', function () {
                tweaks[key] = sel.value;
                applyTweaks();
                onTweakChange();
            });
        });

        ['arcade', 'leaderboard', 'liveFps'].forEach(function (key) {
            var cb = document.getElementById('tweak-' + key);
            if (!cb) return;
            cb.addEventListener('change', function () {
                tweaks[key] = cb.checked;
                applyTweaks();
                if (key === 'liveFps') {
                    if (tweaks.liveFps) startLiveFps(onLiveTick);
                    else stopLiveFps();
                }
                onTweakChange();
            });
        });
    }

    // ── Boot ─────────────────────────────────────────────────────────────────
    var filter = { status: 'all', platform: 'all', audio: 'all' };
    var sort = 'featured';
    var gridEl, heroLbEl, tableEl, filterStatusEl;

    function onLiveTick(state) {
        if (tableEl) renderLeaderboard(tableEl, state);
        if (heroLbEl) renderHeroLeaderboard(heroLbEl, state);
        if (gridEl) {
            // Only update the tile FPS overlays in place (avoid full re-render churn).
            var tiles = gridEl.querySelectorAll('.port-tile');
            tiles.forEach(function (tile) {
                var href = tile.getAttribute('href') || '';
                var m = href.match(/([^/]+)\.html$/);
                var id = m ? m[1] : '';
                var v = state[id];
                var fpsEl = tile.querySelector('.port-tile__screenshot-fps');
                if (v != null) {
                    if (!fpsEl) {
                        fpsEl = el('div', { class: 'port-tile__screenshot-fps' });
                        tile.querySelector('.port-tile__screenshot').appendChild(fpsEl);
                    }
                    fpsEl.textContent = v + ' FPS';
                } else if (fpsEl) {
                    fpsEl.remove();
                }
            });
        }
    }

    function render() {
        var filtered = applyFilterSort(PORTS, filter, sort);
        renderGrid(gridEl, filtered, tweaks.layout, liveFpsState);
        if (filterStatusEl) {
            filterStatusEl.textContent = 'Ports * ' + filtered.length + ' of ' + STATS.total;
        }
        renderLeaderboard(tableEl, liveFpsState);
        renderHeroLeaderboard(heroLbEl, liveFpsState);
    }

    function populateStats() {
        document.getElementById('stat-playable').textContent = STATS.playable + '/' + STATS.total;
        document.getElementById('stat-playable-sub').textContent = STATS.wip + ' WIP * ' + STATS.planned + ' planned';
        document.getElementById('stat-avg-fps').textContent = STATS.avgFps + ' FPS';
        document.getElementById('stat-files').textContent = STATS.totalFiles.toLocaleString();
        document.getElementById('stat-files-sub').textContent = STATS.totalSourceMods + ' upstream modifications';
        var lb = document.getElementById('last-build');
        if (lb) {
            var vc = PORTS.find(function (p) { return p.id === 'vanilla-conquer'; });
            if (vc) lb.textContent = vc.lastBuild;
        }
    }

    function bindFilters() {
        ['status', 'platform', 'audio'].forEach(function (key) {
            var sel = document.getElementById('filter-' + key);
            if (!sel) return;
            sel.addEventListener('change', function () {
                filter[key] = sel.value;
                render();
            });
        });
        var sortSel = document.getElementById('filter-sort');
        if (sortSel) {
            sortSel.addEventListener('change', function () {
                sort = sortSel.value;
                render();
            });
        }
    }

    function init() {
        gridEl = document.getElementById('port-grid');
        heroLbEl = document.getElementById('hero-leaderboard');
        tableEl = document.getElementById('leaderboard-body');
        filterStatusEl = document.getElementById('filter-count');

        initLiveFps();
        populateStats();
        render();
        bindFilters();
        bindSubmitForm();
        bindTweaks(render);
        applyTweaks();

        if (tweaks.liveFps) startLiveFps(onLiveTick);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
