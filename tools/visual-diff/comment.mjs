#!/usr/bin/env node
// Writes the visual diff pull request comment, as Markdown, from a report.json made by compare.mjs.
//
//   node comment.mjs --report <report folder>/report.json [--assets-url <url>] [--artifact-url <url>]
//                    [--max-thumbnails 50]
//
// --assets-url is where the report's thumbnails/ folder is published; without it the comment has no
// thumbnails and links to the local side-by-side files instead, which is what a local run wants.
// --artifact-url is the report artifact of the CI run, which every thumbnail links to.

import fs from 'node:fs';
import path from 'node:path';
import { parseArgs } from 'node:util';
import { fileURLToPath } from 'node:url';

export const MARKER = '<!-- vt-visual-diff -->';

/** A value for an HTML attribute. */
export function escapeAttribute(value) {
  return String(value ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

/** Text for Markdown and inline HTML: no tags, no table breaks, no accidental formatting. */
export function escapeText(value) {
  return String(value ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/\|/g, '&#124;')
    .replace(/([\\`*_[\]#~])/g, '\\$1')
    .replace(/\r?\n/g, ' ');
}

function shortSha(sha) {
  return sha ? String(sha).slice(0, 10) : 'unknown';
}

function describeImage(image) {
  const details = [`${escapeText(image.object_type)} ${escapeText(image.object_id)}`];
  if (image.size_changed) {
    details.push(`size ${image.size_changed.before.join('×')} → ${image.size_changed.after.join('×')}`);
  } else if (image.diff_pixels !== undefined) {
    details.push(`${image.diff_pixels} px differ`);
  }
  return `<b>${escapeText(image.pool)}/${escapeText(image.file)}</b> (${details.join(', ')})`;
}

function imageList(images) {
  return images.map((image) => `- ${describeImage(image)}`).join('\n');
}

export function buildComment(report, { assetsUrl = '', artifactUrl = '', maxThumbnails = 50 } = {}) {
  const lines = [MARKER, ''];

  if (report.status === 'skipped') {
    lines.push('## Visual diff: skipped', '', escapeText(report.skipped_reason));
    return lines.join('\n') + '\n';
  }

  const { counts, result, settings } = report;
  const approved = settings.approved && counts.changed + counts.removed > 0 && counts['newly-failing'] === 0;
  const verdict = result.failed ? '❌ failed' : approved ? '✅ changes approved' : counts.changed + counts.removed + counts.new > 0 ? '✅ passed' : '✅ no visual changes';
  lines.push(`## Visual diff: ${verdict}`, '');
  lines.push(
    `Base \`${shortSha(report.base?.vt_commit)}\` against head \`${shortSha(report.head?.vt_commit)}\`, ` +
      `pools \`${shortSha(report.head?.pools_commit)}\`. ` +
      `Threshold ${settings.threshold}, up to ${settings.max_diff_pixels} differing pixel(s) allowed.`,
    '',
  );
  for (const reason of result.reasons) {
    lines.push(`**${escapeText(reason)}.**`, '');
  }
  if (counts.changed + counts.removed > 0 && !settings.approved) {
    lines.push('If the changes are intended, add the `visual-change-approved` label.', '');
  }

  lines.push('| Class | Count |', '|---|---:|');
  for (const name of ['unchanged', 'changed', 'new', 'removed', 'newly-failing']) {
    lines.push(`| ${name} | ${counts[name]} |`);
  }
  lines.push('');

  if (report.newly_failing.length > 0) {
    lines.push('### Newly failing pools', '');
    for (const pool of report.newly_failing) {
      lines.push(`- <b>${escapeText(pool.pool)}</b> ${escapeText(pool.pool_name)}: ${escapeText(pool.error)}`);
    }
    lines.push('');
  }
  if (report.render_settings_changed.length > 0) {
    lines.push(`Render settings differ for: ${report.render_settings_changed.map(escapeText).join(', ')}.`, '');
  }

  const changed = report.images.filter((image) => image.class === 'changed');
  if (changed.length > 0) {
    lines.push(`<details><summary>Changed images (${changed.length})</summary>`, '');
    lines.push('Each image shows before, the difference in red, and after.', '');
    let shown = 0;
    const withoutThumbnail = [];
    for (const image of changed) {
      if (assetsUrl && image.thumbnail && shown < maxThumbnails) {
        const src = `${assetsUrl.replace(/\/$/, '')}/${image.thumbnail.split('/').map(encodeURIComponent).join('/')}`;
        const img = `<img src="${escapeAttribute(src)}" alt="${escapeAttribute(`${image.pool}/${image.file}`)}">`;
        lines.push(describeImage(image), '', artifactUrl ? `<a href="${escapeAttribute(artifactUrl)}">${img}</a>` : img, '');
        shown++;
      } else if (!assetsUrl) {
        lines.push(`- ${describeImage(image)}: ${escapeText(image.side_by_side)}`);
      } else {
        withoutThumbnail.push(image);
      }
    }
    if (withoutThumbnail.length > 0) {
      lines.push('', 'Without a thumbnail (the pool is not public, or the thumbnail limit was reached):', '', imageList(withoutThumbnail));
    }
    if (artifactUrl) {
      lines.push('', `All diffs and side-by-sides: [report artifact](${artifactUrl}).`);
    }
    lines.push('', '</details>', '');
  }

  for (const [name, title] of [['removed', 'Removed images'], ['new', 'New images']]) {
    const images = report.images.filter((image) => image.class === name);
    if (images.length > 0) {
      lines.push(`<details><summary>${title} (${images.length})</summary>`, '', imageList(images), '', '</details>', '');
    }
  }

  // GitHub rejects comments over 65536 characters
  let body = lines.join('\n') + '\n';
  if (body.length > 60000) {
    body = body.slice(0, 60000) + '\n\n…truncated; see the report artifact.\n';
  }
  return body;
}

function main() {
  const { values } = parseArgs({
    options: {
      report: { type: 'string' },
      'assets-url': { type: 'string', default: '' },
      'artifact-url': { type: 'string', default: '' },
      'max-thumbnails': { type: 'string', default: '50' },
    },
  });
  if (!values.report) {
    console.error('Usage: comment.mjs --report <report.json> [--assets-url <url>] [--artifact-url <url>] [--max-thumbnails N]');
    return 2;
  }
  const report = JSON.parse(fs.readFileSync(values.report, 'utf8'));
  process.stdout.write(
    buildComment(report, {
      assetsUrl: values['assets-url'],
      artifactUrl: values['artifact-url'],
      maxThumbnails: Number(values['max-thumbnails']),
    }),
  );
  return 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  process.exitCode = main();
}
