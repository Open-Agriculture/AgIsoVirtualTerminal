# Object pool CI

The VT is checked against real object pools from
[AgIsoObjectPoolCollection](https://github.com/gunicsba/AgIsoObjectPoolCollection), which is the git
submodule `tests/pools`. [vt-render](../tools/vt-render/README.md) draws every pool with the VT's own
drawing code, and three workflows use the renders:

| Workflow | When | What |
|---|---|---|
| **Object pools** (`object-pools.yml`) | every pull request, pushes to `main` | `pool-load-check`: every pool must load. `visual-diff`: renders the pools with the base commit's build and the pull request's build, and compares them. `submodule-pin-in-own-pr`: a pin bump comes alone |
| **Visual diff comment** (`visual-diff-comment.yml`) | after each Object pools run of a pull request | posts or edits the visual diff comment on the pull request |
| **Release gallery** (`release-gallery.yml`) | a tag is pushed, a release is published | adds the release to the render gallery on GitHub Pages, and attaches the renders to the release as a zip |

Clone with `git clone --recursive`, or in an existing clone run `git submodule update --init tests/pools`.

## Bumping the pools

The submodule is pinned to one commit of the collection, so new or changed pools never change the CI results
by themselves. To take newer pools:

```
git -C tests/pools fetch origin
git -C tests/pools checkout <collection commit>     # usually origin/main
git add tests/pools
git commit -m "Bump the object pool collection to <short commit>"
```

**Open a pull request with only this change.** The `submodule-pin-in-own-pr` check fails if a pull
request moves the pin and changes any other file (`.gitmodules` excepted). Kept alone, the bump's checks
say exactly what the new pools do:

- `pool-load-check` lists any new pool that does not load. Fix the VT in a separate pull request first,
  or bump to a collection commit without that pool.
- `visual-diff` renders the new pools with both the base build and the pull request's build. The code
  is the same in both, so it should find no changes. If it does, the renders are not reproducible, and
  that needs looking into before the bump is merged.

## pool-load-check

Builds the VT and vt-render (Linux, `ubuntu-24.04`), runs `vt-render --all tests/pools --out out`, and prints a
table of every pool: whether it loaded, the error, and the number of unsupported and ignored objects. It
fails if any pool does not load, or vt-render itself fails. The renders are uploaded as the
`object-pool-renders` artifact.

## visual-diff

1. Takes the pull request's renders from `pool-load-check`.
2. Builds vt-render from the pull request's base commit and renders the same pools (the pull request's pin)
   with it. These renders are cached by base commit, collection commit and build environment. A push to
   `main` renders that commit and caches it, and pull requests based on it reuse it: GitHub lets a pull
   request read the caches of its base branch, but not those of other pull requests.
