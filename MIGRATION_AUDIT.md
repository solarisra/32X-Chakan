# Migration audit — 2026-10-22

Most implemented content has been migrated. Hub progression feedback/routing has now been migrated; pause remains the main
missing state. Later-stage content is still stubbed in the source.

## Scope and baseline

Reviewed the original project at
`/home/christian/projects/Genesis/Chakan_Remake_v1`, including its checked-out
`master` (`5b6beab`) and the `create_level_stages` branch (`07e5f08`). The latter
is the appropriate gameplay baseline: the port already uses its player actions,
Fire 1, Air assets, collision definitions and Hub scenery. Comparing only master
would incorrectly classify several port assets/features as missing or changed.

The branch inventory contains 55 C files under `src`, 17 headers under `inc`
(additional level headers live under `src`), and 304 resource files. Its active
resource manifests declare 129 resources referencing 83 unique files. Every
active manifest path is listed in [MIGRATION_ASSETS.csv](MIGRATION_ASSETS.csv),
with source call sites, copied paths and file comparison results. Commented-out
resources, archives, editor files and generated SGDK headers are not treated as
runtime migration requirements.

This is a static source/resource parity review, informed by the existing host
and PicoDrive validation. It is not an exhaustive new playthrough or a visual
inspection of every archived art file. No gameplay changes were made for this
audit.

## Confirmed remaining differences

| Priority | Area | What remains |
| --- | --- | --- |
| Medium | Pause | The source has a pause window and item-selection music. The port retains `STATE_PAUSE` in its enum but implements no pause update, rendering, or input path. The pause bitmap and music track 04 are absent. |

Evidence in the port:

- [State dispatch](sh_src/src/gamestate.c): only Logo, Intro, Menu and Play are handled.
- [Level IDs](sh_src/inc/level_manager.h): Hub plus four terrestrial stage-1 IDs.
- [Level manager](sh_src/src/level_manager.c): transitions and return-to-Hub, with persistent per-game completion bits.
- [Hub interactions](sh_src/src/levels/hub/portal_hub.c): door and sprite destinations select the next incomplete stage, with placeholder guards.
- [Hub sprites](sh_src/src/levels/hub/hub_sprites.c): portals, flames, planets, potions, hourglasses player and a composite containing all conditional completion markers.
- [Hub converter](sh_src/tools/convert_portal_hub.py): reads the local PAL0 image directly.
- [Camera](sh_src/src/camera.c): full controls and renderer-adapted scroll helpers, preserving the regional lock.

Source branch references for comparison:

- `src/levels/hub/portal_hub.c:146–299`: progression, portal access, final door and interaction.
- `src/levels/hub/hub_sprites.c:399` onward: conditional completion markers.
- `src/level_manager.c:15–22,319` onward: completion flags and stage selection helpers.
- `src/gamestate.c:processStatePause`: pause window and track 04.
- `src/input_controls.c:joyHandlerStatePause`: source pause input is itself incomplete; Start assigns `STATE_PAUSE` again rather than resuming. A proper pause migration should fix that behavior rather than reproduce it.

Progression logic exists in the source, but no assignment setting stage completion
to TRUE was found in its gameplay code. The routing/marker machinery is now ported, while a complete playable progression
loop still needs stage-exit triggers and later-stage content.

## Content already covered

| Area | Status |
| --- | --- |
| Logo | Pixels, palette cycles, scream, timing and fades migrated; centering and invalid initial color phases intentionally corrected. |
| Five-scene intro | All story art/text, sprite animation, lightning recording/palette flashes, fades, scrolling and sand cycling migrated. Prepared-screen compositor is verified at 60 presentations/s in all five scene windows in PicoDrive. |
| Menu | Lightning, title, eye animation, splash/main/options screens, cursor, intro music and Start Game migrated. The full recording is preserved; letter reveals are advanced to follow its embedded impacts. |
| Options | Source behavior is preserved: Back works; the second selection is unused. There are no functioning settings in the source to migrate. |
| Hub | Terrain, starfield, eight-tick palette cycle, regional camera lock, planets, flames, terrestrial portals, potions/hourglasses and stage-1 entrance interactions are covered. Completion feedback/routing and markers are now migrated. |
| Four terrestrial stage-1 levels | Earth, Fire, Water and Air maps, music, collision and implemented rendering effects are covered. Fire 1 comes from `create_level_stages`; its master counterpart is empty. |
| Player | Standing/walking/jump/fall/landing, crouch/roll, short collider/headroom, drop-through, variable jump height, coyote time and jump buffering are present. |
| Collision | Geometry/heightmaps, one-way platforms, bounds resolution and door behavior queries are implemented. Hardware-specific streaming/window allocation was adapted rather than copied literally. |
| Tests | TEST_WIDE is on SH-2; TEST_SMALL remains a selectable Genesis/68000 backend. These are alternate test backends, not missing story stages. |
| Active SFX | All three declared source effects—scream, intro lightning and menu lightning—are present. Title impacts remain embedded in the unchanged menu recording. |
| Active stage music | Intro/menu, Hub and all four stage-1 songs are included. Missing track 04 belongs to pause. |

