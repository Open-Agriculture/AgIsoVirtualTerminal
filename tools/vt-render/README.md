# vt-render

Renders ISOBUS object pools to PNG with the VT's own drawing components, without a CAN bus or a window,
and writes a `manifest.json` with a hash of every image. The output depends only on the pool, its
`meta.yaml` and the VT source. Rendering the pools with the target branch and with the head of a pull
request, then comparing the two manifests, shows every image the pull request changes.

## Build

```
cmake -S . -B build-renderer -DAGISOVT_BUILD_POOL_RENDERER=ON
cmake --build build-renderer --config Release --target VtRender
```

The executable is `vt-render` in `build-renderer/VtRender_artefacts/<config>/`.

## Usage

```
vt-render --pool <path>/pool.iop --meta <path>/meta.yaml --out <dir>
vt-render --all <collection root> --out <dir>
```

`--all` renders every `<collection root>/pools/*/*/` of an
[AgIsoObjectPoolCollection](https://github.com/gunicsba/AgIsoObjectPoolCollection) checkout, such as the
`tests/pools` submodule, into
`<dir>/<pool id>/`. Before writing, earlier images and the manifest are removed from each output folder,
so an image that is no longer produced does not linger.

| Exit code | Meaning |
|---|---|
| 0 | Every pool loaded and every image was written |
| 1 | A pool loaded, but some image could not be rendered or written (see `errors` in its manifest) |
| 2 | A pool or its `meta.yaml` could not be loaded. Its folder holds only `manifest.json`, with the error |
| 3 | Wrong arguments |

With `--all`, the exit code is the worst over all pools.

## Output

| File | Contents |
|---|---|
| `ws_designator.png` | The working set designator as the working set selector shows it: fitted to the 72 x 72 button, on the working set's background colour, inside the 96 px wide selector column |
| `ws_designator_active.png` | The same with the highlight of the active working set |
| `dm_<id>.png` | Each data mask, `data_mask_size` square |
| `am_<id>.png` | Each alarm mask, `data_mask_size` square |
| `skm_<id>.png` | Each soft key mask, in the configured soft key layout |
| `screen_<id>.png` | Each data mask with the soft key area of the VT to its right, holding the soft key mask the data mask references |
| `manifest.json` | See below |

### Render settings

Taken from the `render:` block of `meta.yaml`. A value that is `TODO` falls back to the VT's default:
data mask 480, soft keys 60 x 60, 6 soft keys. The keys fill a column top to bottom, as many as fit
next to the data mask, then the next column to the left, as the VT's soft key mask does. Keys beyond
`softkey_count` are drawn off the mask by the VT, and are listed in `unsupported_objects`.

`meta.yaml` is read line by line, not with a YAML parser: only `id`, `name`, `manufacturer`, `public`,
`known_issues` and the `render:` block are used.

### manifest.json

```
manifest_version     1
pool_id              meta.yaml id; the first 8 hex digits of the pool's sha256 if meta.yaml is unusable
pool_name, manufacturer
public               meta.yaml public: whether the pool may be shown in a public gallery
known_issues[]       meta.yaml known_issues
pool_sha256, pool_size
vt_commit            commit the VT was built from, and vt_commit_dirty if the tree had changes
render_settings      platform, renderer, JUCE version, sizes and soft key layout (from_meta lists the
                     values that came from meta.yaml), the bundled fonts with their sha256
load                 { ok, error }; error carries the parser's message and the faulting object id
images[]             file, object_id, object_type, width, height, sha256; screens also have
                     soft_key_mask_id (null if none), designators have active
unsupported_objects[] object_id, object_type, status, reason
                     unsupported: a drawable object type the VT does not draw (window mask, key group,
                                  output list, arched bar graph, graphics context, animation, ...)
                     ignored:     an object no image shows (auxiliary functions and inputs, external
                                  object definitions), or a key beyond the soft key positions
errors[]             file, object_id, object_type, error for each image that could not be produced
```

Lists are in a fixed order (images by file name, objects by id), and the manifest holds no paths and no
timestamps, so two manifests can be compared as text. `sha256` is taken over the pixels, not the PNG
file: the width and height as 32-bit big endian, then every pixel as unpremultiplied RGBA8, row by row.

When comparing two builds, `vt_commit` differs by design. Check that `render_settings` is equal before
comparing the images: a change there, such as another platform or font, changes every image.

## What makes it deterministic

- **Fonts**: every font resolves to the bundled DejaVu Sans Mono 2.37 (`res/fonts`, regular, bold, oblique
  and bold oblique), and fallback to system fonts is off, so a character missing from the font is drawn
  as the font's missing-glyph box on every machine.
- **Software rendering**: images, including decoded picture graphics, are JUCE software images. On
  Windows the default image type would be drawn by Direct2D on the GPU.
- **Fixed scale**: one image pixel per VT pixel. No window is opened, so the display's DPI plays no part.
- **No carried state**: JUCE's glyph cache matches fonts only approximately, so it is cleared before
  every image. Otherwise text would depend on what had been drawn before it.
- **Flashing**: timers never run, so flashing objects are always drawn in their visible state.
- **No timestamps**: neither the PNG files nor the manifest contain one.

Renders are reproducible on one platform. Windows and Linux load the font through different libraries
(DirectWrite, FreeType), so compare images made on the same platform; `render_settings.platform` says
which one.

## Determinism test

```
cmake -S . -B build-renderer -DAGISOVT_BUILD_POOL_RENDERER=ON
cmake --build build-renderer --config Release --target VtRender
ctest --test-dir build-renderer -C Release -R vt-render --output-on-failure
```

It renders the `tests/pools` submodule, or another checkout given with
`-DAGISOVT_POOL_COLLECTION_DIR=<path>`.

`determinism_test.cmake` renders the collection twice with `--all` and requires every file to be the
same. It then renders each pool on its own in a new process and requires the same files as the `--all`
run, where other pools were drawn first. Finally it checks that a pool that cannot be parsed exits with
2 and gets a manifest that reports the error.

## In CI

vt-render runs on every pull request and release; see [doc/object-pool-ci.md](../../doc/object-pool-ci.md).
