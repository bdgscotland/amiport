/**
 * packages.js — amiport package browser
 *
 * Fetches /api/v1/packages.json, renders the package table,
 * handles search/filter/sort, and shows package detail view.
 * No dependencies. No framework.
 */
(function() {
    'use strict';

    var API_URL = 'api/v1/packages.php?_=' + Date.now();
    var packages = [];
    var currentSort = { key: 'name', dir: 'asc' };

    // DOM refs
    var listView     = document.getElementById('pkg-list-view');
    var detailView   = document.getElementById('pkg-detail-view');
    var tbody        = document.getElementById('pkg-tbody');
    var searchInput  = document.getElementById('pkg-search');
    var categorySel  = document.getElementById('pkg-category');
    var countEl      = document.getElementById('pkg-count');
    var emptyEl      = document.getElementById('pkg-empty');
    var emptyQuery   = document.getElementById('pkg-empty-query');
    var errorEl      = document.getElementById('pkg-error');
    var table        = document.getElementById('pkg-table');

    // Detail DOM refs
    var detailTitle    = document.getElementById('pkg-detail-title');
    var detailName     = document.getElementById('pkg-detail-name');
    var detailVersion  = document.getElementById('pkg-detail-version');
    var detailCategory = document.getElementById('pkg-detail-category');
    var detailInstall  = document.getElementById('pkg-detail-install');
    var detailCopy     = document.getElementById('pkg-detail-copy');
    var detailDesc     = document.getElementById('pkg-detail-desc');
    var detailDownload = document.getElementById('pkg-detail-download');
    var detailSize     = document.getElementById('pkg-detail-size');
    var detailMeta     = document.getElementById('pkg-detail-meta');
    var notFoundEl     = document.getElementById('pkg-not-found');

    // --- Helpers ---

    function formatSize(bytes) {
        if (!bytes) return '—';
        if (bytes < 1024) return bytes + ' B';
        return Math.round(bytes / 1024) + ' KB';
    }

    function escapeHtml(str) {
        var div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }

    // --- Fetch packages ---

    function fetchPackages() {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', API_URL, true);
        xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
                try {
                    var data = JSON.parse(xhr.responseText);
                    packages = data.packages || [];
                    computeBadges();
                    populateCategories();
                    render();
                    checkUrlForDetail();
                } catch (e) {
                    showError();
                }
            } else {
                showError();
            }
        };
        xhr.onerror = function() {
            showError();
        };
        xhr.send();
    }

    function showError() {
        tbody.innerHTML = '';
        table.classList.add('hidden');
        errorEl.classList.remove('hidden');
        countEl.textContent = '';
    }

    // --- Compute badges ---

    function computeBadges() {
        // POPULAR: top 3 by download count
        var byDownloads = packages.slice().sort(function(a, b) {
            return (b.downloads || 0) - (a.downloads || 0);
        });
        for (var i = 0; i < packages.length; i++) {
            packages[i]._isPopular = false;
            packages[i]._isNew = false;
        }
        for (var j = 0; j < Math.min(3, byDownloads.length); j++) {
            if ((byDownloads[j].downloads || 0) > 0) {
                byDownloads[j]._isPopular = true;
            }
        }
        // NEW: we check added_at field if present, otherwise skip
        // (server-side filemtime check not available in browser)
        for (var k = 0; k < packages.length; k++) {
            if (packages[k].added_at) {
                var added = new Date(packages[k].added_at);
                var thirtyDaysAgo = new Date(Date.now() - 30 * 24 * 60 * 60 * 1000);
                packages[k]._isNew = added > thirtyDaysAgo;
            }
        }
    }

    // --- Submit vote ---

    function submitVote(packageName, vote, btn) {
        btn.disabled = true;
        var xhr = new XMLHttpRequest();
        xhr.open('POST', 'api/v1/vote.php', true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.onload = function() {
            btn.disabled = false;
            try {
                var resp = JSON.parse(xhr.responseText);
                if (resp.ok) {
                    // Update package data and re-render
                    for (var i = 0; i < packages.length; i++) {
                        if (packages[i].name === packageName) {
                            packages[i].votes_up = resp.votes_up;
                            packages[i].votes_down = resp.votes_down;
                            packages[i].vote_score = resp.score;
                            packages[i].your_vote = resp.your_vote || vote;
                            break;
                        }
                    }
                    computeBadges();
                    render();
                }
            } catch (e) {
                // silently fail
            }
        };
        xhr.onerror = function() { btn.disabled = false; };
        xhr.send(JSON.stringify({package: packageName, vote: vote}));
    }

    // --- Populate category dropdown ---

    function populateCategories() {
        var cats = {};
        for (var i = 0; i < packages.length; i++) {
            var cat = packages[i].category || 'uncategorized';
            cats[cat] = true;
        }
        var sorted = Object.keys(cats).sort();
        for (var j = 0; j < sorted.length; j++) {
            var opt = document.createElement('option');
            opt.value = sorted[j];
            opt.textContent = sorted[j];
            categorySel.appendChild(opt);
        }
    }

    // --- Render table ---

    function render() {
        var query = (searchInput.value || '').toLowerCase();
        var cat = categorySel.value;

        var filtered = packages.filter(function(pkg) {
            var matchesSearch = !query ||
                pkg.name.toLowerCase().indexOf(query) !== -1 ||
                (pkg.description || '').toLowerCase().indexOf(query) !== -1;
            var matchesCat = !cat || pkg.category === cat;
            return matchesSearch && matchesCat;
        });

        // Sort
        filtered.sort(function(a, b) {
            var aVal = (a[currentSort.key] || '').toString().toLowerCase();
            var bVal = (b[currentSort.key] || '').toString().toLowerCase();
            if (aVal < bVal) return currentSort.dir === 'asc' ? -1 : 1;
            if (aVal > bVal) return currentSort.dir === 'asc' ? 1 : -1;
            return 0;
        });

        // Update count
        countEl.textContent = filtered.length + ' package' + (filtered.length !== 1 ? 's' : '') +
            (query || cat ? ' matching' : ' available');

        // Show/hide states
        if (filtered.length === 0 && (query || cat)) {
            table.classList.add('hidden');
            emptyQuery.textContent = query || cat;
            emptyEl.classList.remove('hidden');
            errorEl.classList.add('hidden');
        } else {
            table.classList.remove('hidden');
            emptyEl.classList.add('hidden');
            errorEl.classList.add('hidden');
        }

        // Update sort indicators
        var ths = table.querySelectorAll('th.sortable');
        for (var t = 0; t < ths.length; t++) {
            ths[t].classList.remove('sort-asc', 'sort-desc');
            if (ths[t].getAttribute('data-sort') === currentSort.key) {
                ths[t].classList.add('sort-' + currentSort.dir);
            }
        }

        // Build rows (DOM construction — no innerHTML)
        while (tbody.firstChild) tbody.removeChild(tbody.firstChild);
        for (var i = 0; i < filtered.length; i++) {
            var pkg = filtered[i];
            var tr = document.createElement('tr');
            tr.className = 'clickable';
            tr.setAttribute('data-name', pkg.name);

            var tdName = document.createElement('td');
            var strong = document.createElement('strong');
            strong.textContent = pkg.name;
            tdName.appendChild(strong);

            // NEW badge (package JSON modified in last 30 days)
            if (pkg._isNew) {
                var newBadge = document.createElement('span');
                newBadge.className = 'badge badge--new';
                newBadge.textContent = 'NEW';
                newBadge.setAttribute('aria-label', 'New package');
                tdName.appendChild(document.createTextNode(' '));
                tdName.appendChild(newBadge);
            }

            // POPULAR badge (top 3 by downloads)
            if (pkg._isPopular) {
                var popBadge = document.createElement('span');
                popBadge.className = 'badge badge--blue';
                popBadge.textContent = 'POPULAR';
                popBadge.setAttribute('aria-label', 'Popular package');
                tdName.appendChild(document.createTextNode(' '));
                tdName.appendChild(popBadge);
            }

            // TESTING badge (not fully verified)
            if (pkg.status === 'testing') {
                var testBadge = document.createElement('span');
                testBadge.className = 'badge badge--warning';
                testBadge.textContent = 'TESTING';
                testBadge.setAttribute('aria-label', 'Package is in testing — download disabled');
                tdName.appendChild(document.createTextNode(' '));
                tdName.appendChild(testBadge);
            }

            tr.appendChild(tdName);

            var tdVer = document.createElement('td');
            tdVer.textContent = pkg.version || '';
            tr.appendChild(tdVer);

            var tdDesc = document.createElement('td');
            tdDesc.textContent = pkg.description || '';

            // Vote UI + download count below description
            var voteRow = document.createElement('div');
            voteRow.className = 'vote-group';
            voteRow.setAttribute('role', 'group');
            voteRow.setAttribute('aria-label', 'Vote on ' + pkg.name);

            var dlCount = document.createElement('span');
            dlCount.className = 'dl-count';
            dlCount.textContent = (pkg.downloads || 0) + ' dl';
            voteRow.appendChild(dlCount);

            var scoreSpan = document.createElement('span');
            scoreSpan.className = 'vote-score';
            var score = pkg.vote_score || 0;
            scoreSpan.textContent = (score >= 0 ? '+' : '') + score;
            voteRow.appendChild(scoreSpan);

            var yourVote = pkg.your_vote || 0;

            var btnUp = document.createElement('button');
            btnUp.className = 'btn btn--sm vote-btn vote-btn--up' + (yourVote === 1 ? ' vote-active' : '');
            btnUp.textContent = '\u25B2';
            btnUp.setAttribute('aria-label', 'Vote up');
            btnUp.setAttribute('aria-pressed', yourVote === 1 ? 'true' : 'false');
            if (yourVote !== 1) {
                btnUp.addEventListener('click', (function(name) {
                    return function(ev) { ev.stopPropagation(); submitVote(name, 1, ev.target); };
                })(pkg.name));
            } else {
                btnUp.disabled = true;
            }
            voteRow.appendChild(btnUp);

            var btnDown = document.createElement('button');
            btnDown.className = 'btn btn--sm vote-btn vote-btn--down' + (yourVote === -1 ? ' vote-active' : '');
            btnDown.textContent = '\u25BC';
            btnDown.setAttribute('aria-label', 'Vote down');
            btnDown.setAttribute('aria-pressed', yourVote === -1 ? 'true' : 'false');
            if (yourVote !== -1) {
                btnDown.addEventListener('click', (function(name) {
                    return function(ev) { ev.stopPropagation(); submitVote(name, -1, ev.target); };
                })(pkg.name));
            } else {
                btnDown.disabled = true;
            }
            voteRow.appendChild(btnDown);

            tdDesc.appendChild(voteRow);
            tr.appendChild(tdDesc);

            var tdCat = document.createElement('td');
            var badge = document.createElement('span');
            badge.className = 'badge';
            badge.textContent = pkg.category || '';
            tdCat.appendChild(badge);
            tr.appendChild(tdCat);

            var tdSize = document.createElement('td');
            tdSize.textContent = formatSize(pkg.size);
            tr.appendChild(tdSize);

            var tdAminet = document.createElement('td');
            if (pkg.aminet) {
                var aminetLink = document.createElement('a');
                aminetLink.href = pkg.aminet;
                aminetLink.target = '_blank';
                aminetLink.rel = 'noopener';
                aminetLink.textContent = 'Aminet';
                aminetLink.className = 'aminet-link';
                aminetLink.addEventListener('click', function(ev) { ev.stopPropagation(); });
                tdAminet.appendChild(aminetLink);
            }
            tr.appendChild(tdAminet);

            tbody.appendChild(tr);
        }
    }

    // --- Sort ---

    function handleSort(e) {
        var th = e.target.closest('th.sortable');
        if (!th) return;
        var key = th.getAttribute('data-sort');
        if (currentSort.key === key) {
            currentSort.dir = currentSort.dir === 'asc' ? 'desc' : 'asc';
        } else {
            currentSort.key = key;
            currentSort.dir = 'asc';
        }
        render();
    }

    // --- Row click → detail ---

    function handleRowClick(e) {
        var row = e.target.closest('tr.clickable');
        if (!row) return;
        var name = row.getAttribute('data-name');
        if (name) {
            showDetail(name);
            history.pushState(null, '', 'packages.html?name=' + encodeURIComponent(name));
        }
    }

    // --- Detail view ---

    function showDetail(name) {
        var pkg = null;
        for (var i = 0; i < packages.length; i++) {
            if (packages[i].name === name) {
                pkg = packages[i];
                break;
            }
        }

        listView.classList.add('hidden');
        detailView.classList.remove('hidden');
        notFoundEl.classList.add('hidden');

        if (!pkg) {
            notFoundEl.classList.remove('hidden');
            detailTitle.textContent = 'Not Found';
            return;
        }

        detailTitle.textContent = pkg.name;
        detailName.textContent = pkg.name;
        var verText = 'v' + (pkg.version || '');
        if (pkg.revision && pkg.revision > 1) {
            verText += '-' + pkg.revision;
        }
        detailVersion.textContent = verText;
        detailCategory.textContent = pkg.category || '';
        detailInstall.textContent = 'amiget install ' + pkg.name;
        // Show readme if available, otherwise just description
        if (pkg.readme) {
            var pre = document.createElement('pre');
            pre.className = 'pkg-readme';
            pre.textContent = pkg.readme;
            while (detailDesc.firstChild) detailDesc.removeChild(detailDesc.firstChild);
            detailDesc.appendChild(pre);
        } else {
            detailDesc.textContent = pkg.description || '';
        }
        detailSize.textContent = formatSize(pkg.size);

        // Download link (blocked for non-stable packages)
        if (pkg.download && pkg.status !== 'testing') {
            detailDownload.href = 'api/v1/download.php?name=' + encodeURIComponent(pkg.name);
            detailDownload.classList.remove('hidden', 'btn--disabled');
            var sizeText = formatSize(pkg.size);
            detailDownload.textContent = 'Download .lha' + (sizeText !== '—' ? ' (' + sizeText + ')' : '');
        } else if (pkg.status === 'testing') {
            detailDownload.removeAttribute('href');
            detailDownload.classList.remove('hidden');
            detailDownload.classList.add('btn--disabled');
            detailDownload.textContent = 'Download unavailable (testing)';
        } else {
            detailDownload.classList.add('hidden');
        }

        // Metadata (DOM construction — no innerHTML)
        while (detailMeta.firstChild) detailMeta.removeChild(detailMeta.firstChild);

        function addMetaRow(label, value) {
            var row = document.createElement('div');
            row.className = 'meta-row';
            var dt = document.createElement('dt');
            dt.textContent = label;
            var dd = document.createElement('dd');
            dd.textContent = value;
            row.appendChild(dt);
            row.appendChild(dd);
            detailMeta.appendChild(row);
        }

        function addMetaLink(label, url, text) {
            var row = document.createElement('div');
            row.className = 'meta-row';
            var dt = document.createElement('dt');
            dt.textContent = label;
            var dd = document.createElement('dd');
            var a = document.createElement('a');
            a.href = url;
            a.target = '_blank';
            a.rel = 'noopener';
            a.textContent = text;
            dd.appendChild(a);
            row.appendChild(dt);
            row.appendChild(dd);
            detailMeta.appendChild(row);
        }

        if (pkg.published_at) {
            var pubDate = new Date(pkg.published_at);
            addMetaRow('Published', pubDate.toLocaleDateString(undefined, {year:'numeric',month:'short',day:'numeric'}));
        }
        if (pkg.source)  addMetaRow('Source', pkg.source);
        if (pkg.license) addMetaRow('License', pkg.license);
        if (pkg.stack)   addMetaRow('Stack', pkg.stack + ' bytes');
        if (pkg.aminet)  addMetaLink('Aminet', pkg.aminet, 'View on Aminet');
        if (pkg.requires && pkg.requires.length > 0) {
            addMetaRow('Requires', pkg.requires.join(', '));
        }
        if (pkg.sha256) {
            var row = document.createElement('div');
            row.className = 'meta-row';
            var dt = document.createElement('dt');
            dt.textContent = 'SHA256';
            var dd = document.createElement('dd');
            dd.style.wordBreak = 'break-all';
            dd.textContent = pkg.sha256;
            row.appendChild(dt);
            row.appendChild(dd);
            detailMeta.appendChild(row);
        }

        // Copy button
        detailCopy.onclick = function() {
            var cmd = 'amiget install ' + pkg.name;
            if (navigator.clipboard) {
                navigator.clipboard.writeText(cmd).then(function() {
                    detailCopy.textContent = 'Copied';
                    setTimeout(function() { detailCopy.textContent = 'Copy'; }, 1500);
                });
            }
        };

        document.title = pkg.name + ' — amiport';
    }

    function showList() {
        listView.classList.remove('hidden');
        detailView.classList.add('hidden');
        document.title = 'Packages — amiport';
    }

    // --- Check URL for ?name= param ---

    function checkUrlForDetail() {
        var params = new URLSearchParams(window.location.search);
        var name = params.get('name');
        if (name) {
            showDetail(name);
        } else {
            showList();
        }
    }

    // --- Event listeners ---

    searchInput.addEventListener('input', render);
    categorySel.addEventListener('change', render);

    // Sort headers
    table.querySelector('thead').addEventListener('click', handleSort);

    // Row clicks
    tbody.addEventListener('click', handleRowClick);

    // Back link
    document.getElementById('pkg-back').addEventListener('click', function(e) {
        e.preventDefault();
        showList();
        history.pushState(null, '', 'packages.html');
    });

    // Browser back/forward
    window.addEventListener('popstate', function() {
        if (packages.length > 0) {
            checkUrlForDetail();
        }
    });

    // Clear filter
    document.getElementById('pkg-clear-filter').addEventListener('click', function(e) {
        e.preventDefault();
        searchInput.value = '';
        categorySel.value = '';
        render();
    });

    // Retry
    document.getElementById('pkg-retry').addEventListener('click', function(e) {
        e.preventDefault();
        errorEl.classList.add('hidden');
        fetchPackages();
    });

    // --- Init ---
    fetchPackages();

})();
