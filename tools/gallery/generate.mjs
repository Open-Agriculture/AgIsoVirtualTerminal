#!/usr/bin/env node
// Adds one release to the static render gallery, from vt-render's manifests.
//
//   node generate.mjs --renders <vt-render --all output> --collection <collection checkout> --site <site folder>
//                     --version <version> --repo <owner/name> --site-url <published URL of the site folder>
//
// Writes <site>/<version>/ (replacing it if it exists) and refreshes the files every release shares:
// versions.js, assets/, latest/, compare/ and the root index.html. Other releases are left as they are.
// Only pools whose meta.yaml says public: true are published.
//
// Every page works from disk: links are relative and point at index.html files, and data is loaded
// with <script> tags, not fetch().

import fs from 'node:fs';
import path from 'node:path';
import { parseArgs } from 'node:util';
import { fileURLToPath } from 'node:url';
import { compareVersionsDescending, encodePath, esc, knownIssueBadges, poolStatus, reportIssueUrl } from './lib.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));

const SECTIONS = [
  { title: 'Screens', prefix: 'screen_', note: 'Each data mask with the soft key mask it references, as the VT shows them.' },
  { title: 'Data masks', prefix: 'dm_' },
  { title: 'Alarm masks', prefix: 'am_' },
  { title: 'Soft key masks', prefix: 'skm_' },
  { title: 'Working set designator', prefix: 'ws_designator' },
];

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

function write(file, text) {
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, text);
}

/** meta.yaml says TODO for what nobody has found out yet */
function known(value, fallback) {
  return value && value !== 'TODO' ? value : fallback;
}

function objectNumber(file) {
  return Number(/_(\d+)\.png$/.exec(file)?.[1] ?? 0);
}

