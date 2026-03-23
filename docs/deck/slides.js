/* The Hard Direction — Minimal slide engine */
(function() {
  var current = 0;
  var slides = [];

  function init() {
    slides = Array.from(document.querySelectorAll('.slide'));
    var hash = parseInt(location.hash.replace('#', ''), 10);
    if (hash >= 1 && hash <= slides.length) current = hash - 1;
    show(current);
  }

  function show(idx) {
    slides.forEach(function(s, i) {
      if (i === idx) {
        s.style.display = 'flex';
        requestAnimationFrame(function() { s.classList.add('active'); });
      } else {
        s.classList.remove('active');
        setTimeout(function() { if (i !== current) s.style.display = 'none'; }, 200);
      }
    });
    location.hash = '#' + (idx + 1);
    var counters = document.querySelectorAll('.counter');
    counters.forEach(function(el) {
      el.textContent = (idx + 1) + ' / ' + slides.length;
    });
  }

  function next() { if (current < slides.length - 1) { current++; show(current); } }
  function prev() { if (current > 0) { current--; show(current); } }

  document.addEventListener('keydown', function(e) {
    if (e.key === 'ArrowRight' || e.key === ' ' || e.key === 'PageDown') { e.preventDefault(); next(); }
    if (e.key === 'ArrowLeft' || e.key === 'PageUp') { e.preventDefault(); prev(); }
    if (e.key === 'Home') { e.preventDefault(); current = 0; show(current); }
    if (e.key === 'End') { e.preventDefault(); current = slides.length - 1; show(current); }
    if (e.key === 'f' || e.key === 'F') {
      if (!document.fullscreenElement) document.documentElement.requestFullscreen();
      else document.exitFullscreen();
    }
  });

  document.addEventListener('click', function(e) {
    if (e.clientX > window.innerWidth / 2) next(); else prev();
  });

  window.addEventListener('hashchange', function() {
    var hash = parseInt(location.hash.replace('#', ''), 10);
    if (hash >= 1 && hash <= slides.length && hash - 1 !== current) {
      current = hash - 1;
      show(current);
    }
  });

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
  else init();
})();
