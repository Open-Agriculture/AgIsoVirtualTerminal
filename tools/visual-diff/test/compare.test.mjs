import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { PNG } from 'pngjs';
import { buildComment, MARKER } from '../comment.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const compareScript = path.join(here, '..', 'compare.mjs');

/** A solid image with an optional block of another colour. */
function png(width, height, colour, block = null) {
  const image = new PNG({ width, height });
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const inBlock = block && x >= block.x && x < block.x + block.size && y >= block.y && y < block.y + block.size;
      image.data.set(inBlock ? block.colour : colour, 4 * (y * width + x));
    }
  }
  return PNG.sync.write(image);
}

/** Writes a vt-render --all style folder: <folder>/<pool>/manifest.json and the images. */
function writeRenders(folder, pools, info = { available: true, fingerprint: 'same', vt_commit: 'abc', pools_commit: 'def' }) {
  fs.mkdirSync(folder, { recursive: true });
  fs.writeFileSync(path.join(folder, 'render-info.json'), JSON.stringify(info));
  for (const [id, pool] of Object.entries(pools)) {
    const poolFolder = path.join(folder, id);
    fs.mkdirSync(poolFolder, { recursive: true });
    const images = Object.entries(pool.images ?? {}).map(([file, image]) => {
      fs.writeFileSync(path.join(poolFolder, file), image.data);
      return { file, object_id: 1000, object_type: 'DataMask', sha256: image.sha };
    });
    fs.writeFileSync(
      path.join(poolFolder, 'manifest.json'),
      JSON.stringify({
        pool_id: id,
        pool_name: pool.name ?? id,
        public: pool.public ?? false,
        render_settings: { platform: 'linux' },
        load: { ok: pool.ok ?? true, error: pool.ok === false ? 'parsing failed at object 7' : null },
        images: pool.ok === false ? [] : images,
      }),
    );
  }
}

function tempFolder() {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'visual-diff-test-'));
}

function run(base, head, out, ...extra) {
  const result = spawnSync(process.execPath, [compareScript, '--base', base, '--head', head, '--out', out, ...extra], { encoding: 'utf8' });
  const reportFile = path.join(out, 'report.json');
  return { code: result.status, stderr: result.stderr, report: fs.existsSync(reportFile) ? JSON.parse(fs.readFileSync(reportFile, 'utf8')) : null };
}

const white = [255, 255, 255, 255];
const red = [255, 0, 0, 255];

function scenario(headOverrides = {}) {
  const folder = tempFolder();
  const base = {
    aaaaaaaa: {
      public: true,
      images: {
        'dm_1.png': { data: png(20, 20, white), sha: 'same' },
        'dm_2.png': { data: png(20, 20, white), sha: 'old' },
        'dm_3.png': { data: png(20, 20, white), sha: 'gone' },
        'dm_4.png': { data: png(20, 20, white), sha: 'resized' },
      },
    },
    bbbbbbbb: { images: { 'dm_1.png': { data: png(4, 4, white), sha: 'x' } } },
    cccccccc: { ok: false },
  };
  const head = {
    aaaaaaaa: {
      public: true,
      images: {
        'dm_1.png': { data: png(20, 20, white), sha: 'same' },
        'dm_2.png': { data: png(20, 20, white, { x: 5, y: 5, size: 3, colour: red }), sha: 'new-pixels' },
        'dm_4.png': { data: png(24, 20, white), sha: 'resized-now' },
        'dm_5.png': { data: png(20, 20, white), sha: 'brand-new' },
      },
    },
    bbbbbbbb: headOverrides.bbbbbbbb ?? { images: { 'dm_1.png': { data: png(4, 4, white), sha: 'x' } } },
    cccccccc: { ok: false },
  };
  writeRenders(path.join(folder, 'base'), base);
  writeRenders(path.join(folder, 'head'), head);
  return folder;
}

