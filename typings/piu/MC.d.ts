/*
 * Copyright (c) 2020-2026 Shinya Ishikawa
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/// <reference path="../easing.d.ts" />

declare module "piu/MC" {
  import { } from "commodetto/Poco";   // for global screen
  import type * as MC from "piu/MC-types";

  global {
    const Skin: MC.SkinConstructor
    const Texture: MC.TextureConstructor
    const Style: MC.StyleConstructor
    const Behavior: MC.BehaviorConstructor
    const Content: MC.ContentConstructor
    const Container: MC.ContainerConstructor
    const Application: MC.ApplicationConstructor
    const Scroller: MC.ScrollerConstructor
    const Row: MC.RowConstructor
    const Column: MC.ColumnConstructor
    const Layout: MC.LayoutConstructor
    const Image: MC.ImageConstructor
    const Die: MC.DieConstructor
    const Port: MC.PortConstructor
    const Label: MC.LabelConstructor
    const Transition: MC.TransitionConstructor
    const Text: MC.TextConstructor
    const Link: MC.LinkConstructor
    const application: MC.Application
    const Locals: MC.Locals
    const blendColors: MC.blendColors
    const hsl: MC.hsl
    const hsla: MC.hsla
    const rgb: MC.rgb
    const rgba: MC.rgba
  }

  export * from "piu/MC-types";
}
