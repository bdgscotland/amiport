/**
 * packages.js — amiport package browser
 *
 * Fetches /api/v1/packages.json, renders the package table,
 * handles search/filter/sort, and shows package detail view.
 * No dependencies. No framework.
 */
(function() {
    'use strict';

    var API_URL = 'api/v1/packages.json';
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
            tr.appendChild(tdName);

            var tdVer = document.createElement('td');
            tdVer.textContent = pkg.version || '';
            tr.appendChild(tdVer);

            var tdDesc = document.createElement('td');
            tdDesc.textContent = pkg.description || '';
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
        detailVersion.textContent = 'v' + (pkg.version || '');
        detailCategory.textContent = pkg.category || '';
        detailInstall.textContent = 'amiget install ' + pkg.name;
        detailDesc.textContent = pkg.description || '';
        detailSize.textContent = formatSize(pkg.size);

        // Download link
        if (pkg.download) {
            detailDownload.href = 'api/v1/download.php?name=' + encodeURIComponent(pkg.name);
            detailDownload.classList.remove('hidden');
            var sizeText = formatSize(pkg.size);
            detailDownload.textContent = 'Download .lha' + (sizeText !== '—' ? ' (' + sizeText + ')' : '');
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
