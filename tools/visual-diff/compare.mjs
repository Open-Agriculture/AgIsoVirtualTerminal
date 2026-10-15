#!/usr/bin/env node
// Compares two vt-render output folders, one rendered with the base build and one with the head build,
// and classifies every image as unchanged, changed, new or removed, and every pool that loaded with the
// base build but not with the head build as newly-failing.
//
//   node compare.mjs --base <base renders> --head <head renders> --out <report folder>
//                    [--threshold 0.1] [--max-diff-pixels 0] [--approved]
//
// Images whose pixel hashes (from manifest.json) are equal are unchanged without being opened. The others
// are compared with pixelmatch: an image is changed if more than --max-diff-pixels pixels differ by more
// than --threshold (pixelmatch's colour distance, 0 to 1). Anti-aliasing pixels are not counted.
//
// Writes <out>/report.json, and for every changed image a diff, a side-by-side (before | diff | after) and,
// for pools whose meta.yaml says public: true, a half-size thumbnail of the side-by-side.
//
// Exit code: 0 pass, 1 fail (anything changed, removed or newly-failing; with --approved only
// newly-failing), 2 the inputs could not be compared.

import fs from 'node:fs';
import path from 'node:path';
import { parseArgs } from 'node:util';
import { fileURLToPath } from 'node:url';
import pixelmatch from 'pixelmatch';
import { PNG } from 'pngjs';

export const CLASSES = ['unchanged', 'changed', 'new', 'removed', 'newly-failing'];
const GUTTER = 8;
const GUTTER_COLOUR = [128, 128, 128, 255];

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

/** The manifests of a vt-render --all output folder, by pool id, and the render-info.json the CI adds. */
export function readRenders(folder) {
  const infoFile = path.join(folder, 'render-info.json');
  const info = fs.existsSync(infoFile) ? readJson(infoFile) : null;
  const pools = new Map();
  if (fs.existsSync(folder)) {
    for (const entry of fs.readdirSync(folder, { withFileTypes: true })) {
      const manifestFile = path.join(folder, entry.name, 'manifest.json');
      if (entry.isDirectory() && fs.existsSync(manifestFile)) {
        pools.set(entry.name, readJson(manifestFile));
      }
    }
  }
  return { folder, info, pools };
}

function readPng(file) {
  return PNG.sync.read(fs.readFileSync(file));
}

function writePng(file, png) {
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, PNG.sync.write(png));
}

function filled(width, height, colour) {
  const png = new PNG({ width, height });
  for (let i = 0; i < png.data.length; i += 4) {
    png.data.set(colour, i);
  }
  return png;
}

/** before | diff | after, top aligned; a missing diff (the sizes differ) leaves its slot empty. */
function sideBySide(before, diff, after) {
  const middle = diff ?? filled(Math.max(before.width, after.width), Math.max(before.height, after.height), GUTTER_COLOUR);
  const parts = [before, middle, after];
  const width = parts.reduce((sum, part) => sum + part.width, 0) + 2 * GUTTER;
  const height = Math.max(...parts.map((part) => part.height));
  const result = filled(width, height, GUTTER_COLOUR);
  let x = 0;
  for (const part of parts) {
    PNG.bitblt(part, result, 0, 0, part.width, part.height, x, 0);
    x += part.width + GUTTER;
  }
  return result;
}

/** Averages each 2 x 2 block, which keeps thin lines visible where dropping pixels would lose them. */
function halfSize(png) {
  const width = Math.max(1, Math.floor(png.width / 2));
  const height = Math.max(1, Math.floor(png.height / 2));
  const result = new PNG({ width, height });
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      for (let channel = 0; channel < 4; channel++) {
        let sum = 0;
        for (const [dx, dy] of [[0, 0], [1, 0], [0, 1], [1, 1]]) {
          const sx = Math.min(png.width - 1, 2 * x + dx);
          const sy = Math.min(png.height - 1, 2 * y + dy);
          sum += png.data[4 * (sy * png.width + sx) + channel];
        }
        result.data[4 * (y * width + x) + channel] = Math.round(sum / 4);
      }
    }
  }
  return result;
}

function imagesByFile(manifest) {
  return new Map((manifest?.images ?? []).map((image) => [image.file, image]));
}

function loaded(manifest) {
  return Boolean(manifest?.load?.ok);
}

/** Render settings without what differs between two correct builds. */
function comparableSettings(manifest) {
  return JSON.stringify(manifest?.render_settings ?? null);
}

