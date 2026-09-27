#!/usr/bin/env node
// Summarises a vt-render --all run: one row per pool of the collection with whether it loaded, the load
// error and the number of unsupported objects. Pools without a manifest.json (vt-render stopped before
// reaching them) count as not loaded.
//
//   node summarize.mjs --renders <vt-render output> --pools <collection checkout> [--summary <file>]
//
// The table is printed and, with --summary (the CI passes $GITHUB_STEP_SUMMARY), appended to that file.
// Exit code 1 if any pool did not load, or vt-render itself failed (render-info.json exit code above 2).

import fs from 'node:fs';
import path from 'node:path';
import { parseArgs } from 'node:util';

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

function escapeCell(value) {
  return String(value ?? '').replace(/\|/g, '\\|').replace(/\r?\n/g, ' ');
}

/** Every <pools>/pools/<manufacturer>/<slug>/ with a pool.iop, with the id from its meta.yaml. */
function findPools(collection) {
  const root = path.join(collection, 'pools');
  const pools = [];
  for (const manufacturer of fs.readdirSync(root, { withFileTypes: true }).filter((entry) => entry.isDirectory())) {
    for (const slug of fs.readdirSync(path.join(root, manufacturer.name), { withFileTypes: true }).filter((entry) => entry.isDirectory())) {
      const folder = path.join(root, manufacturer.name, slug.name);
      if (!fs.existsSync(path.join(folder, 'pool.iop'))) {
        continue;
      }
      const metaFile = path.join(folder, 'meta.yaml');
      const meta = fs.existsSync(metaFile) ? fs.readFileSync(metaFile, 'utf8') : '';
      const id = /^id:\s*["']?([0-9a-f]{8})["']?\s*$/m.exec(meta)?.[1] ?? null;
      pools.push({ id, folder: `${manufacturer.name}/${slug.name}` });
    }
  }
  return pools.sort((a, b) => a.folder.localeCompare(b.folder));
}

function main() {
  const { values } = parseArgs({
    options: {
      renders: { type: 'string' },
      pools: { type: 'string' },
      summary: { type: 'string', default: '' },
    },
  });
  if (!values.renders || !values.pools) {
    console.error('Usage: summarize.mjs --renders <folder> --pools <collection checkout> [--summary <file>]');
    return 2;
  }

  const rows = [];
  for (const pool of findPools(values.pools)) {
    const manifestFile = pool.id ? path.join(values.renders, pool.id, 'manifest.json') : null;
    if (!manifestFile || !fs.existsSync(manifestFile)) {
      rows.push({ ...pool, loaded: false, error: pool.id ? 'no manifest.json: vt-render did not finish this pool' : 'meta.yaml has no valid id', unsupported: '' });
      continue;
    }
    const manifest = readJson(manifestFile);
    rows.push({
      ...pool,
      loaded: Boolean(manifest.load?.ok),
      error: manifest.load?.error ?? '',
      unsupported: (manifest.unsupported_objects ?? []).filter((object) => object.status === 'unsupported').length,
      ignored: (manifest.unsupported_objects ?? []).filter((object) => object.status === 'ignored').length,
      renderErrors: (manifest.errors ?? []).length,
    });
  }

  const failed = rows.filter((row) => !row.loaded);
  const infoFile = path.join(values.renders, 'render-info.json');
  const exitCode = fs.existsSync(infoFile) ? readJson(infoFile).exit_code : null;
  const crashed = exitCode !== null && exitCode !== undefined && ![0, 1, 2].includes(Number(exitCode));

  const lines = [
    '## Object pool load check',
    '',
    `${rows.length - failed.length} of ${rows.length} pools loaded.` + (crashed ? ` **vt-render exited with ${exitCode}.**` : ''),
    '',
    '| Pool id | Folder | Loaded | Error | Unsupported objects | Ignored objects | Image errors |',
    '|---|---|---|---|---:|---:|---:|',
    ...rows.map((row) =>
      `| ${escapeCell(row.id ?? '?')} | ${escapeCell(row.folder)} | ${row.loaded ? 'yes' : '**no**'} | ${escapeCell(row.error)} | ` +
        `${escapeCell(row.unsupported)} | ${escapeCell(row.ignored ?? '')} | ${escapeCell(row.renderErrors ?? '')} |`,
    ),
    '',
  ];
  const text = lines.join('\n');
  console.log(text);
  if (values.summary) {
    fs.appendFileSync(values.summary, text + '\n');
  }

  for (const row of failed) {
    // GitHub shows these as annotations on the run
    if (process.env.GITHUB_ACTIONS) {
      console.log(`::error title=Pool ${row.id ?? row.folder} does not load::${row.folder}: ${row.error}`);
    }
  }
  return failed.length > 0 || crashed ? 1 : 0;
}

process.exitCode = main();
