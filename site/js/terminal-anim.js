/**
 * terminal-anim.js — Typing animation for the hero terminal
 *
 * Types each command character-by-character with 30ms delay.
 * Output lines appear instantly. Cursor blinks after last line.
 * Respects prefers-reduced-motion: shows all text immediately.
 * No dependencies. ~50 lines.
 */
(function() {
    'use strict';

    var container = document.querySelector('.terminal-anim');
    if (!container) return;

    var lines = [
        { type: 'cmd',    prompt: '1.SYS:> ', text: 'amiget install grep' },
        { type: 'output', text: 'Downloading grep-1.68.lha... 45KB/45KB' },
        { type: 'output', text: 'Installed grep 1.68 to C:' },
        { type: 'cmd',    prompt: '1.SYS:> ', text: 'grep -i "hello" README' },
        { type: 'output', text: 'Hello, Amiga world!' },
        { type: 'cursor', prompt: '1.SYS:> ' }
    ];

    var reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

    function createLine(line) {
        var div = document.createElement('div');
        if (line.type === 'cmd' || line.type === 'cursor') {
            var prompt = document.createElement('span');
            prompt.className = 'shell-prompt';
            prompt.textContent = line.prompt;
            div.appendChild(prompt);
            if (line.type === 'cursor') {
                var cursor = document.createElement('span');
                cursor.className = 'shell-cursor';
                div.appendChild(cursor);
            } else {
                var cmd = document.createElement('span');
                cmd.className = 'shell-cmd';
                div.appendChild(cmd);
            }
        } else {
            div.className = 'shell-output';
        }
        return div;
    }

    // Show all instantly (reduced motion or noscript fallback)
    function showAll() {
        for (var i = 0; i < lines.length; i++) {
            var line = lines[i];
            var div = createLine(line);
            if (line.type === 'cmd') {
                div.querySelector('.shell-cmd').textContent = line.text;
            } else if (line.type === 'output') {
                div.textContent = line.text;
            }
            container.appendChild(div);
        }
    }

    // Animate typing
    function animate() {
        var lineIdx = 0;

        function nextLine() {
            if (lineIdx >= lines.length) return;
            var line = lines[lineIdx];
            var div = createLine(line);
            container.appendChild(div);

            if (line.type === 'output') {
                div.textContent = line.text;
                lineIdx++;
                setTimeout(nextLine, 100);
            } else if (line.type === 'cmd') {
                var cmdEl = div.querySelector('.shell-cmd');
                var charIdx = 0;
                function typeChar() {
                    if (charIdx < line.text.length) {
                        cmdEl.textContent += line.text[charIdx];
                        charIdx++;
                        setTimeout(typeChar, 30);
                    } else {
                        lineIdx++;
                        setTimeout(nextLine, 500);
                    }
                }
                typeChar();
            } else {
                // cursor line — done
                lineIdx++;
            }
        }

        nextLine();
    }

    if (reducedMotion) {
        showAll();
    } else {
        animate();
    }
})();