## Source stubs and unused data, not lost finished gameplay

- All eight terrestrial stage-2/stage-3 files on `create_level_stages` are
  18-line placeholders: loading returns its input and processing is empty.
  Elemental `.c` files are empty; FINAL_LEVEL is routing/enum scaffolding, not
  a finished level. Their absence from the port does not omit playable maps.
- The generic entity module is not ported. Its enemy creation immediately
  clears the active flag, and entity updates are disabled in the main stage
  loops. TEST_WIDE does call its basic player-entity synchronization/physics;
  that experimental entity path is not reproduced by the port.
- `player_health` defaults to 12 in the source, but no working health/damage
  system or HUD was found. Hazard/switch/trigger behavior enums are not evidence
  of a complete combat, pickup or scripted-event system. Source potions and
  hourglasses are scenery; there is no finished inventory to migrate.
- The source's legacy falling/death path requests a hard reset when a falling
  flag is set. No active gameplay assignment enabling that flag was found.
  The port instead has level respawn/bounds handling; this is not full death
  behavior parity, but the source path is dormant.
- Eight declared music tracks have no active source C references: 03, 06, 08,
  10, 12 and 13–15. They remain absent as unused material, separate from the
  genuinely missing pause track 04. Jump SFX calls are commented out and have
  no active resource declaration.
- Menu PAL1/PAL2 `.pal` declarations have no active C uses. The converted menu
  uses the embedded image palettes and explicit runtime colors; their absent
  standalone copies do not establish missing menu behavior.
- The branch's logo path moved under `menus/`; the port keeps the established
  root-level logo paths. The inventory treats these as relocated assets.
- Chakan's PNG bytes differ, but decoded indexed pixels and Genesis palette
  match exactly. This is encoding drift, not missing animation art.

## Verification and maintenance follow-up

1. Choose the desired pause/resume behavior and migrate the pause screen/music.
2. Add actual stage-exit triggers when their design/content is ready; the
   completion API and placeholder guards are implemented.
3. Hub PAL0 parity is restored to the gameplay branch (see follow-up below).
4. Keep `create_level_stages` pinned as the gameplay source baseline and record
   intentional fixes separately from omissions.
5. Update old smoke-test startup assumptions: the portal harness still reserves
   only 420 startup VBlanks and does not navigate Intro/Menu before placing a
   player. Its old portal scripts cannot validate the new default startup flow
   unchanged. Older sections of PORTING.md also contain historical statements
   that menus/SFX are pending; the current state sections supersede them.
6. Extend performance work only where needed: the intro's measured 60 fps does
   not establish 60 fps for the logo, menu or every gameplay level. PAL timing,
   long play sessions and real hardware still need separate verification.

## Progression follow-up completed

The port now includes all 24 stage flags, derived group/realm completion, source
entrance routing, hidden completed portals, final-door selection, and all 24
conditional completion markers. `level_manager_complete_current()` records a
stage exit and returns to Hub through the normal fade. Later-stage runtimes
remain unavailable, and no stage-exit region has been invented. Save persistence
is not implemented in the source or this change.

## Camera follow-up completed

Camera controls and scroll-helper coverage have now been ported. Lookahead is
velocity-aware in every SH-2 level but remains disabled by default. The original
follow deadzone and Hub lock remain intact. SGDK Map/VDP helpers target GdlFrame
and explicit background dimensions; shake is clamped to software-renderer bounds.

## Hub palette follow-up completed

PAL0 entry 2 has been restored byte-for-byte from `create_level_stages` 07e5f08
(`0x08C`). PAL1 and starfield palettes already match that branch. The checked-out
master and the foreground image's embedded palette use `0x0AE` instead; the
explicit runtime palette takes precedence, as in SGDK. Tests pin all 32 runtime
foreground palette entries rather than assuming the image palette is authoritative.
