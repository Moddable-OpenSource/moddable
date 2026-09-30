# FreeType in the Moddable SDK

FreeType **2.14.3**, vendored from <https://gitlab.freedesktop.org/freetype/freetype> (tag `VER-2-14-3`). Used by the Commodetto outline drawing module and the FreeType font engine. FreeType is distributed under the FreeType License (BSD-style with credit clause) or GPLv2; see `docs/LICENSE.TXT` in the upstream distribution.

## What is here

Only the module directories the SDK builds are vendored: `base`, `smooth`, `sfnt`, `truetype`, `cff`, `psaux`, `pshinter`, `psnames`, `raster`, and `autofit`, plus the complete `include` tree. Omitted upstream modules include `type1`, `type42`, `cid`, `bdf`, `pcf`, `pfr`, `winfonts`, `sdf`, `svg`, `cache`, the validators, and the compression modules.

Which of these files are actually compiled is decided by the manifests, not by what is present on disk. `manifest.json` builds only the rasterizer, stroker, and supporting math — what the outline module needs.

## Local changes

Keep this list current. Everything below is marked with a `MODDABLE:` comment at the change site, so `grep -rn MODDABLE: src include` finds them all when moving to a new release.

| File | Change |
| --- | --- |
| `include/ft2build.h` | Defines `FT2_BUILD_LIBRARY`. FreeType expects it on the command line, but a manifest's `C_FLAGS` is overwritten by the platform makefile, and there is no portable way to pass a define to only these sources. |
| `include/freetype/config/ftoption.h` | `FT_RENDER_POOL_SIZE` is set by `MODDEF_FT_RENDER_POOL_SIZE`, which a manifest sets as `ft.render_pool_size`. The default is upstream's 16384, so nothing changes unless a project asks. The pool is statically allocated, and an outline whose cells do not fit is rendered in bands that each walk it again: on an ESP32-S3 drawing `piu/outline/shapes`, 8192 costs about 25% more CPU than 16384, and 32768 saves about 12% more. It is compared with `#if` in `ftgrays.c`, so the value must be an integer constant expression. |
| `src/smooth/ftgrays.c` | The rasterizer cell pool is static rather than on the stack, which is far too much stack for a microcontroller. Uses IRAM on the original ESP32 and a one-time allocation on nrf52. |
| `src/base/ftoutln.c` | `FT_Outline_Render` and `FT_Outline_Get_Bitmap` are behind `MODDABLE_FT_OUTLINE_RENDER`. They are the only functions in the file that reference the renderer layer, and nothing in the SDK calls them: both the outline module and the font engine drive `ft_grays_raster` directly. Omitting them keeps the object layer out of builds that only draw outlines. |
| `src/psaux/psintrp.c` | `CF2_GlyphPathRec`, which holds three hint maps and is about 16K, comes from the heap rather than the stack. Upstream makes it a local of `cf2_interpT2CharString`, which is more stack than a microcontroller task has: rendering any OpenType/CFF glyph overflowed. One allocation per glyph is nothing beside rasterizing one. |
| `src/autofit/afmodule.c`, `aflatin.c`, `afcjk.c` | `AF_GlyphHintsRec`, which embeds its point, contour, segment, and edge arrays and is about 6.7K, comes from the heap rather than the stack. Upstream makes it a local of `af_autofitter_load_glyph`, and again of `af_latin_metrics_init_widths` and `af_cjk_metrics_init_widths`, which run beneath it for the first glyph at a new size -- two at once, more stack than a microcontroller task has. If the allocation in `init_widths` fails, the fallback stem widths are used, as when a font lacks its standard character. |
| `src/base/ftstroke.c` | `FT_Glyph_Stroke` and `FT_Glyph_StrokeBorder` are behind `MODDABLE_FT_GLYPH`, so stroking does not pull in `ftglyph.c`. |
| `include/freetype/config/ftmodule.h` | Replaced. Upstream registers every module FreeType ships; this registers only the ones the SDK builds, selected by the same defines that choose the source files. |
| `include/freetype/config/ftoption.h` | A Moddable block at the end overrides the upstream defaults: no filesystem, no compression libraries, no glyph names, and hinting and variable font support off unless the manifest asks for them. |
| `include/freetype/config/ftoption.h` | `TT_CONFIG_OPTION_GPOS_KERNING` follows `cfe.kern`, and can be set on its own with `cfe.ft.gposKern`. Without it only the legacy `kern` table is read, so a font that kerns entirely from GPOS -- which most OpenType and nearly all CFF fonts do -- kerns not at all, and does so silently. `ttgpos.c` is compiled either way as part of the `sfnt` amalgamation; the option only decides whether it does anything, and costs about 3K of flash. FreeType's implementation reads pair values whose `valueFormat1` is exactly `0x4`, so a variable font that carries kerning deltas (`0x44`) is still skipped -- that needs a shaper. The GPOS table is read in place from a memory stream, so it costs no RAM. Reading `cfe.kern` here means reading a manifest boolean, which arrives as the word `true`: the same block gives `true` and `false` values for the few lines that need them, where the language has not already, and undefines them again. |
| `moddable/` | Not upstream. `ftsystemModdable.c` stands in for `ftsystem.c`: fonts are always opened from memory, so the only piece needed is a stub for `FT_Stream_Open`. |

## Notes for the next update

The manifests compile FreeType's single-object files -- `psaux.c`, `sfnt.c`,
`truetype.c`, `cff.c` -- each of which `#include`s the other sources of its
module. The generated makefile does not `-include` its dependency files, so
editing one of those included sources rebuilds nothing. Touch the file that
includes it, or delete the object, or the change silently will not be in the
build.

`FT_Pos` is the size of a `long`: 4 bytes on a microcontroller, 8 on a 64-bit host. Code that stores `FT_Vector` in its own buffers must size them with `sizeof(FT_Pos)` rather than assuming either. See `PocoOutlineByteLength` in `../outline/commodettoPocoOutline.h`.
