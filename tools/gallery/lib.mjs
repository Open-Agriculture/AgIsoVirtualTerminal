// Helpers shared by the gallery generator and its tests.

/** Escapes text and attribute values for HTML. */
export function esc(value) {
  return String(value ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

/** Encodes each segment of a relative path for use in a URL. */
export function encodePath(relativePath) {
  return relativePath.split('/').map(encodeURIComponent).join('/');
}

/**
 * Orders versions newest first: numeric parts compare as numbers (1.10.0 after 1.9.2), and a pre-release
 * (2.0.0-rc1) comes before its release (2.0.0).
 */
export function compareVersionsDescending(a, b) {
  const split = (version) => {
    const [release, ...preRelease] = String(version).split('-');
    return { parts: release.split(/[^0-9]+/).filter(Boolean).map(Number), preRelease: preRelease.join('-') };
  };
  const va = split(a);
  const vb = split(b);
  for (let i = 0; i < Math.max(va.parts.length, vb.parts.length); i++) {
    const difference = (vb.parts[i] ?? -1) - (va.parts[i] ?? -1);
    if (difference !== 0) {
      return difference;
    }
  }
  if (va.preRelease !== vb.preRelease) {
    if (!va.preRelease) {
      return -1;
    }
    if (!vb.preRelease) {
      return 1;
    }
  }
  return vb.preRelease.localeCompare(va.preRelease, undefined, { numeric: true });
}

/** renders OK, partial (has unsupported objects or images that failed), or fails to load. */
export function poolStatus(manifest) {
  if (!manifest.load?.ok) {
    return { id: 'fails', label: 'fails to load' };
  }
  const unsupported = (manifest.unsupported_objects ?? []).some((object) => object.status === 'unsupported');
  if (unsupported || (manifest.errors ?? []).length > 0) {
    return { id: 'partial', label: 'partial' };
  }
  return { id: 'ok', label: 'renders OK' };
}

/** "Known issue #N" for GitHub issue and pull request URLs, "Known issue" for others; only https links. */
export function knownIssueBadges(urls) {
  return (urls ?? [])
    .filter((url) => /^https:\/\//.test(url))
    .map((url) => {
      const number = /\/(?:issues|pull)\/(\d+)(?:[/?#]|$)/.exec(url)?.[1];
      return { url, label: number ? `Known issue #${number}` : 'Known issue' };
    });
}

/**
 * The link that opens the render-bug issue form with its fields filled in. Every value is percent-encoded
 * on its own, so spaces, accents, slashes, & and # in names cannot break the query. The image is linked by
 * URL, never embedded.
 */
export function reportIssueUrl({ repo, version, poolId, poolName, objectId, objectType, imageUrl }) {
  const fields = [
    ['template', 'render-bug.yml'],
    ['title', `[Render] ${poolName || poolId}: ${objectType} ${objectId}`],
    ['pool-id', poolId],
    ['object-id', objectId],
    ['object-type', objectType],
    ['vt-version', version],
    ['image-url', imageUrl],
  ];
  const query = fields.map(([name, value]) => `${encodeURIComponent(name)}=${encodeURIComponent(String(value ?? ''))}`).join('&');
  return `https://github.com/${repo}/issues/new?${query}`;
}
