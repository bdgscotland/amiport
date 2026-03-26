/**
 * activity.js -- amiport activity feed renderer
 *
 * Fetches /api/v1/activity.php and renders recent events
 * into #activity-feed using the activity-feed CSS component.
 */
(function () {
    'use strict';

    var API_URL = 'api/v1/activity.php?_=' + Date.now();

    var BULLETS = {
        'port_published': '\u25B8',
        'port_updated': '\u25B8',
        'request_submitted': '?',
        'request_status': '\u25C8',
        'milestone': '\u2605'
    };

    function relativeTime(isoString) {
        var then = new Date(isoString);
        var now = new Date();
        var diffMs = now.getTime() - then.getTime();
        if (diffMs < 0) return 'just now';
        var diffSec = Math.floor(diffMs / 1000);
        if (diffSec < 60) return 'just now';
        var diffMin = Math.floor(diffSec / 60);
        if (diffMin < 60) return diffMin + (diffMin === 1 ? ' min ago' : ' mins ago');
        var diffHr = Math.floor(diffMin / 60);
        if (diffHr < 24) return diffHr + (diffHr === 1 ? ' hour ago' : ' hours ago');
        var diffDay = Math.floor(diffHr / 24);
        if (diffDay < 7) return diffDay + (diffDay === 1 ? ' day ago' : ' days ago');
        var diffWeek = Math.floor(diffDay / 7);
        if (diffWeek < 5) return diffWeek + (diffWeek === 1 ? ' week ago' : ' weeks ago');
        var diffMonth = Math.floor(diffDay / 30);
        return diffMonth + (diffMonth === 1 ? ' month ago' : ' months ago');
    }

    function renderFeed(container, items) {
        while (container.firstChild) {
            container.removeChild(container.firstChild);
        }

        if (!items || items.length === 0) {
            var emptyP = document.createElement('p');
            emptyP.className = 'text-muted';
            emptyP.textContent = 'No recent activity yet. Check back after the next port ships!';
            container.appendChild(emptyP);
            return;
        }

        var ul = document.createElement('ul');
        ul.className = 'activity-feed';

        var max = Math.min(items.length, 10);
        for (var i = 0; i < max; i++) {
            var item = items[i];

            var li = document.createElement('li');
            li.className = 'activity-feed__item';

            // Bullet indicator
            var bullet = document.createElement('span');
            bullet.className = 'activity-feed__bullet';
            bullet.textContent = BULLETS[item.type] || '\u25B8';
            li.appendChild(bullet);

            // Title (link if URL present)
            var titleWrap = document.createElement('span');
            titleWrap.className = 'activity-feed__title';
            if (item.url) {
                var a = document.createElement('a');
                a.href = item.url;
                a.textContent = item.title || '';
                titleWrap.appendChild(a);
            } else {
                titleWrap.textContent = item.title || '';
            }
            li.appendChild(titleWrap);

            // Relative timestamp
            var timeEl = document.createElement('time');
            timeEl.className = 'activity-feed__time';
            timeEl.textContent = relativeTime(item.timestamp);
            if (item.timestamp) {
                timeEl.setAttribute('datetime', item.timestamp);
            }
            li.appendChild(timeEl);

            ul.appendChild(li);
        }

        container.appendChild(ul);
    }

    function renderError(container) {
        while (container.firstChild) {
            container.removeChild(container.firstChild);
        }
        var p = document.createElement('p');
        p.className = 'text-muted';
        p.textContent = 'Could not load activity.';
        container.appendChild(p);
    }

    function init() {
        var container = document.getElementById('activity-feed');
        if (!container) return;

        var xhr = new XMLHttpRequest();
        xhr.open('GET', API_URL, true);
        xhr.onload = function () {
            if (xhr.status >= 200 && xhr.status < 300) {
                try {
                    var data = JSON.parse(xhr.responseText);
                    if (Array.isArray(data)) {
                        renderFeed(container, data);
                    } else {
                        renderError(container);
                    }
                } catch (e) {
                    renderError(container);
                }
            } else {
                renderError(container);
            }
        };
        xhr.onerror = function () {
            renderError(container);
        };
        xhr.send();
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
