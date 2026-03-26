/**
 * changelog.js — Fetch packages and render a publication timeline
 */
(function() {
    'use strict';

    var loadingEl = document.getElementById('changelog-loading');
    var emptyEl = document.getElementById('changelog-empty');
    var tableEl = document.getElementById('changelog-table');
    var tbodyEl = document.getElementById('changelog-tbody');

    if (!tbodyEl) return;

    var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];

    function formatDate(dateStr) {
        var d = new Date(dateStr);
        if (isNaN(d.getTime())) return dateStr || '';
        var day = d.getDate();
        var mon = months[d.getMonth()];
        var year = d.getFullYear();
        return day + ' ' + mon + ' ' + year;
    }

    function escapeText(str) {
        var el = document.createElement('span');
        el.textContent = str;
        return el.textContent;
    }

    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/api/v1/packages.php', true);
    xhr.onload = function() {
        loadingEl.classList.add('hidden');

        if (xhr.status !== 200) {
            emptyEl.textContent = 'Could not load changelog.';
            emptyEl.classList.remove('hidden');
            return;
        }

        var resp;
        try {
            resp = JSON.parse(xhr.responseText);
        } catch (e) {
            emptyEl.textContent = 'Could not parse package data.';
            emptyEl.classList.remove('hidden');
            return;
        }

        var packages = resp.packages || resp;
        if (!Array.isArray(packages)) {
            packages = [];
        }

        // Filter to published packages and sort by published_at descending
        var published = [];
        for (var i = 0; i < packages.length; i++) {
            var pkg = packages[i];
            var pubDate = pkg.published_at || pkg.publish_date || '';
            if (pubDate && (pkg.status || '') !== 'testing') {
                published.push(pkg);
            }
        }

        published.sort(function(a, b) {
            var da = a.published_at || a.publish_date || '';
            var db = b.published_at || b.publish_date || '';
            return db.localeCompare(da);
        });

        if (published.length === 0) {
            emptyEl.classList.remove('hidden');
            return;
        }

        for (var j = 0; j < published.length; j++) {
            var p = published[j];
            var tr = document.createElement('tr');

            var tdDate = document.createElement('td');
            tdDate.textContent = formatDate(p.published_at || p.publish_date || '');
            tr.appendChild(tdDate);

            var tdName = document.createElement('td');
            var link = document.createElement('a');
            link.href = 'packages.html?name=' + encodeURIComponent(p.name || '');
            link.textContent = p.name || '';
            tdName.appendChild(link);
            tr.appendChild(tdName);

            var tdVer = document.createElement('td');
            tdVer.textContent = p.version || '';
            tr.appendChild(tdVer);

            var tdDesc = document.createElement('td');
            tdDesc.textContent = p.description || '';
            tr.appendChild(tdDesc);

            tbodyEl.appendChild(tr);
        }

        tableEl.classList.remove('hidden');
    };

    xhr.onerror = function() {
        loadingEl.classList.add('hidden');
        emptyEl.textContent = 'Could not load changelog.';
        emptyEl.classList.remove('hidden');
    };

    xhr.send();
})();