test('classifies unchanged, changed, new and removed images and writes the diff files', () => {
  const folder = scenario();
  const { code, report } = run(path.join(folder, 'base'), path.join(folder, 'head'), path.join(folder, 'out'));

  assert.equal(code, 1);
  assert.deepEqual(report.counts, { unchanged: 2, changed: 2, new: 1, removed: 1, 'newly-failing': 0 });
  const byFile = Object.fromEntries(report.images.map((image) => [image.file, image]));
  assert.equal(byFile['dm_2.png'].class, 'changed');
  assert.equal(byFile['dm_2.png'].diff_pixels, 9);
  assert.deepEqual(byFile['dm_4.png'].size_changed, { before: [20, 20], after: [24, 20] });
  assert.equal(byFile['dm_3.png'].class, 'removed');
  assert.equal(byFile['dm_5.png'].class, 'new');
  assert.equal(report.still_failing.length, 1);

  const sideBySide = PNG.sync.read(fs.readFileSync(path.join(folder, 'out', byFile['dm_2.png'].side_by_side)));
  assert.equal(sideBySide.width, 3 * 20 + 2 * 8);
  assert.ok(fs.existsSync(path.join(folder, 'out', byFile['dm_2.png'].diff)));
  assert.ok(fs.existsSync(path.join(folder, 'out', byFile['dm_2.png'].thumbnail)), 'public pools get a thumbnail');
});

test('the approval label lets changes pass', () => {
  const folder = scenario();
  const { code, report } = run(path.join(folder, 'base'), path.join(folder, 'head'), path.join(folder, 'out'), '--approved');
  assert.equal(code, 0);
  assert.equal(report.result.failed, false);
});

test('a pool that stops loading fails even with the approval label', () => {
  const folder = scenario({ bbbbbbbb: { ok: false } });
  const { code, report } = run(path.join(folder, 'base'), path.join(folder, 'head'), path.join(folder, 'out'), '--approved');
  assert.equal(code, 1);
  assert.equal(report.counts['newly-failing'], 1);
  assert.equal(report.newly_failing[0].pool, 'bbbbbbbb');
});

test('differences within the allowed pixel count are unchanged', () => {
  const folder = scenario();
  const { report } = run(path.join(folder, 'base'), path.join(folder, 'head'), path.join(folder, 'out'), '--max-diff-pixels', '9');
  const image = report.images.find((entry) => entry.file === 'dm_2.png');
  assert.equal(image.class, 'unchanged');
  assert.equal(image.hash_differs, true);
});

test('renders from different build environments are not compared', () => {
  const folder = tempFolder();
  writeRenders(path.join(folder, 'base'), {}, { available: true, fingerprint: 'one' });
  writeRenders(path.join(folder, 'head'), {}, { available: true, fingerprint: 'two' });
  const { code, stderr } = run(path.join(folder, 'base'), path.join(folder, 'head'), path.join(folder, 'out'));
  assert.equal(code, 2);
  assert.match(stderr, /different build environments/);
});

test('a base without vt-render skips the comparison', () => {
  const folder = tempFolder();
  writeRenders(path.join(folder, 'base'), {}, { available: false });
  writeRenders(path.join(folder, 'head'), {});
  const { code, report } = run(path.join(folder, 'base'), path.join(folder, 'head'), path.join(folder, 'out'));
  assert.equal(code, 0);
  assert.equal(report.status, 'skipped');
  assert.match(buildComment(report), /skipped/);
});

test('the comment starts with the marker, escapes names and links thumbnails to the artifact', () => {
  const folder = scenario();
  const { report } = run(path.join(folder, 'base'), path.join(folder, 'head'), path.join(folder, 'out'));
  report.images[0].pool_name = '<script>|x';
  const body = buildComment(report, { assetsUrl: 'https://example.org/pr-1/', artifactUrl: 'https://example.org/artifact' });

  assert.ok(body.startsWith(MARKER));
  assert.match(body, /\| changed \| 2 \|/);
  assert.match(body, /<a href="https:\/\/example.org\/artifact"><img src="https:\/\/example.org\/pr-1\/thumbnails\/aaaaaaaa\/dm_2.png"/);
  assert.ok(!body.includes('<script>'));
  assert.match(body, /visual-change-approved/);
});
