(function () {
  function qs(selector) { return document.querySelector(selector); }
  function qsa(selector) { return Array.prototype.slice.call(document.querySelectorAll(selector)); }

  document.addEventListener('DOMContentLoaded', function () {
    var toggle = qs('[data-menu-toggle]');
    var links = qs('[data-nav-links]');
    if (toggle && links) {
      toggle.addEventListener('click', function () {
        links.classList.toggle('open');
      });
    }

    var year = qs('[data-current-year]');
    if (year) {
      year.textContent = new Date().getFullYear();
    }

    renderVisualTests();
    renderBenchmarks();
    renderTestDetail();
  });

  function renderVisualTests() {
    var grid = qs('[data-test-grid]');
    var filters = qsa('[data-test-filter]');
    var tests = window.ARTIC_VISUAL_TESTS || [];
    if (!grid || !tests.length) {
      return;
    }

    function draw(category) {
      grid.innerHTML = '';
      tests.filter(function (item) {
        return category === 'All' || item.category === category;
      }).forEach(function (item) {
        var card = document.createElement('article');
        card.className = 'test-card';
        card.innerHTML = [
          '<div class="chips"><span class="badge ' + (item.maturity === 'Flagship' ? 'good' : 'warn') + '">' + item.maturity + '</span><span class="chip">' + item.category + '</span></div>',
          '<h3>' + item.name + '</h3>',
          '<p>' + item.proof + '</p>',
          '<div class="tag-row">' + item.tags.map(function (tag) { return '<span class="chip">' + tag + '</span>'; }).join('') + '</div>',
          '<div class="button-row"><a class="button ghost" href="test-view.html?id=' + encodeURIComponent(item.id) + '">View Test</a></div>'
        ].join('');
        grid.appendChild(card);
      });
    }

    filters.forEach(function (btn) {
      btn.addEventListener('click', function () {
        filters.forEach(function (chip) { chip.classList.remove('active'); });
        btn.classList.add('active');
        draw(btn.getAttribute('data-test-filter'));
      });
    });

    draw('All');
  }

  function renderBenchmarks() {
    var grid = qs('[data-benchmark-grid]');
    var items = window.ARTIC_BENCHMARKS || [];
    if (!grid || !items.length) {
      return;
    }

    items.forEach(function (item) {
      var card = document.createElement('article');
      card.className = 'metric-card';
      card.innerHTML = [
        '<div class="chips"><span class="chip">' + item.category + '</span><span class="badge ' + (item.delta.indexOf('-') === 0 ? 'good' : 'warn') + '">Delta ' + item.delta + '</span></div>',
        '<h3>' + item.name + '</h3>',
        '<p class="subtle">' + item.config + '</p>',
        '<div class="table-wrap" style="margin-top:12px;">',
        '<table><tbody>',
        '<tr><th>Last baseline</th><td>' + item.lastBaseline + '</td></tr>',
        '<tr><th>Current ref</th><td>' + item.currentReference + '</td></tr>',
        '<tr><th>Environment</th><td>' + item.environment + '</td></tr>',
        '</tbody></table></div>',
        '<p style="margin-top:12px;">' + item.note + '</p>'
      ].join('');
      grid.appendChild(card);
    });
  }

  function renderTestDetail() {
    var detail = qs('[data-test-detail]');
    var tests = window.ARTIC_VISUAL_TESTS || [];
    if (!detail || !tests.length) {
      return;
    }

    var params = new URLSearchParams(window.location.search);
    var id = params.get('id');
    var item = tests.find(function (entry) { return entry.id === id; }) || tests[0];
    if (!item) {
      return;
    }

    detail.innerHTML = [
      '<section class="page-hero"><div class="wrap split">',
      '<div><div class="eyebrow">' + item.category + ' | ' + item.maturity + '</div>',
      '<h1>' + item.name + '</h1>',
      '<p class="hero-sub">' + item.purpose + '</p>',
      '<div class="chips"><span class="badge good">What this proves</span><span class="chip">' + item.proof + '</span></div>',
      '<div class="button-row"><a class="button primary" href="visual-tests.html">Back to visual tests</a><a class="button ghost" href="docs.html">View docs</a></div></div>',
      '<aside class="hero-media"><div class="media-stage">',
      '<div class="scene-tile"><strong>Scene</strong><span>' + item.scene + '</span></div>',
      '<div class="scene-tile"><strong>Determinism</strong><span>' + item.metrics.hash + '</span></div>',
      '<div class="scene-tile"><strong>Runtime</strong><span>' + item.metrics.dt + ' | ' + item.metrics.platform + '</span></div>',
      '<div class="scene-tile"><strong>Benchmark</strong><span>' + item.metrics.benchmark + '</span></div>',
      '</div></aside>',
      '</div></section>',
      '<section class="section"><div class="wrap metric-grid">',
      metricCard('Bodies / particles', item.metrics.bodies),
      metricCard('Constraints', item.metrics.constraints),
      metricCard('Replay hash', item.metrics.hash),
      metricCard('Platform', item.metrics.platform),
      '</div></section>',
      '<section class="section"><div class="wrap card-grid">',
      infoCard('Technical notes', item.notes),
      infoCard('Known limitations', item.limitations),
      infoCard('Recommended context', item.context),
      '</div></section>',
      '<section class="section"><div class="wrap cta-grid">',
      infoCard('Related docs', item.docs.join('<br>')),
      infoCard('Next action', 'Request the evaluation pack or route a technical review using the contact page.'),
      '</div></section>'
    ].join('');
  }

  function metricCard(label, value) {
    return '<article class="metric-card"><div class="value">' + value + '</div><div class="label">' + label + '</div></article>';
  }

  function infoCard(title, body) {
    return '<article class="card"><h3>' + title + '</h3><p>' + body + '</p></article>';
  }
})();