/** Pool id -> reference image files (relative to the pool folder) of the collection. */
function findReferenceImages(collection) {
  const result = new Map();
  const root = path.join(collection, 'pools');
  if (!fs.existsSync(root)) {
    return result;
  }
  for (const manufacturer of fs.readdirSync(root, { withFileTypes: true }).filter((entry) => entry.isDirectory())) {
    for (const slug of fs.readdirSync(path.join(root, manufacturer.name), { withFileTypes: true }).filter((entry) => entry.isDirectory())) {
      const folder = path.join(root, manufacturer.name, slug.name);
      const metaFile = path.join(folder, 'meta.yaml');
      const referenceFolder = path.join(folder, 'reference');
      const id = fs.existsSync(metaFile) ? /^id:\s*["']?([0-9a-f]{8})["']?\s*$/m.exec(fs.readFileSync(metaFile, 'utf8'))?.[1] : null;
      if (id && fs.existsSync(referenceFolder)) {
        const files = fs
          .readdirSync(referenceFolder)
          .filter((name) => /^\d+_[a-z0-9-]+\.(jpe?g|png)$/i.test(name))
          .sort()
          .map((name) => ({ source: path.join(referenceFolder, name), name, objectId: Number(name.split('_')[0]), terminal: name.replace(/^\d+_|\.[^.]+$/g, '') }));
        result.set(id, files);
      }
    }
  }
  return result;
}

function page({ root, title, body, version = '', pool = '', kind }) {
  return `<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${esc(title)}</title>
<link rel="stylesheet" href="${root}assets/gallery.css">
</head>
<body data-root="${esc(root)}" data-version="${esc(version)}" data-pool="${esc(pool)}" data-page="${esc(kind)}">
<header class="top">
  <a class="home" href="${root}latest/index.html">AgIsoVirtualTerminal render gallery</a>
  <label class="version">Version <select id="version-select" aria-label="Version"></select></label>
</header>
<main>
${body}
</main>
<script src="${root}versions.js"></script>
<script src="${root}assets/gallery.js"></script>
</body>
</html>
`;
}

function badge(status) {
  return `<span class="badge badge-${esc(status.id)}">${esc(status.label)}</span>`;
}

function knownIssues(manifest) {
  return knownIssueBadges(manifest.known_issues)
    .map((issue) => `<a class="badge badge-issue" href="${esc(issue.url)}">${esc(issue.label)}</a>`)
    .join(' ');
}

function figure({ src, caption, reportUrl, className = '' }) {
  const report = reportUrl ? `<a class="report" href="${esc(reportUrl)}">Looks wrong?</a>` : '';
  return `<figure class="${esc(className)}"><a href="${esc(src)}"><img src="${esc(src)}" alt="${esc(caption)}" loading="lazy"></a><figcaption>${esc(caption)} ${report}</figcaption></figure>`;
}

function renderIndexPage({ version, pools }) {
  const manufacturers = [...new Set(pools.map(({ manifest }) => known(manifest.manufacturer, 'Unknown')))].sort();
  const cards = pools
    .map(({ id, manifest }) => {
      const manufacturer = known(manifest.manufacturer, 'Unknown');
      const hasIcon = (manifest.images ?? []).some((image) => image.file === 'ws_designator.png');
      const icon = hasIcon ? `<img src="${encodePath(`${id}/ws_designator.png`)}" alt="" width="96" height="96">` : '<span class="no-icon">?</span>';
      return `<a class="card" href="${encodePath(id)}/index.html" data-manufacturer="${esc(manufacturer)}">
  ${icon}
  <span class="card-text"><strong>${esc(known(manifest.pool_name, id))}</strong>
  <span>${esc(manufacturer)}</span>
  <span>${badge(poolStatus(manifest))}</span></span>
</a>`;
    })
    .join('\n');
  const options = manufacturers.map((name) => `<option value="${esc(name)}">${esc(name)}</option>`).join('');
  const body = `<h1>Release ${esc(version)}</h1>
<p>${pools.length} object pool${pools.length === 1 ? '' : 's'}, drawn by this release's VT code with vt-render.</p>
<label>Manufacturer <select id="manufacturer-filter"><option value="">All</option>${options}</select></label>
<div class="cards">
${cards || '<p>No public pools in this release.</p>'}
</div>`;
  return page({ root: '../', title: `Render gallery ${version}`, body, version, kind: 'index' });
}

function renderPoolPage({ version, id, manifest, references, repo, siteUrl }) {
  const name = known(manifest.pool_name, id);
  const images = [...(manifest.images ?? [])];
  const reportUrl = (image) =>
    reportIssueUrl({
      repo,
      version,
      poolId: id,
      poolName: name,
      objectId: image.object_id,
      objectType: image.object_type,
      imageUrl: `${siteUrl.replace(/\/$/, '')}/${encodePath(`${version}/${id}/${image.file}`)}`,
    });

  // A reference photo goes next to the first render of the object it shows: the screen, or else the mask
  const placedReferences = new Set();
  const sections = SECTIONS.map((section) => {
    const sectionImages = images.filter((image) => image.file.startsWith(section.prefix)).sort((a, b) => objectNumber(a.file) - objectNumber(b.file) || a.file.localeCompare(b.file));
    if (sectionImages.length === 0) {
      return '';
    }
    const figures = sectionImages
      .map((image) => {
        const refs = references.filter((reference) => reference.objectId === image.object_id && !placedReferences.has(reference.name) && section.prefix !== 'ws_designator');
        refs.forEach((reference) => placedReferences.add(reference.name));
        const ours = figure({ src: encodePath(image.file), caption: `${image.object_type} ${image.object_id} (${image.file})`, reportUrl: reportUrl(image) });
        if (refs.length === 0) {
          return ours;
        }
        const theirs = refs.map((reference) => figure({ src: encodePath(`reference/${reference.name}`), caption: `On a ${reference.terminal} terminal`, className: 'reference' })).join('');
        return `<div class="with-reference">${ours}${theirs}</div>`;
      })
      .join('\n');
    return `<section><h2>${esc(section.title)}</h2>${section.note ? `<p>${esc(section.note)}</p>` : ''}<div class="figures">${figures}</div></section>`;
  }).join('\n');

  const otherReferences = references.filter((reference) => !placedReferences.has(reference.name));
  const otherReferenceSection = otherReferences.length
    ? `<section><h2>Other reference images</h2><div class="figures">${otherReferences
        .map((reference) => figure({ src: encodePath(`reference/${reference.name}`), caption: `Object ${reference.objectId} on a ${reference.terminal} terminal`, className: 'reference' }))
        .join('')}</div></section>`
    : '';

  const warnings = [];
  if (!manifest.load?.ok) {
    warnings.push(`<li><strong>The pool does not load:</strong> ${esc(manifest.load?.error)}</li>`);
  }
  for (const error of manifest.errors ?? []) {
    warnings.push(`<li>${esc(error.file)} (${esc(error.object_type)} ${esc(error.object_id)}): ${esc(error.error)}</li>`);
  }
  const objects = manifest.unsupported_objects ?? [];
  const objectRows = objects
    .map((object) => `<tr><td>${esc(object.object_id)}</td><td>${esc(object.object_type)}</td><td>${esc(object.status)}</td><td>${esc(object.reason)}</td></tr>`)
    .join('');

  const body = `<p class="crumbs"><a href="../index.html">Release ${esc(version)}</a> / ${esc(name)}</p>
<h1>${esc(name)}</h1>
<p>${esc(known(manifest.manufacturer, 'Unknown manufacturer'))} · pool ${esc(id)} · ${badge(poolStatus(manifest))} ${knownIssues(manifest)}</p>
<p><a href="../../compare/${encodePath(id)}/index.html">Compare with other releases</a></p>
${warnings.length ? `<section class="warnings"><h2>Warnings</h2><ul>${warnings.join('')}</ul></section>` : ''}
${sections}
${otherReferenceSection}
<section><h2>Unsupported and ignored objects</h2>
${objects.length ? `<p>Objects the VT does not draw, or that no image shows.</p><table><thead><tr><th>Object</th><th>Type</th><th>Status</th><th>Reason</th></tr></thead><tbody>${objectRows}</tbody></table>` : '<p>None.</p>'}
</section>`;
  return page({ root: '../../', title: `${name} (${version})`, body, version, pool: id, kind: 'pool' });
}

function renderComparePage({ id, name }) {
  const body = `<p class="crumbs"><a href="../../latest/index.html">Latest release</a> / compare</p>
<h1>${esc(name)}: compare releases</h1>
<div class="compare-controls">
  <label>Left <select id="compare-a"></select></label>
  <label>Right <select id="compare-b"></select></label>
  <label>Image <select id="compare-image"></select></label>
</div>
<p id="compare-message"></p>
<div class="figures" id="compare-side-by-side"></div>
<h2>Overlay</h2>
<p>Drag the slider: the left release is on the left of the line, the right release on its right.</p>
<div class="overlay" id="compare-overlay"></div>
<input type="range" id="compare-slider" min="0" max="100" value="50" aria-label="Overlay position">`;
  return page({ root: '../../', title: `${name}: compare releases`, body, pool: id, kind: 'compare' });
}

function redirect(target) {
  return `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta http-equiv="refresh" content="0; url=${esc(target)}"><title>Render gallery</title></head>
<body><p><a href="${esc(target)}">Go to the latest release</a></p></body></html>
`;
}

export function generate({ renders, collection, site, version, repo, siteUrl }) {
  if (!/^[A-Za-z0-9._-]+$/.test(version)) {
    throw new Error(`Unusable version name: ${version}`);
  }

  const pools = fs
    .readdirSync(renders, { withFileTypes: true })
    .filter((entry) => entry.isDirectory() && fs.existsSync(path.join(renders, entry.name, 'manifest.json')))
    .map((entry) => ({ id: entry.name, manifest: readJson(path.join(renders, entry.name, 'manifest.json')) }))
    .filter(({ id, manifest }) => manifest.public === true && /^[0-9a-f]{8}$/.test(id))
    .sort((a, b) => known(a.manifest.manufacturer, '~').localeCompare(known(b.manifest.manufacturer, '~')) || a.id.localeCompare(b.id));
  const references = findReferenceImages(collection);

  const versionFolder = path.join(site, version);
  fs.rmSync(versionFolder, { recursive: true, force: true });
  const data = { pools: {} };

  for (const { id, manifest } of pools) {
    const poolFolder = path.join(versionFolder, id);
    fs.mkdirSync(poolFolder, { recursive: true });
    for (const image of manifest.images ?? []) {
      if (/^[A-Za-z0-9_.-]+\.png$/.test(image.file)) {
        fs.copyFileSync(path.join(renders, id, image.file), path.join(poolFolder, image.file));
      }
    }
    const poolReferences = references.get(id) ?? [];
    for (const reference of poolReferences) {
      fs.mkdirSync(path.join(poolFolder, 'reference'), { recursive: true });
      fs.copyFileSync(reference.source, path.join(poolFolder, 'reference', reference.name));
    }
    write(path.join(poolFolder, 'index.html'), renderPoolPage({ version, id, manifest, references: poolReferences, repo, siteUrl }));
    data.pools[id] = {
      name: known(manifest.pool_name, id),
      manufacturer: known(manifest.manufacturer, ''),
      images: (manifest.images ?? []).map(({ file, object_id, object_type, width, height }) => ({ file, object_id, object_type, width, height })),
    };
  }
  write(path.join(versionFolder, 'index.html'), renderIndexPage({ version, pools }));
  write(path.join(versionFolder, 'data.json'), JSON.stringify(data, null, 1) + '\n');
  write(path.join(versionFolder, 'data.js'), `window.VT_GALLERY_DATA = window.VT_GALLERY_DATA || {};\nwindow.VT_GALLERY_DATA[${JSON.stringify(version)}] = ${JSON.stringify(data)};\n`);

  // What every release shares: the list of releases, the assets, latest/ and the comparison pages
  const versionsFile = path.join(site, 'versions.json');
  const previousVersions = fs.existsSync(versionsFile) ? readJson(versionsFile) : [];
  const versions = [
    ...previousVersions.filter((entry) => entry.version !== version),
    { version, pools: pools.map(({ id }) => ({ id, name: data.pools[id].name })) },
  ].sort((a, b) => compareVersionsDescending(a.version, b.version));
  const latest = versions[0].version;
  write(versionsFile, JSON.stringify(versions, null, 1) + '\n');
  write(path.join(site, 'versions.js'), `window.VT_GALLERY = ${JSON.stringify({ latest, versions: versions.map((entry) => ({ version: entry.version, pools: entry.pools.map((pool) => pool.id) })) })};\n`);
  for (const asset of ['gallery.css', 'gallery.js']) {
    write(path.join(site, 'assets', asset), fs.readFileSync(path.join(here, 'assets', asset), 'utf8'));
  }
  write(path.join(site, 'latest', 'index.html'), redirect(`../${encodePath(latest)}/index.html`));
  write(path.join(site, 'index.html'), redirect('latest/index.html'));
  write(path.join(site, '.nojekyll'), '');

  const comparePools = new Map();
  for (const entry of versions) {
    for (const pool of entry.pools) {
      if (!comparePools.has(pool.id)) {
        comparePools.set(pool.id, pool.name);
      }
    }
  }
  fs.rmSync(path.join(site, 'compare'), { recursive: true, force: true });
  for (const [id, name] of comparePools) {
    write(path.join(site, 'compare', id, 'index.html'), renderComparePage({ id, name }));
  }
  return { pools: pools.length, latest, versions: versions.map((entry) => entry.version) };
}

function main() {
  const { values } = parseArgs({
    options: {
      renders: { type: 'string' },
      collection: { type: 'string' },
      site: { type: 'string' },
      version: { type: 'string' },
      repo: { type: 'string' },
      'site-url': { type: 'string' },
    },
  });
  for (const name of ['renders', 'collection', 'site', 'version', 'repo', 'site-url']) {
    if (!values[name]) {
      console.error('Usage: generate.mjs --renders <folder> --collection <folder> --site <folder> --version <version> --repo <owner/name> --site-url <url>');
      return 2;
    }
  }
  const result = generate({
    renders: values.renders,
    collection: values.collection,
    site: values.site,
    version: values.version,
    repo: values.repo,
    siteUrl: values['site-url'],
  });
  console.log(`Release ${values.version}: ${result.pools} public pools. Releases: ${result.versions.join(', ')}; latest ${result.latest}.`);
  return 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  process.exitCode = main();
}