3. Compares the two with `tools/visual-diff/compare.mjs`. Images whose pixel hashes match are unchanged.
   The others are compared with [pixelmatch](https://github.com/mapbox/pixelmatch). It is plain
   JavaScript, so it gives the same answer on every machine with no native binary to install. Its speed
   hardly matters here, because only images whose hashes differ reach it.
4. Classifies every image as **unchanged**, **changed**, **new** or **removed**, and every pool that
   loaded with the base build but not the pull request's build as **newly-failing**.
5. Writes a diff and a before | diff | after side-by-side for every changed image, uploaded as the
   `visual-diff-report` artifact.

The job fails if anything is changed, removed or newly-failing. The `visual-change-approved` label
accepts changed and removed images; newly-failing pools fail even with the label. Adding or removing the
label re-runs the job (`visual-change-approval.yml`).

An image counts as changed when more than `VISUAL_DIFF_MAX_DIFF_PIXELS` pixels (default 0) differ by more
than `VISUAL_DIFF_THRESHOLD` (pixelmatch's colour distance from 0 to 1, default 0.1); anti-aliased pixels do
not count. Set either as a repository variable to change it.

Renders are only compared when both builds ran in the same environment: `render-info.json` holds a
fingerprint of the OS release, the compiler and the libraries involved in drawing, and a mismatch stops
the comparison instead of reporting false changes. It can happen while GitHub rolls out a new runner
image; re-run the job.

If the base commit has no vt-render (the pull request that adds it), the comparison is skipped.

### The pull request comment

The Visual diff comment workflow keeps one comment per pull request up to date: the counts per class,
the newly failing pools, and collapsible lists of changed, removed and new images. It runs from `main`
with permission to comment, because a pull request from a fork cannot comment from its own run. It
reads the pull request's report only as data.

Thumbnails of changed images are pushed to the `visual-diff-assets` branch under `pr-<number>/`, replacing
the previous run's, and link to the report artifact. Only pools whose `meta.yaml` says `public: true` get
thumbnails; the others are listed by name, and their images are in the artifact.

### Running it locally

With Node 20 or later, and vt-render built from both commits (see [vt-render](../tools/vt-render/README.md)):

```
git worktree add ../vt-base origin/main
cmake -S ../vt-base -B ../vt-base/build-renderer -DAGISOVT_BUILD_POOL_RENDERER=ON
cmake --build ../vt-base/build-renderer --config Release --target VtRender

../vt-base/build-renderer/VtRender_artefacts/Release/vt-render --all tests/pools --out renders-base
build-renderer/VtRender_artefacts/Release/vt-render --all tests/pools --out renders-head

npm ci --prefix tools/visual-diff
node tools/visual-diff/compare.mjs --base renders-base --head renders-head --out visual-diff
node tools/visual-diff/comment.mjs --report visual-diff/report.json
```

`compare.mjs` takes `--threshold`, `--max-diff-pixels` and `--approved`, and exits with 0 (pass), 1
(fail) or 2 (could not compare). `comment.mjs` prints the comment, linking to the local side-by-side
files. Tests: `npm test --prefix tools/visual-diff`.

## Release gallery

Every pushed tag becomes `/<tag>/` on GitHub Pages, and earlier releases stay:

- `/<version>/index.html`: a card per pool, with its working set designator, manufacturer, name and
  status: renders OK, partial (has unsupported objects) or fails to load. Can be filtered by manufacturer.
- `/<version>/<pool id>/index.html`: the composed screens, then the data, alarm and soft key masks, the
  warnings, and the unsupported objects. Photos in the pool's `reference/` folder of the collection
  appear next to the render of the object they show. The pool's `known_issues` appear as badges.
- `/compare/<pool id>/index.html`: two releases of a pool side by side, and on top of each other with a
  slider.
- `/latest/` goes to the newest release, and every page has a release selector.

The pages load nothing from other sites and work when opened from disk. **Only pools marked
`public: true` in their `meta.yaml` are published**, in the gallery and in the zip attached to the release.

Build it locally from renders:

```
node tools/gallery/generate.mjs --renders renders-head --collection tests/pools --site site \
  --version 1.6.0 --repo Open-Agriculture/AgIsoVirtualTerminal --site-url https://open-agriculture.github.io/AgIsoVirtualTerminal
```

then open `site/index.html`. Tests: `npm test --prefix tools/gallery`.

## Reporting rendering problems

Under every gallery image, **Looks wrong?** opens the *Rendering problem* issue form
(`.github/ISSUE_TEMPLATE/render-bug.yml`), with the pool id, object id, object type, VT version and image
URL filled in. The reporter describes the problem, and can add a photo from a real terminal.

## Repository setup

These are done once in the repository settings:

- Labels: `visual-change-approved`, `visual` and `render-bug` (the issue form only applies labels that exist).
- Pages: deploy from the `gh-pages` branch, root folder. For a custom domain, set the repository variable
  `GALLERY_URL` so that the image links in issues point at it.
- Branch protection: require `pool-load-check`, `visual-diff` and `submodule-pin-in-own-pr`.
- Optional repository variables: `VISUAL_DIFF_THRESHOLD`, `VISUAL_DIFF_MAX_DIFF_PIXELS`.