export function compare({ base, head, out, threshold = 0.1, maxDiffPixels = 0, approved = false }) {
  const report = {
    version: 1,
    status: 'compared',
    base: base.info,
    head: head.info,
    settings: { threshold, max_diff_pixels: maxDiffPixels, approved },
    counts: Object.fromEntries(CLASSES.map((name) => [name, 0])),
    newly_failing: [],
    still_failing: [],
    render_settings_changed: [],
    images: [],
    result: { failed: false, reasons: [] },
  };

  if (base.info && base.info.available === false) {
    report.status = 'skipped';
    report.skipped_reason = 'The base commit has no vt-render, so there is nothing to compare against.';
    return report;
  }
  if (base.info && head.info && base.info.fingerprint !== head.info.fingerprint) {
    throw new Error(
      `The base and head renders come from different build environments (${base.info.fingerprint} and ` +
        `${head.info.fingerprint}), so their pixels cannot be compared. Re-run the job.`,
    );
  }

  const poolIds = [...new Set([...base.pools.keys(), ...head.pools.keys()])].sort();
  for (const pool of poolIds) {
    const before = base.pools.get(pool);
    const after = head.pools.get(pool);
    const about = after ?? before;
    const poolInfo = { pool, pool_name: about.pool_name ?? '', public: Boolean(about.public) };

    if (loaded(before) && !loaded(after)) {
      report.newly_failing.push({ ...poolInfo, error: after ? after.load?.error ?? 'unknown error' : 'no manifest.json was written' });
      report.counts['newly-failing']++;
      continue;
    }
    if (!loaded(before) && !loaded(after)) {
      report.still_failing.push({ ...poolInfo, error: after?.load?.error ?? before?.load?.error ?? 'no manifest.json' });
      continue;
    }
    if (before && after && comparableSettings(before) !== comparableSettings(after)) {
      report.render_settings_changed.push(pool);
    }

    const beforeImages = loaded(before) ? imagesByFile(before) : new Map();
    const afterImages = imagesByFile(after);
    const files = [...new Set([...beforeImages.keys(), ...afterImages.keys()])].sort();

    for (const file of files) {
      const beforeImage = beforeImages.get(file);
      const afterImage = afterImages.get(file);
      const entry = {
        ...poolInfo,
        file,
        object_id: (afterImage ?? beforeImage).object_id,
        object_type: (afterImage ?? beforeImage).object_type,
      };

      if (!beforeImage) {
        entry.class = 'new';
      } else if (!afterImage) {
        entry.class = 'removed';
      } else if (beforeImage.sha256 === afterImage.sha256) {
        entry.class = 'unchanged';
      } else {
        Object.assign(entry, diffImage(base.folder, head.folder, out, pool, file, poolInfo.public, threshold, maxDiffPixels));
      }

      report.counts[entry.class]++;
      if (entry.class !== 'unchanged' || entry.hash_differs) {
        report.images.push(entry);
      }
    }
  }

  const { counts } = report;
  if (counts['newly-failing'] > 0) {
    report.result.reasons.push(`${counts['newly-failing']} pool(s) no longer load`);
  }
  if (!approved && counts.changed + counts.removed > 0) {
    report.result.reasons.push(`${counts.changed} image(s) changed and ${counts.removed} removed, without the visual-change-approved label`);
  }
  report.result.failed = report.result.reasons.length > 0;
  return report;
}

function diffImage(baseFolder, headFolder, out, pool, file, isPublic, threshold, maxDiffPixels) {
  const before = readPng(path.join(baseFolder, pool, file));
  const after = readPng(path.join(headFolder, pool, file));
  const result = { hash_differs: true };
  let diff = null;

  if (before.width !== after.width || before.height !== after.height) {
    result.class = 'changed';
    result.size_changed = { before: [before.width, before.height], after: [after.width, after.height] };
  } else {
    diff = new PNG({ width: before.width, height: before.height });
    result.diff_pixels = pixelmatch(before.data, after.data, diff.data, before.width, before.height, { threshold });
    result.class = result.diff_pixels > maxDiffPixels ? 'changed' : 'unchanged';
  }

  if (result.class === 'changed') {
    if (diff) {
      result.diff = `diff/${pool}/${file}`;
      writePng(path.join(out, result.diff), diff);
    }
    const combined = sideBySide(before, diff, after);
    result.side_by_side = `side-by-side/${pool}/${file}`;
    writePng(path.join(out, result.side_by_side), combined);
    if (isPublic) {
      result.thumbnail = `thumbnails/${pool}/${file}`;
      writePng(path.join(out, result.thumbnail), halfSize(combined));
    }
  }
  return result;
}

function main() {
  const { values } = parseArgs({
    options: {
      base: { type: 'string' },
      head: { type: 'string' },
      out: { type: 'string' },
      threshold: { type: 'string', default: '0.1' },
      'max-diff-pixels': { type: 'string', default: '0' },
      approved: { type: 'boolean', default: false },
    },
  });
  const threshold = Number(values.threshold);
  const maxDiffPixels = Number(values['max-diff-pixels']);
  if (!values.base || !values.head || !values.out || !(threshold >= 0 && threshold <= 1) || !(Number.isInteger(maxDiffPixels) && maxDiffPixels >= 0)) {
    console.error('Usage: compare.mjs --base <folder> --head <folder> --out <folder> [--threshold 0..1] [--max-diff-pixels N] [--approved]');
    return 2;
  }

  fs.rmSync(values.out, { recursive: true, force: true });
  fs.mkdirSync(values.out, { recursive: true });

  let report;
  try {
    report = compare({
      base: readRenders(values.base),
      head: readRenders(values.head),
      out: values.out,
      threshold,
      maxDiffPixels,
      approved: values.approved,
    });
  } catch (error) {
    console.error(error.message);
    return 2;
  }
  fs.writeFileSync(path.join(values.out, 'report.json'), JSON.stringify(report, null, 2) + '\n');

  if (report.status === 'skipped') {
    console.log(report.skipped_reason);
    return 0;
  }
  console.log(CLASSES.map((name) => `${name}: ${report.counts[name]}`).join(', '));
  for (const reason of report.result.reasons) {
    console.log(`FAIL: ${reason}`);
  }
  return report.result.failed ? 1 : 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  process.exitCode = main();
}
