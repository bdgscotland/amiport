/**
 * stats.js — amiport statistics page
 * Fetches /api/v1/stats.php, renders bar chart and vote table.
 */
(function() {
    'use strict';

    var API_URL = 'api/v1/stats.php';

    function fetchStats() {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', API_URL, true);
        xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
                try {
                    var data = JSON.parse(xhr.responseText);
                    if (data.ok === false) {
                        showError();
                        return;
                    }
                    render(data);
                } catch (e) {
                    showError();
                }
            } else {
                showError();
            }
        };
        xhr.onerror = function() { showError(); };
        xhr.send();
    }

    function showError() {
        document.getElementById('stats-error').classList.remove('hidden');
        document.getElementById('stats-overview').classList.add('hidden');
    }

    function render(data) {
        // Total downloads
        var totalEl = document.querySelector('.stats-big-number__value');
        if (data.total_downloads === 0) {
            document.getElementById('stats-empty').classList.remove('hidden');
            document.getElementById('stats-overview').classList.add('hidden');
            document.getElementById('bar-chart').parentElement.parentElement.classList.add('hidden');
            return;
        }
        totalEl.textContent = data.total_downloads.toLocaleString();

        // Trends
        var trends = data.trends || {};
        setText('trend-7d', trends['7d'] || 0);
        setText('trend-30d', trends['30d'] || 0);
        setText('trend-90d', trends['90d'] || 0);

        // Bar chart
        renderBarChart(data.popular || []);

        // Voted table
        renderVotedTable(data.popular || []);
    }

    function setText(id, val) {
        var el = document.getElementById(id);
        if (el) el.textContent = val.toLocaleString ? val.toLocaleString() : val;
    }

    function renderBarChart(popular) {
        var container = document.getElementById('bar-chart');
        if (!popular.length) {
            container.textContent = 'No download data yet.';
            return;
        }

        var maxCount = popular[0].downloads || 1;

        for (var i = 0; i < popular.length; i++) {
            var pkg = popular[i];
            var pct = Math.round((pkg.downloads / maxCount) * 100);

            var row = document.createElement('div');
            row.className = 'bar-chart__row';
            row.setAttribute('aria-label', pkg.name + ': ' + pkg.downloads + ' downloads');

            var label = document.createElement('span');
            label.className = 'bar-chart__label';
            label.textContent = pkg.name;

            var barWrap = document.createElement('div');
            barWrap.className = 'bar-chart__bar-wrap';

            var bar = document.createElement('div');
            bar.className = 'bar-chart__bar';
            bar.style.width = pct + '%';

            var count = document.createElement('span');
            count.className = 'bar-chart__count';
            count.textContent = pkg.downloads;

            barWrap.appendChild(bar);
            row.appendChild(label);
            row.appendChild(barWrap);
            row.appendChild(count);
            container.appendChild(row);
        }
    }

    function renderVotedTable(popular) {
        var tbody = document.getElementById('voted-tbody');
        var emptyEl = document.getElementById('voted-empty');
        var table = document.getElementById('voted-table');

        // Filter to packages with at least one vote
        var voted = popular.filter(function(p) { return p.votes_up > 0 || p.votes_down > 0; });

        if (voted.length === 0) {
            table.classList.add('hidden');
            emptyEl.classList.remove('hidden');
            return;
        }

        // Sort by score descending
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

    fetchStats();
})();
