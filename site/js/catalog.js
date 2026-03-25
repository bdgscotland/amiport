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

    // DOM refs
    var tbody       = document.getElementById('cat-tbody');
    var searchInput = document.getElementById('cat-search');
    var categorySel = document.getElementById('cat-category');
    var tierSel     = document.getElementById('cat-tier');
    var sortSel     = document.getElementById('cat-sort');
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
        var sortKey = sortSel.value;
        result.sort(function(a, b) {
            if (sortKey === 'score') return ((b.readiness || {}).score || 0) - ((a.readiness || {}).score || 0);
            if (sortKey === 'complexity') return ((b.analysis || {}).source_complexity || 0) - ((a.analysis || {}).source_complexity || 0);
            return (a.name || '').localeCompare(b.name || '');
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
                el('td', null, hwDots(c.hardware_fit))
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

        if (c.notes) {
            right.appendChild(el('div', { style: { fontSize: '13px', color: '#4A4A4A', marginTop: '8px', fontStyle: 'italic' } }, c.notes));
        }

        // Wrap in a detail row spanning all columns
        var content = el('div', { style: { display: 'flex', gap: '24px', flexWrap: 'wrap', padding: '8px 0' } }, [left, right]);
        var td = el('td', { colspan: '6', style: { background: '#A8A8A8', borderTop: '1px solid #606060' } }, content);
        return el('tr', { className: 'cat-detail-row' }, td);
    }

    // --- Shim ROI Table ---

    function renderShimTable() {
        var unlocks = (catalog.summary || {}).top_unlocks || [];
        if (unlocks.length === 0) {
            shimEmpty.classList.remove('hidden');
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
    sortSel.addEventListener('change', function() { expandedId = null; renderCandidates(); });

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
