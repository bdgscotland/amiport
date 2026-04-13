/**
 * news.js -- amiport news archive renderer
 *
 * Fetches data/news.json and renders each entry on news.html.
 * Supports a tiny markdown-lite subset in body text:
 *   - blank line    -> new paragraph
 *   - [text](url)   -> <a> link
 *   - **bold**      -> <strong>
 *   - `code`        -> <code>
 * Everything else is HTML-escaped.
 */
(function () {
    'use strict';

    // Served via api/v1/news.php because site/.htaccess blocks the
    // /data/ directory. The PHP endpoint just echoes data/news.json
    // after validating it parses.
    var NEWS_URL = 'api/v1/news.php?_=' + Date.now();

    function escapeHtml(s) {
        return String(s)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }

    function renderInline(text) {
        // Escape first, then rewrite markers on the escaped text.
        var out = escapeHtml(text);
        // Links: [text](url) -- allow http(s) and site-relative only.
        out = out.replace(/\[([^\]]+)\]\(([^)]+)\)/g, function (_m, label, url) {
            if (!/^(https?:\/\/|\/|#|mailto:)/.test(url)) {
                return label;
            }
            var target = /^https?:\/\//.test(url) ? ' target="_blank" rel="noopener"' : '';
            return '<a href="' + url + '"' + target + '>' + label + '</a>';
        });
        out = out.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
        out = out.replace(/`([^`]+)`/g, '<code>$1</code>');
        return out;
    }

    function renderBody(body) {
        var paragraphs = String(body || '').split(/\n\s*\n/);
        var html = '';
        for (var i = 0; i < paragraphs.length; i++) {
            var p = paragraphs[i].trim();
            if (p === '') continue;
            html += '<p>' + renderInline(p) + '</p>';
        }
        return html;
    }

    function formatDate(iso) {
        var d = new Date(iso);
        if (isNaN(d.getTime())) return escapeHtml(iso);
        var months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
                      'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
        return d.getUTCDate() + ' ' + months[d.getUTCMonth()] + ' ' + d.getUTCFullYear();
    }

    function render(items) {
        var loading = document.getElementById('news-loading');
        var empty = document.getElementById('news-empty');
        var list = document.getElementById('news-list');

        if (loading) loading.classList.add('hidden');

        if (!Array.isArray(items) || items.length === 0) {
            if (empty) empty.classList.remove('hidden');
            return;
        }

        // Sort newest-first
        items.sort(function (a, b) {
            return String(b.date || '').localeCompare(String(a.date || ''));
        });

        var parts = [];
        for (var i = 0; i < items.length; i++) {
            var item = items[i];
            var id = escapeHtml(item.id || ('entry-' + i));
            var title = escapeHtml(item.title || '(untitled)');
            var date = formatDate(item.date || '');
            var body = renderBody(item.body || '');
            var tags = '';
            if (Array.isArray(item.tags) && item.tags.length > 0) {
                var tparts = [];
                for (var t = 0; t < item.tags.length; t++) {
                    tparts.push('<span class="news-tag">' + escapeHtml(item.tags[t]) + '</span>');
                }
                tags = '<div class="news-tags">' + tparts.join(' ') + '</div>';
            }
            parts.push(
                '<article class="news-item" id="' + id + '">' +
                    '<header class="news-item__head">' +
                        '<h3 class="news-item__title">' +
                            '<a href="#' + id + '">' + title + '</a>' +
                        '</h3>' +
                        '<time class="news-item__date" datetime="' + escapeHtml(item.date || '') + '">' + date + '</time>' +
                    '</header>' +
                    '<div class="news-item__body">' + body + '</div>' +
                    tags +
                '</article>'
            );
        }
        list.innerHTML = parts.join('');

        // Scroll to anchor if present
        if (window.location.hash) {
            var target = document.getElementById(window.location.hash.slice(1));
            if (target) target.scrollIntoView();
        }
    }

    function fail(msg) {
        var loading = document.getElementById('news-loading');
        if (loading) {
            loading.textContent = msg;
        }
    }

    var xhr = new XMLHttpRequest();
    xhr.open('GET', NEWS_URL, true);
    xhr.onload = function () {
        if (xhr.status >= 200 && xhr.status < 300) {
            try {
                var data = JSON.parse(xhr.responseText);
                // Accept either an array or { items: [...] }
                var items = Array.isArray(data) ? data : (data && data.items) || [];
                render(items);
            } catch (e) {
                fail('News feed is malformed. Try again later.');
            }
        } else {
            fail('News feed unavailable.');
        }
    };
    xhr.onerror = function () { fail('News feed unavailable.'); };
    xhr.send();
})();
