// Version selector, manufacturer filter and release comparison for the render gallery. Runs from disk
// as well as from a server: data comes from <script> files (versions.js, <version>/data.js), not fetch().
(function () {
  'use strict';

  var body = document.body;
  var root = body.getAttribute('data-root') || '';
  var currentVersion = body.getAttribute('data-version');
  var pool = body.getAttribute('data-pool');
  var gallery = window.VT_GALLERY || { latest: '', versions: [] };

  function encodeSegments(path) {
    return path.split('/').map(encodeURIComponent).join('/');
  }

  function imageUrl(version, file) {
    return root + encodeSegments(version + '/' + pool + '/' + file);
  }

  function option(select, value, text) {
    var element = document.createElement('option');
    element.value = value;
    element.textContent = text;
    select.appendChild(element);
    return element;
  }

  // Version selector: stays on the same pool when the other release has it
  var versionSelect = document.getElementById('version-select');
  if (versionSelect) {
    if (!currentVersion) {
      option(versionSelect, '', 'Choose a release');
    }
    gallery.versions.forEach(function (entry) {
      option(versionSelect, entry.version, entry.version + (entry.version === gallery.latest ? ' (latest)' : ''));
    });
    versionSelect.value = currentVersion || '';
    versionSelect.addEventListener('change', function () {
      var chosen = gallery.versions.filter(function (entry) { return entry.version === versionSelect.value; })[0];
      if (!chosen) {
        return;
      }
      var hasPool = pool && chosen.pools.indexOf(pool) >= 0;
      window.location.href = root + encodeURIComponent(chosen.version) + '/' + (hasPool ? encodeURIComponent(pool) + '/' : '') + 'index.html';
    });
  }

  // Manufacturer filter on a release's index page
  var filter = document.getElementById('manufacturer-filter');
  if (filter) {
    filter.addEventListener('change', function () {
      var cards = document.querySelectorAll('.card');
      for (var i = 0; i < cards.length; i++) {
        cards[i].hidden = filter.value !== '' && cards[i].getAttribute('data-manufacturer') !== filter.value;
      }
    });
  }

  if (body.getAttribute('data-page') === 'compare') {
    setupCompare();
  }

  function loadScript(src) {
    return new Promise(function (resolve) {
      var script = document.createElement('script');
      script.src = src;
      script.onload = resolve;
      script.onerror = resolve; // a missing release just has no data
      document.head.appendChild(script);
    });
  }

  function setupCompare() {
    var message = document.getElementById('compare-message');
    var selectA = document.getElementById('compare-a');
    var selectB = document.getElementById('compare-b');
    var selectImage = document.getElementById('compare-image');
    var sideBySide = document.getElementById('compare-side-by-side');
    var overlay = document.getElementById('compare-overlay');
    var slider = document.getElementById('compare-slider');

    var versions = gallery.versions
      .filter(function (entry) { return entry.pools.indexOf(pool) >= 0; })
      .map(function (entry) { return entry.version; });

    Promise.all(versions.map(function (version) { return loadScript(root + encodeURIComponent(version) + '/data.js'); })).then(function () {
      var data = window.VT_GALLERY_DATA || {};
      var available = versions.filter(function (version) { return data[version] && data[version].pools[pool]; });
      if (available.length === 0) {
        message.textContent = 'No release has this pool.';
        return;
      }
      if (available.length === 1) {
        message.textContent = 'Only release ' + available[0] + ' has this pool so far.';
      }
      available.forEach(function (version) {
        option(selectA, version, version);
        option(selectB, version, version);
      });
      selectA.value = available[Math.min(1, available.length - 1)];
      selectB.value = available[0];

      function files(version) {
        return data[version].pools[pool].images.map(function (image) { return image.file; });
      }

      function rank(file) {
        var order = ['screen_', 'dm_', 'am_', 'skm_'];
        for (var i = 0; i < order.length; i++) {
          if (file.indexOf(order[i]) === 0) {
            return i;
          }
        }
        return order.length;
      }

      function fillImages() {
        var previous = selectImage.value;
        var all = files(selectA.value).concat(files(selectB.value)).filter(function (file, index, list) { return list.indexOf(file) === index; });
        all.sort(function (a, b) { return rank(a) - rank(b) || a.localeCompare(b, undefined, { numeric: true }); });
        selectImage.innerHTML = '';
        all.forEach(function (file) { option(selectImage, file, file); });
        if (all.indexOf(previous) >= 0) {
          selectImage.value = previous;
        }
      }

      function figure(version) {
        var element = document.createElement('figure');
        var caption = document.createElement('figcaption');
        caption.textContent = version;
        if (files(version).indexOf(selectImage.value) >= 0) {
          var image = document.createElement('img');
          image.src = imageUrl(version, selectImage.value);
          image.alt = selectImage.value + ' in ' + version;
          element.appendChild(image);
        } else {
          caption.textContent = version + ': not in this release';
        }
        element.appendChild(caption);
        return element;
      }

      function show() {
        sideBySide.innerHTML = '';
        sideBySide.appendChild(figure(selectA.value));
        sideBySide.appendChild(figure(selectB.value));

        overlay.innerHTML = '';
        var inA = files(selectA.value).indexOf(selectImage.value) >= 0;
        var inB = files(selectB.value).indexOf(selectImage.value) >= 0;
        if (!inA || !inB) {
          overlay.textContent = 'The overlay needs the image in both releases.';
          return;
        }
        var under = document.createElement('img');
        under.src = imageUrl(selectA.value, selectImage.value);
        under.alt = selectImage.value + ' in ' + selectA.value;
        var over = document.createElement('img');
        over.src = imageUrl(selectB.value, selectImage.value);
        over.alt = selectImage.value + ' in ' + selectB.value;
        over.className = 'over';
        var line = document.createElement('div');
        line.className = 'line';
        overlay.appendChild(under);
        overlay.appendChild(over);
        overlay.appendChild(line);
        position();
      }

      function position() {
        var over = overlay.querySelector('.over');
        var line = overlay.querySelector('.line');
        if (over) {
          over.style.clipPath = 'inset(0 0 0 ' + slider.value + '%)';
          line.style.left = slider.value + '%';
        }
      }

      selectA.addEventListener('change', function () { fillImages(); show(); });
      selectB.addEventListener('change', function () { fillImages(); show(); });
      selectImage.addEventListener('change', show);
      slider.addEventListener('input', position);
      fillImages();
      show();
    });
  }
})();
