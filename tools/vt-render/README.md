# vt-render

A developer tool that renders ISOBUS object pools to PNG with the VT's own drawing components, without a CAN bus or a window, and writes a `manifest.json` with a hash of every image. Rendering a pool with two builds, for example `main` and a pull request, shows what a change does to how it is drawn.

## Build

```
cmake -S . -B build-renderer -DAGISOVT_BUILD_POOL_RENDERER=ON
cmake --build build-renderer --config Release --target VtRender
```

The executable is `vt-render` in `build-renderer/VtRender_artefacts/<config>/`. It compiles the same sources as the VT, so a change to the VT needs no change here.

## Usage

```
vt-render --pool <path>/pool.iop --out <dir>
vt-render --all <collection root> --out <dir>
```

`--all` renders every `<collection root>/pools/*/*/pool.iop` of an [AgIsoObjectPoolCollection](https://github.com/gunicsba/AgIsoObjectPoolCollection) checkout into `<dir>/<pool id>/`, where the id is the first 8 hex digits of the pool's SHA-256. Earlier images and the manifest are removed from each output folder first.

| Exit code | Meaning |
|---|---|
| 0 | Every pool loaded and every image was written |
| 1 | A pool loaded, but some image could not be rendered or written (see `errors` in its manifest) |
| 2 | A pool could not be loaded. Its folder holds only `manifest.json`, with the error |
| 3 | Wrong arguments |

With `--all`, the exit code is the worst over all pools.

## Output

| File | Contents |
|---|---|
| `ws_designator.png` | The working set designator as the working set selector shows it |
| `ws_designator_active.png` | The same with the active working set's highlight |
| `dm_<id>.png` | Each data mask |
| `am_<id>.png` | Each alarm mask |
| `skm_<id>.png` | Each soft key mask. Further pages are `skm_<id>_page<n>.png` |
| `screen_<id>.png` | Each data or alarm mask with the VT's soft key area to its right, showing the first page of its soft key mask |
| `aux_<id>.png` | Each Auxiliary Function Type 2 and Auxiliary Input Type 2 |
| `manifest.json` | See below |

Everything is drawn with the VT's defaults: data mask 480, 12 soft keys of 60 x 60.

### manifest.json

```
manifest_version     1
pool_id              the first 8 hex digits of the pool's SHA-256
pool_name, manufacturer, public, known_issues[]
                     not filled in yet
pool_sha256, pool_size
vt_commit            git describe of the VT source, with -dirty if the tree had changes
render_settings      platform, renderer, JUCE version, sizes and soft key layout
load                 { ok, error }; error carries the parser's message and the faulting object id
images[]             file, object_id, object_type, width, height, sha256; screens also have
                     soft_key_mask_id (null if none), soft key masks have page and page_count,
                     designators have active; auxiliary objects have function_type,
                     critical_control and single_assignment, functions also assignment_restriction
unsupported_objects[] object_id, object_type, status (unsupported or ignored), reason
errors[]             file, object_id, object_type, error for each image that could not be produced
```

`sha256` is taken over the pixels, not the PNG file. Lists are in a fixed order and the manifest holds no paths or timestamps, so two manifests can be compared as text.

## Comparing two builds

Build vt-render once from each version into its own build folder, then on the same machine render the same pools with each:

```
vt-render --pool pool.iop --out out/main
vt-render --pool pool.iop --out out/my-branch
diff -rq out/main out/my-branch
```

Only look at the images that differ. `vt_commit` in the manifest differs by design.

Text uses the machine's monospaced font, as the VT does, so only compare renders made on the same machine.
