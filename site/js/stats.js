/**
 * stats.js — amiport statistics page
 *
 * Fetches /api/v1/stats.php and /api/v1/packages.php,
 * renders big numbers, SVG bar charts, category breakdown,
 * publication timeline, and vote table.
 * No dependencies. No charting library.
 */
(function() {
    'use strict';

    var STATS_URL = 'api/v1/stats.php';
    var PACKAGES_URL = 'api/v1/packages.php?_=' + Date.now();

    var allPackages = [];

    function fetchJSON(url, cb) {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
                try { cb(null, JSON.parse(xhr.responseText)); }
                catch (e) { cb(e); }
            } else { cb(new Error('HTTP ' + xhr.status)); }
        };
        xhr.onerror = function() { cb(new Error('Network error')); };
        xhr.send();
    }

    function showError() {
        document.getElementById('stats-error').classList.remove('hidden');
    }

    function setText(id, val) {
        var el = document.getElementById(id);
        if (el) el.textContent = val.toLocaleString ? val.toLocaleString() : val;
    }

    // --- SVG bar chart ---
    function renderSVGBarChart(container, items, labelKey, valueKey) {
        if (!items.length) {
            container.textContent = 'No data yet.';
            return;
        }

        var maxVal = 0;
        for (var i = 0; i < items.length; i++) {
            if ((items[i][valueKey] || 0) > maxVal) maxVal = items[i][valueKey];
        }
        if (maxVal === 0) maxVal = 1;

        var barHeight = 22;
        var gap = 4;
        var labelWidth = 70;
        var countWidth = 50;
        var chartWidth = 600;
        var barAreaWidth = chartWidth - labelWidth - countWidth - 10;
        var svgHeight = items.length * (barHeight + gap);

        var ns = 'http://www.w3.org/2000/svg';
        var svg = document.createElementNS(ns, 'svg');
        svg.setAttribute('width', '100%');
        svg.setAttribute('viewBox', '0 0 ' + chartWidth + ' ' + svgHeight);
        svg.setAttribute('role', 'img');

        // Build aria-label
        var ariaLabel = 'Downloads per package: ';
        for (var j = 0; j < items.length; j++) {
            ariaLabel += items[j][labelKey] + ' ' + (items[j][valueKey] || 0);
            if (j < items.length - 1) ariaLabel += ', ';
        }
        svg.setAttribute('aria-label', ariaLabel);

        for (var k = 0; k < items.length; k++) {
            var item = items[k];
            var val = item[valueKey] || 0;
            var y = k * (barHeight + gap);
            var barW = Math.max(2, (val / maxVal) * barAreaWidth);

            // Label
            var text = document.createElementNS(ns, 'text');
            text.setAttribute('x', labelWidth - 4);
            text.setAttribute('y', y + barHeight / 2 + 4);
            text.setAttribute('text-anchor', 'end');
            text.setAttribute('font-size', '12');
            text.setAttribute('font-weight', 'bold');
            text.setAttribute('fill', '#776644');
            text.textContent = item[labelKey];
            svg.appendChild(text);

            // Bar background (inset bevel)
            var bgRect = document.createElementNS(ns, 'rect');
            bgRect.setAttribute('x', labelWidth);
            bgRect.setAttribute('y', y);
            bgRect.setAttribute('width', barAreaWidth);
            bgRect.setAttribute('height', barHeight);
            bgRect.setAttribute('fill', '#9A9A9A');
            bgRect.setAttribute('stroke', '#606060');
            bgRect.setAttribute('stroke-width', '1');
            svg.appendChild(bgRect);

            // Bar fill
            var barRect = document.createElementNS(ns, 'rect');
            barRect.setAttribute('x', labelWidth + 1);
            barRect.setAttribute('y', y + 1);
            barRect.setAttribute('width', barW);
            barRect.setAttribute('height', barHeight - 2);
            barRect.setAttribute('fill', '#CC9933');
            svg.appendChild(barRect);

            // Count
            var countText = document.createElementNS(ns, 'text');
            countText.setAttribute('x', labelWidth + barAreaWidth + 6);
            countText.setAttribute('y', y + barHeight / 2 + 4);
            countText.setAttribute('font-size', '12');
            countText.setAttribute('fill', '#4A4A4A');
            countText.textContent = val;
            svg.appendChild(countText);
        }

        container.appendChild(svg);
    }

    // --- Category breakdown (simple bar chart) ---
    function renderCategoryChart(packages) {
        var container = document.getElementById('category-chart');
        var cats = {};
        for (var i = 0; i < packages.length; i++) {
            var cat = packages[i].category || 'uncategorized';
            cats[cat] = (cats[cat] || 0) + 1;
        }

        var items = [];
        for (var key in cats) {
            items.push({ name: key, count: cats[key] });
        }
        items.sort(function(a, b) { return b.count - a.count; });

        if (!items.length) {
            container.textContent = 'No category data.';
            return;
        }

        var max = items[0].count;
        for (var j = 0; j < items.length; j++) {
            var pct = Math.round((items[j].count / max) * 100);
            var row = document.createElement('div');
            row.className = 'bar-chart__row';

            var label = document.createElement('span');
            label.className = 'bar-chart__label';
            label.textContent = items[j].name;

            var barWrap = document.createElement('div');
            barWrap.className = 'bar-chart__bar-wrap';
            var bar = document.createElement('div');
            bar.className = 'bar-chart__bar';
            bar.style.width = pct + '%';
            barWrap.appendChild(bar);

            var count = document.createElement('span');
            count.className = 'bar-chart__count';
            count.textContent = items[j].count;

            row.appendChild(label);
            row.appendChild(barWrap);
            row.appendChild(count);
            container.appendChild(row);
        }
    }

    // --- Publication timeline ---
    function renderTimeline(packages) {
        var container = document.getElementById('timeline-list');
        var withDates = packages.filter(function(p) {
            return p.published_at || p.publish_date;
        });

        withDates.sort(function(a, b) {
            var dateA = a.published_at || a.publish_date || '';
            var dateB = b.published_at || b.publish_date || '';
            return dateB.localeCompare(dateA);
        });

        if (!withDates.length) {
            container.innerHTML = '<li class="timeline-list__item"><span class="text-muted">No publication dates available.</span></li>';
            return;
        }

        for (var i = 0; i < withDates.length; i++) {
            var pkg = withDates[i];
            var dateStr = pkg.published_at || pkg.publish_date;
            var date = new Date(dateStr);
            var formatted = date.toLocaleDateString(undefined, {year:'numeric',month:'short',day:'numeric'});

            var li = document.createElement('li');
            li.className = 'timeline-list__item';

            var dateSpan = document.createElement('span');
            dateSpan.className = 'timeline-list__date';
            dateSpan.textContent = formatted;
            li.appendChild(dateSpan);

            var nameSpan = document.createElement('span');
            nameSpan.className = 'timeline-list__name';
            nameSpan.textContent = pkg.name;
            li.appendChild(nameSpan);

            var verSpan = document.createElement('span');
            verSpan.className = 'timeline-list__ver';
            verSpan.textContent = 'v' + (pkg.version || '');
            li.appendChild(verSpan);

            container.appendChild(li);
        }
    }

    // --- Voted table ---
    function renderVotedTable(popular) {
        var tbody = document.getElementById('voted-tbody');
        var emptyEl = document.getElementById('voted-empty');
        var table = document.getElementById('voted-table');

        var voted = popular.filter(function(p) { return p.votes_up > 0 || p.votes_down > 0; });

        if (voted.length === 0) {
            table.classList.add('hidden');
            emptyEl.classList.remove('hidden');
            return;
        }

        voted.sort(function(a, b) {
            return (b.votes_up - b.votes_down) - (a.votes_up - a.votes_down);
        });

        for (var i = 0; i < voted.length; i++) {
            var pkg = voted[i];
            var tr = document.createElement('tr');

            var tdName = document.createElement('td');
            tdName.textContent = pkg.name;
            tr.appendChild(tdName);

            var tdUp = document.createElement('td');
            tdUp.textContent = '+' + pkg.votes_up;
            tr.appendChild(tdUp);

            var tdDown = document.createElement('td');
            tdDown.textContent = '-' + pkg.votes_down;
            tr.appendChild(tdDown);

            var tdScore = document.createElement('td');
            var score = pkg.votes_up - pkg.votes_down;
            tdScore.textContent = (score >= 0 ? '+' : '') + score;
            tr.appendChild(tdScore);

            tbody.appendChild(tr);
        }
    }

    // --- Load data ---
    function init() {
        // Fetch both stats and packages
        fetchJSON(STATS_URL, function(err, statsData) {
            if (err) { showError(); return; }

            fetchJSON(PACKAGES_URL, function(err2, pkgData) {
                if (err2) { showError(); return; }

                var packages = pkgData.packages || [];
                allPackages = packages;

                // Big numbers
                setText('stat-ports', packages.length);
                setText('stat-downloads', statsData.total_downloads || 0);

                // Total tests
                var totalTests = 0;
                for (var i = 0; i < packages.length; i++) {
                    totalTests += packages[i].test_pass || 0;
                }
                setText('stat-tests', totalTests);

                if (statsData.total_downloads === 0) {
                    document.getElementById('stats-empty').classList.remove('hidden');
                }

                // SVG downloads chart
                var popular = statsData.popular || [];
                renderSVGBarChart(
                    document.getElementById('downloads-chart'),
                    popular, 'name', 'downloads'
                );

                // Category breakdown
                renderCategoryChart(packages);

                // Timeline
                renderTimeline(packages);

                // Voted table
                renderVotedTable(popular);
            });
        });
    }

    init();
})();
