# Touch settings page — design

Status: **design, not yet implemented.** Branch `feature/touch-settings-page`.

## Why

The VT is normally run on a tablet in a cab, where the desktop-sized menu bar is too small a
target to hit reliably. [#212](https://github.com/Open-Agriculture/AgIsoVirtualTerminal/pull/212)
addresses that by doubling the menu bar and its popup item heights, but applies globally and
looks oversized on a desktop.

The review on #212 landed on the real fix:

> In my opinion for proper touch based operation the whole menubar needs to be replaced with some
> dedicated settings button (around the ACK button) and touch oriented menu structure. I think it
> could be an interim solution until we got that implemented properly, but it should have an user
> option to disable it.

So: #212 stays as the interim (plus an opt-out), and this document describes the replacement.

## Shape

A **single settings page**, opened by a cogwheel next to the ACK button, occupying the whole
window. Not an overlay on the Data Mask area — ISO 11783-6 4.5.2 requires the Data Mask's square
aspect ratio to be "strictly enforced", so shrinking or overlaying it fights a constraint for no
benefit.

**No popups anywhere.** Everything that is a modal dialog today becomes a row, a pane, or an
inline confirmation.

```
┌──────────────────────────────────────────────────────────────────────┐
│  ←                                                          Settings │
├────────────────────────────────────┬─────────────────────────────────┤
│ [ ISOBUS Language/Units ][ Hardware ]   CONFIGURATION                │
│                                    │                                 │
│  Language      [HU] Hungarian hu/HU│    App language    Follow System │
│  Decimal symbol             Point  │    VT version                 3 │
│  Date format             yyyymmdd  │    VT number        1 (Primary) │
│  Time format                  24 h │    CAN hardware               1 │
│  Units system               Metric │    Log level    👁  debug        │
│  Distance units             Metric │                                 │
│  Area units                 Metric │   CONTROL                       │
│  Volume units               Metric │    ☑ Auto-start VT on launch    │
│  Mass units                 Metric │    ☐ Always on top              │
│  Temperature units          Metric │    ☐ (third — TBD)              │
│                                    │    ☐ CAN traffic log            │
│                                    │    ☐ Save IOP before parsing    │
│                                    │    ACK button              ›    │
│                                    │                                 │
│                                    │   TROUBLESHOOTING               │
│                                    │    [📦 Diagnostics] [📦 Session] │
│                                    │    [🗑 Delete pools]            │
├────────────────────────────────────┴─────────────────────────────────┤
│  AgISOVirtualTerminal 1.4.0-44-gd8b8b21                              │
│  [ RESTORE DEFAULTS ]                        [ CANCEL ]  [ APPLY ]   │
└──────────────────────────────────────────────────────────────────────┘
```

The left pane is **swappable** between ISOBUS Language/Units and Hardware Capabilities via the
segmented control at its top. A segmented control rather than a button, so it reads as "these are
two views of this pane" rather than "this navigates away". Pending edits in the pane you swap away
from must survive the swap and still be committed by APPLY — otherwise it is a trap.

## Staged apply

Settings are **staged, not applied instantly**, matching the current popups' OK/Cancel semantics.

- **APPLY** commits everything pending, across both panes.
- **CANCEL** discards everything pending and leaves the page.
- **RESTORE DEFAULTS** resets to defaults (still staged — the user must then APPLY).

This is a safety property, not just a convention: CAN hardware and VT number can disconnect an
operator mid-operation, and a mis-tap should not take effect until deliberately confirmed.

The cost is that every setting needs pending-vs-active state and CANCEL has to roll all of it back.

## The cogwheel

Lives next to the ACK button, inside `WorkingSetSelectorComponent` — which already owns the ACK
button (`set_ack_button_visible()`, `update_ack_button_bounds()`), so this is the same pattern in
the same component.

**The CAN status indicator merges into it**, as a coloured dot in the centre of the gear. This is
what commercial units do and it reads clearly in practice, including on a sun-washed screen.
It frees the 150 px (`CAN_STATUS_INDICATOR_WIDTH`) currently reserved at the right of the menu
bar. It only encodes binary connected/disconnected — but so does today's text.

## Icons

Icons reduce the translation surface, they do not remove it. Use them where the meaning is
unambiguous, and keep text everywhere else — a wrong guess on a safety-adjacent control is worse
than a translated word.

Good candidates: the gear itself, eye / crossed-eye for log window visibility, trash for delete
pools, package for diagnostic bundles, country flags for the locale row.

Not candidates: "Decimal symbol", "VT version", "Country code" — these stay text.

## The locale row

Today language and country are two free-text editors. They are one concept to an operator, so
they become **one row**:

```
Language        [HU]  Hungarian — hu/HU
```

flag + English name + the raw codes. The raw codes stay visible so the row remains diagnosable
when someone reports odd client behaviour.

**Curated list plus an escape hatch.** Replacing free text with a fixed dropdown would be a
capability regression — the protocol permits pairs no curated list will cover. So: a curated list
of common locales, plus a **Custom…** entry at the bottom that reveals the two raw code fields.
Custom uses the `xx` placeholder flag.

This means the curated list does not have to be exhaustive and is not worth agonising over: a
locale we miss is either raised as a ticket and added, or entered through Custom in the meantime.

Note the protocol does *not* define a subset of valid countries. AgIsoStack documents the field as
"alpha-2 country codes in accordance with ISO 3166-1", so any country code is legal — the curation
is ours. Reasonable starting set, taken from the languages a real commercial implement pool ships:

```
bg cs da de el en es et fi fr hr hu it lt lv nl no pl pt ro ru sk sl sr sv tr uk
```

### Flags

[lipis/flag-icons](https://github.com/lipis/flag-icons), **MIT licensed**, keyed on ISO 3166-1
alpha-2 — the same codes the protocol uses, so the mapping is direct.

Vendor a **curated subset** of the SVGs into `res/flags/` rather than pulling the repository via
`FetchContent`; it is ~17 MB and we need a few dozen files. Keep the MIT notice with them. JUCE
renders them via `Drawable::createFromSVG`.

Do **not** use emoji flags: Windows ships no glyphs for regional-indicator pairs, so `🇭🇺`
renders as two boxed letters.

## Translations

Two different languages live in this app and must not be conflated:

| | What it is | Where it lives |
|---|---|---|
| **ISOBUS Language/Units** | Broadcast to implements so they localise their own pools. Protocol state. | `languageCommandInterface`, defaults `en`/`US` |
| **App language** | The VT's own chrome. Does not exist today. | JUCE `LocalisedStrings` |

An operator may legitimately want the UI in Hungarian while still commanding implements in
English. Both rows exist from day one, even if App language initially offers only English.

Implement pool text is **not** ours to translate — that follows the ISOBUS language command.
Only VT chrome is translatable.

Mechanically this is `TRANS("…")` plus `LocalisedStrings`, already in JUCE, no new dependency.
Translation files are a trivial format:

```
language: Hungarian
countries: hu

"Auto-start VT on launch" = "VT automatikus indítása"
```

They go in a `translations/` folder next to the executable rather than embedded as `BinaryData`,
so a contributor can add a language by dropping in a file and testing it without a toolchain.
"Follow System" resolves via `SystemStats::getDisplayLanguage()`.

**Scope: the new settings page only.** `getCommandInfo()` supplies the menu bar strings, so
wrapping those too would turn this into an app-wide string pass. The menu bar is likely to be
retired sooner than expected, and its untranslated strings leave with it.

## Open questions

- **The third Control checkbox** — if it is the touch-mode / hide-menu-bar toggle, it is also
  Marton's opt-out for #212 and should land alongside this rather than after.
- **Start/Stop.** It is under Control today, but it is the most-used control in the app and
  burying it two taps behind a cogwheel is a regression. Options: keep it on the main screen next
  to the cogwheel, or let APPLY subsume it (changing CAN hardware stages a stop/reconfigure/start).
  Undecided.
- **Working set selector orientation.** A horizontal mode is wanted for running the VT
  side-by-side with another application. Bigger than it looks: `WorkingSetSelectorComponent`
  hardcodes `WIDTH = 96`, pins itself to `(0, 0, WIDTH, minimum_height())`, and derives positions
  from a `static button_bounds(int index)` with no instance state to consult. Flipping it also
  moves the ACK button and interacts with the soft key mask column. Out of scope for the first
  pass, noted here so the layout work accounts for it.

## Suggested phasing

1. Cogwheel (with merged CAN status) + page shell + staged-apply plumbing + the Control checkboxes
   and Troubleshooting buttons. Menu bar untouched.
2. Right pane Configuration rows, migrating the Logging / ACK / CAN hardware popups.
3. Left pane: locale row with flags, units, and the Hardware Capabilities swap.
4. Touch mode hides the menu bar; #212 becomes redundant.

Migrating all six popup editors at once makes it very hard to test against real hardware, which is
the only way several of these get verified.
