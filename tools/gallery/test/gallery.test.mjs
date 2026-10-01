import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { generate } from '../generate.mjs';
import { compareVersionsDescending, knownIssueBadges, poolStatus, reportIssueUrl } from '../lib.mjs';

test('the prefill URL encodes spaces, accents and slashes and decodes back to the same values', () => {
  const fields = {
    repo: 'Open-Agriculture/AgIsoVirtualTerminal',
    version: '1.6.0',
    poolId: 'ab/cd ef',
    poolName: 'Kühne & Söhne / Sprüher #2 "Große" 100%',
    objectId: 1000,
    objectType: 'Data Mask/Ärger',
    imageUrl: 'https://example.github.io/AgIsoVirtualTerminal/1.6.0/0cd84f4a/screen_1000.png',
  };
  const url = reportIssueUrl(fields);

  const query = url.slice(url.indexOf('?') + 1);
  assert.doesNotMatch(query, /[ \/"#äöüÄÖÜß]/, 'no raw space, slash, quote, hash or accent in the query');
  assert.match(query, /pool-id=ab%2Fcd%20ef/);
  assert.match(query, /object-type=Data%20Mask%2F%C3%84rger/);

  const parsed = new URL(url);
  assert.equal(parsed.origin + parsed.pathname, 'https://github.com/Open-Agriculture/AgIsoVirtualTerminal/issues/new');
  assert.equal(parsed.searchParams.get('template'), 'render-bug.yml');
  assert.equal(parsed.searchParams.get('pool-id'), 'ab/cd ef');
  assert.equal(parsed.searchParams.get('object-id'), '1000');
  assert.equal(parsed.searchParams.get('object-type'), 'Data Mask/Ärger');
  assert.equal(parsed.searchParams.get('vt-version'), '1.6.0');
  assert.equal(parsed.searchParams.get('image-url'), fields.imageUrl);
  assert.equal(parsed.searchParams.get('title'), '[Render] Kühne & Söhne / Sprüher #2 "Große" 100%: Data Mask/Ärger 1000');
  assert.equal([...parsed.searchParams.keys()].length, 7, 'an & in a value does not start a new parameter');
});

test('the prefill URL stays short: the image is linked, not embedded', () => {
  const url = reportIssueUrl({ repo: 'o/r', version: '1', poolId: '0cd84f4a', poolName: 'x', objectId: 1, objectType: 'DataMask', imageUrl: 'https://e.org/1/0cd84f4a/dm_1.png' });
  assert.ok(url.length < 400);
  assert.doesNotMatch(url, /data%3Aimage|base64/);
});

test('versions sort newest first by number', () => {
  assert.deepEqual(['1.5.1', '1.10.0', '1.9.2', '2.0.0-rc1', '1.5.0', '2.0.0', '2.0.0-rc2'].sort(compareVersionsDescending), ['2.0.0', '2.0.0-rc2', '2.0.0-rc1', '1.10.0', '1.9.2', '1.5.1', '1.5.0']);
});

test('pool status and known issue badges', () => {
  assert.equal(poolStatus({ load: { ok: false } }).id, 'fails');
  assert.equal(poolStatus({ load: { ok: true }, unsupported_objects: [{ status: 'ignored' }] }).id, 'ok');
  assert.equal(poolStatus({ load: { ok: true }, unsupported_objects: [{ status: 'unsupported' }] }).id, 'partial');
  assert.deepEqual(knownIssueBadges(['https://github.com/o/r/issues/166', 'https://example.org/x', 'javascript:alert(1)']), [
    { url: 'https://github.com/o/r/issues/166', label: 'Known issue #166' },
    { url: 'https://example.org/x', label: 'Known issue' },
  ]);
});

function writePool(renders, id, manifest, files) {
  fs.mkdirSync(path.join(renders, id), { recursive: true });
  for (const file of files) {
    fs.writeFileSync(path.join(renders, id, file), 'png');
  }
  fs.writeFileSync(
    path.join(renders, id, 'manifest.json'),
    JSON.stringify({
      pool_id: id,
      load: { ok: true, error: null },
      unsupported_objects: [],
      errors: [],
      known_issues: [],
      images: files.map((file) => ({ file, object_id: Number(/_(\d+)/.exec(file)?.[1] ?? 0), object_type: 'DataMask' })),
      ...manifest,
    }),
  );
}

test('generates a release, keeps earlier ones and publishes only public pools', () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'gallery-test-'));
  const renders = path.join(folder, 'renders');
  const collection = path.join(folder, 'collection');
  const site = path.join(folder, 'site');

  writePool(renders, 'aaaaaaaa', { public: true, pool_name: 'Söhne <b>/ 1', manufacturer: 'Hardi', known_issues: ['https://github.com/o/r/issues/7'] }, ['ws_designator.png', 'screen_1000.png', 'dm_1000.png']);
  writePool(renders, 'bbbbbbbb', { public: false, pool_name: 'secret', manufacturer: 'Horsch' }, ['dm_1.png']);
  fs.mkdirSync(path.join(collection, 'pools', 'hardi', 'x', 'reference'), { recursive: true });
  fs.writeFileSync(path.join(collection, 'pools', 'hardi', 'x', 'meta.yaml'), 'id: "aaaaaaaa"\n');
  fs.writeFileSync(path.join(collection, 'pools', 'hardi', 'x', 'reference', '1000_john-deere-g5.jpg'), 'jpg');

  const options = { renders, collection, site, repo: 'o/r', siteUrl: 'https://o.github.io/r' };
  generate({ ...options, version: '1.5.1' });
  const result = generate({ ...options, version: '1.6.0' });

  assert.deepEqual(result.versions, ['1.6.0', '1.5.1']);
  assert.ok(fs.existsSync(path.join(site, '1.5.1', 'aaaaaaaa', 'index.html')), 'the earlier release is kept');
  assert.ok(!fs.existsSync(path.join(site, '1.6.0', 'bbbbbbbb')), 'a pool that is not public is not published');
  assert.match(fs.readFileSync(path.join(site, 'latest', 'index.html'), 'utf8'), /url=\.\.\/1\.6\.0\/index\.html/);
  assert.match(fs.readFileSync(path.join(site, 'versions.js'), 'utf8'), /"latest":"1\.6\.0"/);

  const poolPage = fs.readFileSync(path.join(site, '1.6.0', 'aaaaaaaa', 'index.html'), 'utf8');
  assert.ok(!poolPage.includes('<b>/ 1'), 'names are escaped');
  assert.match(poolPage, /Known issue #7/);
  assert.match(poolPage, /Looks wrong\?/);
  assert.match(poolPage, /with-reference.*reference\/1000_john-deere-g5\.jpg/s, 'the reference photo sits next to the screen');
  assert.ok(poolPage.indexOf('screen_1000.png') < poolPage.indexOf('dm_1000.png'), 'screens come first');
  assert.ok(fs.existsSync(path.join(site, '1.6.0', 'aaaaaaaa', 'reference', '1000_john-deere-g5.jpg')));
  assert.ok(fs.existsSync(path.join(site, 'compare', 'aaaaaaaa', 'index.html')));

  const pages = [path.join(site, '1.6.0', 'index.html'), path.join(site, '1.6.0', 'aaaaaaaa', 'index.html'), path.join(site, 'compare', 'aaaaaaaa', 'index.html')];
  for (const pageFile of pages) {
    const html = fs.readFileSync(pageFile, 'utf8');
    const external = [...html.matchAll(/(?:src|href)="(https?:[^"]*)"/g)].map((match) => match[1]).filter((url) => !url.startsWith('https://github.com/') && !url.startsWith('https://o.github.io/'));
    assert.deepEqual(external, [], `${pageFile} loads nothing from elsewhere`);
    assert.doesNotMatch(html, /(?:src|href)="\//, `${pageFile} has no root-relative links, which break from disk`);
  }
});
