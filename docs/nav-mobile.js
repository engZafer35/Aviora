(function () {
  function closeNav(topbar, btn) {
    topbar.classList.remove('nav-open');
    btn.setAttribute('aria-expanded', 'false');
    btn.setAttribute('aria-label', 'Open menu');
  }

  function initTopNav() {
    var topbar = document.querySelector('.topbar');
    if (!topbar) {
      return;
    }

    var btn = topbar.querySelector('.nav-toggle');
    var nav = topbar.querySelector('.nav');
    if (!btn || !nav) {
      return;
    }

    nav.id = nav.id || 'site-nav';
    btn.setAttribute('aria-controls', nav.id);

    if (btn.dataset.navBound === '1') {
      return;
    }
    btn.dataset.navBound = '1';

    btn.addEventListener('click', function (event) {
      event.stopPropagation();
      var open = topbar.classList.toggle('nav-open');
      btn.setAttribute('aria-expanded', open ? 'true' : 'false');
      btn.setAttribute('aria-label', open ? 'Close menu' : 'Open menu');
    });

    nav.querySelectorAll('a').forEach(function (link) {
      link.addEventListener('click', function () {
        closeNav(topbar, btn);
      });
    });

    document.addEventListener('click', function (event) {
      if (!topbar.classList.contains('nav-open')) {
        return;
      }
      if (!topbar.contains(event.target)) {
        closeNav(topbar, btn);
      }
    });

    document.addEventListener('keydown', function (event) {
      if (event.key === 'Escape') {
        closeNav(topbar, btn);
      }
    });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initTopNav);
  } else {
    initTopNav();
  }
})();
