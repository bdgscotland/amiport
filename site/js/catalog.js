/**
 * catalog.js — amiport porting catalog browser
 *
 * Detail expands INLINE below the clicked row (not a separate section).
 * No dependencies. No framework. No innerHTML (XSS-safe DOM construction).
 */
(function() {
    'use strict';

    var API_URL = 'api/v1/catalog.php?_=' + Date.now();
    var catalog = null;
    var candidates = [];
    var expandedId = null;
    var currentSort = { key: 'score', dir: 'desc' };

    // DOM refs
    var tbody       = document.getElementById('cat-tbody');
    var searchInput = document.getElementById('cat-search');
    var categorySel = document.getElementById('cat-category');
    var tierSel     = document.getElementById('cat-tier');
    var catTable    = document.getElementById('cat-table');
    var countEl     = document.getElementById('cat-count');
    var emptyEl     = document.getElementById('cat-empty');
    var errorEl     = document.getElementById('cat-error');
    var shimTbody   = document.getElementById('shim-tbody');
    var shimEmpty   = document.getElementById('shim-empty');
    var sumReady    = document.getElementById('cat-ready');
    var sumFeasible = document.getElementById('cat-feasible');
    var sumBlocked  = document.getElementById('cat-blocked');
    var sumPorted   = document.getElementById('cat-ported');
    var sumShim     = document.getElementById('cat-shim');

    var tierColors = {
        'ready': '#448844', 'feasible': '#B8860B', 'blocked': '#BB4444',
        'infeasible': '#4A4A4A', 'unanalyzed': '#606060'
    };

    var fitSymbols = {
        'fits': '\u25CF', 'tight': '\u25D0', 'needs_fast_ram': '\u25CB',
        'stack_risk': '\u26A0', 'exceeds': '\u2715', 'unknown': '?'
    };
    var fitColors = {
        'fits': '#448844', 'tight': '#B8860B', 'exceeds': '#BB4444',
        'stack_risk': '#BB4444', 'needs_fast_ram': '#606060', 'unknown': '#606060'
    };
    var fitLabels = {
        'fits': 'Fits', 'tight': 'Tight fit', 'needs_fast_ram': 'Needs Fast RAM',
        'stack_risk': 'Stack too deep', 'exceeds': 'Won\'t fit', 'unknown': 'Not analyzed'
    };

    var VOTE_API = 'api/v1/catalog-vote.php';

    // --- Safe DOM helpers ---

    function el(tag, attrs, children) {
        var node = document.createElement(tag);
        if (attrs) {
            for (var k in attrs) {
                if (k === 'style' && typeof attrs[k] === 'object') {
                    for (var s in attrs[k]) node.style[s] = attrs[k][s];
                } else if (k === 'className') {
                    node.className = attrs[k];
                } else {
                    node.setAttribute(k, attrs[k]);
                }
            }
        }
        if (children != null) {
            if (!Array.isArray(children)) children = [children];
            for (var i = 0; i < children.length; i++) {
                var child = children[i];
                if (typeof child === 'string') node.appendChild(document.createTextNode(child));
                else if (child) node.appendChild(child);
            }
        }
        return node;
    }

    function text(s) { return document.createTextNode(s || ''); }

    function clearNode(n) { while (n.firstChild) n.removeChild(n.firstChild); }

    function tierBadge(tier) {
        return el('span', { className: 'badge', style: {
            background: tierColors[tier] || '#606060', color: '#fff',
            padding: '1px 6px', fontSize: '12px', display: 'inline-block'
        }}, tier || 'unknown');
    }

    function valueBadge(val) {
        if (!val) return text('--');
        var colors = { 5: '#448844', 4: '#6B8E23', 3: '#B8860B', 2: '#4A4A4A', 1: '#BB4444' };
        var labels = { 5: 'essential', 4: 'valuable', 3: 'useful', 2: 'low', 1: 'skip' };
        return el('span', { style: {
            color: colors[val] || '#4A4A4A', fontSize: '12px',
            fontWeight: val >= 4 ? 'bold' : 'normal'
        }}, labels[val] || String(val));
    }

    function aminetBadge(status) {
        if (!status) return el('span', { className: 'text-muted', style: { fontSize: '12px' } }, '?');
        var color = status === 'missing' ? '#448844' :
                   status === 'outdated' ? '#B8860B' : '#4A4A4A';
        var label = status === 'missing' ? 'missing' :
                   status === 'outdated' ? 'outdated' : 'exists';
        return el('span', { style: {
            color: color, fontSize: '12px', fontWeight: status === 'missing' ? 'bold' : 'normal'
        }}, label);
    }

    function hwDots(hw) {
        var wrapper = el('span', { style: { whiteSpace: 'nowrap' } });
        var profiles = ['stock_a1200', 'a1200_accel', 'a4000_030'];
        var labels = ['A1200', '+Accel', 'A4000'];
        for (var i = 0; i < profiles.length; i++) {
            var fit = (hw || {})[profiles[i]] || 'unknown';
            wrapper.appendChild(el('span', {
                title: labels[i] + ': ' + (fitLabels[fit] || ''),
                style: { color: fitColors[fit] || '#606060', cursor: 'default' }
            }, fitSymbols[fit] || '?'));
            if (i < 2) wrapper.appendChild(text(' '));
        }
        return wrapper;
    }

    function voteCell(c) {
        var v = c.community_votes || { up: 0, down: 0, score: 0 };
        var wrapper = el('span', { style: { whiteSpace: 'nowrap', display: 'inline-flex', alignItems: 'center', gap: '2px' } });

        var upBtn = el('button', {
            'aria-label': 'Upvote ' + (c.name || c.id),
            title: 'Upvote',
            className: 'vote-btn',
            style: { background: 'none', border: 'none', cursor: 'pointer', padding: '0 2px', fontSize: '11px', color: '#448844', lineHeight: '1' }
        }, '\u25B2');
        upBtn.addEventListener('click', function(e) {
            e.stopPropagation();
            castVote(c, 1);
        });

        var scoreSpan = el('span', {
            className: 'vote-score',
            'data-slug': c.id,
            style: { fontSize: '12px', minWidth: '20px', textAlign: 'center', fontWeight: v.score > 0 ? 'bold' : 'normal', color: v.score > 0 ? '#448844' : v.score < 0 ? '#BB4444' : '#606060' }
        }, String(v.score));

        var downBtn = el('button', {
            'aria-label': 'Downvote ' + (c.name || c.id),
            title: 'Downvote',
            className: 'vote-btn',
            style: { background: 'none', border: 'none', cursor: 'pointer', padding: '0 2px', fontSize: '11px', color: '#BB4444', lineHeight: '1' }
        }, '\u25BC');
        downBtn.addEventListener('click', function(e) {
            e.stopPropagation();
            castVote(c, -1);
        });

        wrapper.appendChild(upBtn);
        wrapper.appendChild(scoreSpan);
        wrapper.appendChild(downBtn);
        return wrapper;
    }

    function castVote(c, vote) {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', VOTE_API, true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
                try {
                    var resp = JSON.parse(xhr.responseText);
                    if (resp.ok) {
                        // Update candidate data
                        c.community_votes = { up: resp.votes_up, down: resp.votes_down, score: resp.score };
                        // Update displayed score
                        var scoreEls = document.querySelectorAll('.vote-score[data-slug="' + c.id + '"]');
                        for (var i = 0; i < scoreEls.length; i++) {
                            clearNode(scoreEls[i]);
                            scoreEls[i].appendChild(text(String(resp.score)));
                            scoreEls[i].style.fontWeight = resp.score > 0 ? 'bold' : 'normal';
                            scoreEls[i].style.color = resp.score > 0 ? '#448844' : resp.score < 0 ? '#BB4444' : '#606060';
                        }
                    }
                } catch (e) { /* ignore parse errors */ }
            }
        };
        xhr.send(JSON.stringify({ slug: c.id, vote: vote }));
    }

    // --- Fetch ---

    function fetchCatalog() {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', API_URL, true);
        xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
                try {
                    catalog = JSON.parse(xhr.responseText);
                    candidates = catalog.candidates || [];
                    renderSummary();
                    renderCandidates();
                    renderShimTable();
                } catch (e) { showError(); }
            } else { showError(); }
        };
        xhr.onerror = function() { showError(); };
        xhr.send();
    }

    function showError() {
        errorEl.classList.remove('hidden');
        clearNode(tbody);
    }

    // --- Summary ---

    function renderSummary() {
        var s = catalog.summary || {};
        var t = s.tiers || {};
        function set(node, label, val) {
            clearNode(node);
            node.appendChild(text(label + ': '));
            node.appendChild(el('strong', null, String(val)));
        }
        set(sumReady,    'Ready',    t.ready || 0);
        set(sumFeasible, 'Feasible', t.feasible || 0);
        set(sumBlocked,  'Blocked',  (t.blocked || 0) + (t.infeasible || 0));
        set(sumPorted,   'Ported',   s.ported || 0);
        set(sumShim,     'Shims',    (s.shim_implemented || 0) + '/' + (s.shim_total || 0));
    }

    // --- Filter + Sort ---

    function getFiltered() {
        var q = (searchInput.value || '').toLowerCase();
        var cat = categorySel.value;
        var tier = tierSel.value;
        var result = [];
        for (var i = 0; i < candidates.length; i++) {
            var c = candidates[i];
            if (cat && c.amiport_category !== cat) continue;
            var cTier = (c.readiness || {}).tier || 'unanalyzed';
            if (tier && cTier !== tier) continue;
            if (q && ((c.name || '') + ' ' + (c.description || '') + ' ' + (c.notes || '')).toLowerCase().indexOf(q) === -1) continue;
            result.push(c);
        }
        var sk = currentSort.key;
        var dir = currentSort.dir === 'asc' ? 1 : -1;
        result.sort(function(a, b) {
            var va, vb;
            if (sk === 'name') { va = a.name || ''; vb = b.name || ''; return dir * va.localeCompare(vb); }
            if (sk === 'score') { va = (a.readiness || {}).score || 0; vb = (b.readiness || {}).score || 0; }
            else if (sk === 'tier') {
                var order = { ready: 4, feasible: 3, blocked: 2, infeasible: 1, unanalyzed: 0 };
                va = order[(a.readiness || {}).tier] || 0; vb = order[(b.readiness || {}).tier] || 0;
            }
            else if (sk === 'source') { va = (a.upstream || {}).source || ''; vb = (b.upstream || {}).source || ''; return dir * va.localeCompare(vb); }
            else if (sk === 'aminet') {
                var ao = { missing: 3, outdated: 2, exists: 1 };
                va = ao[a.aminet_status] || 0; vb = ao[b.aminet_status] || 0;
            }
            else if (sk === 'value') { va = a.community_value || 0; vb = b.community_value || 0; }
            else if (sk === 'votes') { va = (a.community_votes || {}).score || 0; vb = (b.community_votes || {}).score || 0; }
            else { va = 0; vb = 0; }
            return dir * (va - vb);
        });
        return result;
    }

    // --- Render candidates with inline expand ---

    function renderCandidates() {
        var filtered = getFiltered();
        countEl.textContent = filtered.length + ' of ' + candidates.length + ' candidates';
        if (filtered.length === 0 && candidates.length > 0) {
            emptyEl.classList.remove('hidden');
            clearNode(tbody);
            return;
        }
        emptyEl.classList.add('hidden');
        clearNode(tbody);

        for (var i = 0; i < filtered.length; i++) {
            var c = filtered[i];
            var r = c.readiness || {};
            var score = r.score != null ? r.score.toFixed(2) : '--';
            var tier = r.tier || 'unanalyzed';

            // Main row
            var tr = el('tr', {
                className: 'cat-row' + (expandedId === c.id ? ' cat-row--expanded' : ''),
                'data-id': c.id,
                tabindex: '0',
                style: { cursor: 'pointer' }
            }, [
                el('td', null, el('strong', null, c.name || c.id)),
                el('td', null, score),
                el('td', null, tierBadge(tier)),
                el('td', null, c.description || ''),
                el('td', null, (c.upstream || {}).source || '?'),
                el('td', null, hwDots(c.hardware_fit)),
                el('td', null, aminetBadge(c.aminet_status)),
                el('td', null, valueBadge(c.community_value)),
                el('td', null, voteCell(c))
            ]);
            tr.addEventListener('click', toggleDetail);
            tr.addEventListener('keydown', function(e) { if (e.key === 'Enter') toggleDetail.call(this, e); });
            tbody.appendChild(tr);

            // Inline detail row (only if expanded)
            if (expandedId === c.id) {
                tbody.appendChild(buildDetailRow(c));
            }
        }
    }

    function toggleDetail(e) {
        var id = this.getAttribute('data-id');
        var clickedRow = this;

        // Remove any existing detail row
        var existing = tbody.querySelector('.cat-detail-row');
        if (existing) existing.parentNode.removeChild(existing);

        // Also un-highlight previous row
        var prev = tbody.querySelector('.cat-row--expanded');
        if (prev) prev.className = prev.className.replace(' cat-row--expanded', '');

        if (expandedId === id) {
            // Collapse — already removed above
            expandedId = null;
        } else {
            // Expand — insert detail row right after clicked row
            expandedId = id;
            clickedRow.className += ' cat-row--expanded';
            var c = null;
            for (var i = 0; i < candidates.length; i++) {
                if (candidates[i].id === id) { c = candidates[i]; break; }
            }
            if (c) {
                var detailRow = buildDetailRow(c);
                if (clickedRow.nextSibling) {
                    tbody.insertBefore(detailRow, clickedRow.nextSibling);
                } else {
                    tbody.appendChild(detailRow);
                }
            }
        }
    }

    function buildDetailRow(c) {
        var a = c.analysis || {};
        var mix = a.posix_tier_mix || {};
        var u = c.upstream || {};
        var hw = c.hardware_fit || {};
        var profiles = catalog.hardware_profiles || {};

        // Build detail content
        var cells = [];

        // Left column: analysis
        var left = el('div', { style: { flex: '1', minWidth: '250px' } });

        // Score + tier
        left.appendChild(el('div', { style: { marginBottom: '8px' } }, [
            el('strong', { style: { fontSize: '18px' } }, c.name || c.id),
            text('  '),
            tierBadge((c.readiness || {}).tier || 'unanalyzed'),
            text('  '),
            el('span', { className: 'text-muted' }, c.amiport_category || '')
        ]));

        if (c.description) left.appendChild(el('p', { style: { margin: '0 0 8px 0' } }, c.description));

        if (a.posix_tier_mix) {
            left.appendChild(el('div', { style: { fontSize: '13px', marginBottom: '4px' } }, [
                text('POSIX calls: '),
                el('span', { style: { color: '#448844' } }, 'T1:' + (mix.tier1 || 0)),
                text(' '),
                el('span', { style: { color: '#B8860B' } }, 'T2:' + (mix.tier2 || 0)),
                text(' '),
                el('span', { style: { color: '#BB4444' } }, 'T3:' + (mix.tier3 || 0))
            ]));
        }

        var missing = a.missing_shims || [];
        if (missing.length > 0) {
            left.appendChild(el('div', { style: { fontSize: '13px', marginBottom: '4px' } }, [
                text('Missing: '), el('span', { style: { color: '#BB4444' } }, missing.join(', '))
            ]));
        }
        if (a.blocker_workaround) {
            left.appendChild(el('div', { style: { fontSize: '13px', marginBottom: '4px', color: '#4A4A4A' } },
                'Workaround: ' + a.blocker_workaround));
        }
        if (a.estimated_binary_kb) {
            left.appendChild(el('div', { style: { fontSize: '13px', color: '#4A4A4A' } },
                'Est. binary: ' + a.estimated_binary_kb + 'KB, stack: ' + (a.estimated_stack_kb || '?') + 'KB'));
        }

        // Right column: hardware + upstream
        var right = el('div', { style: { flex: '1', minWidth: '200px' } });

        right.appendChild(el('div', { style: { fontSize: '13px', fontWeight: 'bold', marginBottom: '4px' } }, 'Hardware'));
        for (var p in profiles) {
            var fit = hw[p] || 'unknown';
            right.appendChild(el('div', { style: { fontSize: '13px', marginBottom: '2px' } }, [
                el('span', { style: { color: fitColors[fit] || '#606060' } }, (fitSymbols[fit] || '?') + ' '),
                text((profiles[p].label || p) + ' -- ' + (fitLabels[fit] || fit))
            ]));
        }

        right.appendChild(el('div', { style: { fontSize: '13px', fontWeight: 'bold', marginTop: '8px', marginBottom: '4px' } }, 'Source'));
        right.appendChild(el('div', { style: { fontSize: '13px' } }, [
            text((u.source || '?') + ' v' + (u.version || '?') + ' (' + (u.license || '?') + ')')
        ]));
        if (u.lines_of_code) {
            right.appendChild(el('div', { style: { fontSize: '13px', color: '#4A4A4A' } },
                u.lines_of_code + ' lines, ' + (u.files || '?') + ' file(s)'));
        }

        // Aminet status
        if (c.aminet_status) {
            right.appendChild(el('div', { style: { fontSize: '13px', fontWeight: 'bold', marginTop: '8px', marginBottom: '4px' } }, 'Aminet'));
            right.appendChild(el('div', { style: { fontSize: '13px' } }, [
                aminetBadge(c.aminet_status),
                text(c.aminet_notes ? ' -- ' + c.aminet_notes : '')
            ]));
        }

        // Community value
        if (c.community_value) {
            right.appendChild(el('div', { style: { fontSize: '13px', fontWeight: 'bold', marginTop: '8px', marginBottom: '4px' } }, 'Community Value'));
            right.appendChild(el('div', { style: { fontSize: '13px' } }, [
                valueBadge(c.community_value),
                text(' -- ' + (c.community_value_reason || ''))
            ]));
        }

        if (c.notes) {
            right.appendChild(el('div', { style: { fontSize: '13px', color: '#4A4A4A', marginTop: '8px', fontStyle: 'italic' } }, c.notes));
        }

        // Wrap in a detail row spanning all columns
        var content = el('div', { style: { display: 'flex', gap: '24px', flexWrap: 'wrap', padding: '8px 0' } }, [left, right]);
        var td = el('td', { colspan: '9', style: { background: '#A8A8A8', borderTop: '1px solid #606060' } }, content);
        return el('tr', { className: 'cat-detail-row' }, td);
    }

    // --- Shim ROI Table ---

    function renderShimTable() {
        var unlocks = (catalog.summary || {}).top_unlocks || [];

        // Populate compact summary on catalog page (if present)
        var summaryText = document.getElementById('shim-summary-text');
        if (summaryText) {
            var shimCount = Object.keys(catalog.shim_functions || {}).length;
            var topName = unlocks.length > 0 ? unlocks[0].name : '';
            var topUnlocks = unlocks.length > 0 ? unlocks[0].unlocks : 0;
            summaryText.textContent = shimCount + ' POSIX functions tracked. ' +
                (topName ? 'Top unlock: ' + topName + ' (' + topUnlocks + ' candidates). ' : '');
        }

        // Full table only renders on shims.html (where #shim-tbody exists)
        if (!shimTbody) return;

        if (unlocks.length === 0) {
            if (shimEmpty) shimEmpty.classList.remove('hidden');
            clearNode(shimTbody);
            return;
        }
        clearNode(shimTbody);
        for (var i = 0; i < unlocks.length; i++) {
            var s = unlocks[i];
            var shimData = (catalog.shim_functions || {})[s.name] || {};
            var programList = shimData.unlocks || [];
            var programs = programList.length > 0
                ? programList.slice(0, 6).join(', ') + (programList.length > 6 ? ', ...' : '')
                : 'pending analysis';

            shimTbody.appendChild(el('tr', null, [
                el('td', null, el('strong', null, s.name)),
                el('td', null, 'T' + s.tier),
                el('td', null, s.complexity || '?'),
                el('td', null, el('strong', null, s.unlocks > 0 ? String(s.unlocks) : '--')),
                el('td', { className: 'text-muted' }, programs)
            ]));
        }
    }

    // --- Events ---

    searchInput.addEventListener('input', function() { expandedId = null; renderCandidates(); });
    categorySel.addEventListener('change', function() { expandedId = null; renderCandidates(); });
    tierSel.addEventListener('change', function() { expandedId = null; renderCandidates(); });

    // Sortable column headers
    var sortHeaders = catTable.querySelectorAll('th.sortable');
    for (var si = 0; si < sortHeaders.length; si++) {
        sortHeaders[si].addEventListener('click', function() {
            var key = this.getAttribute('data-sort');
            if (currentSort.key === key) {
                currentSort.dir = currentSort.dir === 'desc' ? 'asc' : 'desc';
            } else {
                currentSort.key = key;
                currentSort.dir = (key === 'name' || key === 'source') ? 'asc' : 'desc';
            }
            updateSortIndicators();
            expandedId = null;
            renderCandidates();
        });
    }

    function updateSortIndicators() {
        var headers = catTable.querySelectorAll('th.sortable');
        for (var i = 0; i < headers.length; i++) {
            var h = headers[i];
            var base = h.textContent.replace(/ [\u25B2\u25BC]$/, '');
            if (h.getAttribute('data-sort') === currentSort.key) {
                h.textContent = base + (currentSort.dir === 'asc' ? ' \u25B2' : ' \u25BC');
            } else {
                h.textContent = base;
            }
        }
    }

    document.getElementById('cat-clear-filter').addEventListener('click', function(e) {
        e.preventDefault();
        searchInput.value = '';
        categorySel.value = '';
        tierSel.value = '';
        expandedId = null;
        renderCandidates();
    });

    document.getElementById('cat-retry').addEventListener('click', function(e) {
        e.preventDefault();
        errorEl.classList.add('hidden');
        fetchCatalog();
    });

    document.addEventListener('keydown', function(e) {
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT' || e.target.tagName === 'TEXTAREA') {
            if (e.key === 'Escape') { searchInput.value = ''; searchInput.blur(); expandedId = null; renderCandidates(); }
            return;
        }
        if (e.key === '/') { e.preventDefault(); searchInput.focus(); }
        if (e.key === 'Escape' && expandedId) { expandedId = null; renderCandidates(); }
    });

    // --- Init ---
    fetchCatalog();
})();
