/*
 * Copyright (c) 2020-2025 Shinya Ishikawa
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

/// <reference path="./MC-types.d.ts" />

declare module "piu/MC" {
  import {} from "commodetto/Poco";   // for global screen

  global {
    const Skin: SkinConstructor
    const Texture: TextureConstructor
    const Style: StyleConstructor
    const Behavior: BehaviorConstructor
    const Content: ContentConstructor
    const Container: ContainerConstructor
    const Application: ApplicationConstructor
    const Scroller: ScrollerConstructor
    const Row: RowConstructor
    const Column: ColumnConstructor
    const Layout: LayoutConstructor
    const Image: ImageConstructor
    const Die: DieConstructor
    const Port: PortConstructor
    const Label: LabelConstructor
    const Transition: TransitionConstructor
    const Text: TextConstructor
    const Link: LinkConstructor
    const application: Application
    const Locals: Locals
    const blendColors: blendColors
    const hsl: hsl
    const hsla: hsla
    const rgb: rgb
    const rgba: rgba
  }

  export {
    Coordinates,
    Position,
    Size,
    Bounds,
    Color,
    ContentState,
    TimeProperty,
    TouchProperty,
    TemplateStyle,
    TemplateStyleFactory,
    TemplateComponent,
    TemplateComponentFactory,
    Behavior,
    BehaviorConstructor,
    Content,
    ContentDictionary,
    ContentConstructor,
    Style,
    StyleDictionaryBase,
    StyleConstructor,
    TextStyleDictionary,
    LabelStyleDictionary,
    StyleDictionary,
    Texture,
    TextureDictionary,
    TextureConstructor,
    Skin,
    SkinConstructor,
    TextureSkinDictionary,
    ColorSkinDictionary,
    SkinDictionary,
    Transition,
    TransitionConstructor,
    Container,
    ContainerDictionary,
    ContainerConstructor,
    Label,
    LabelDictionary,
    LabelConstructor,
    Port,
    PortConstructor,
    TextBlock,
    TextSpan,
    Text,
    TextDictionary,
    TextConstructor,
    Application,
    ApplicationDictionary,
    ApplicationConstructor,
    Column,
    ColumnConstructor,
    Layout,
    LayoutConstructor,
    Image,
    ImageDictionary,
    ImageConstructor,
    Die,
    DieConstructor,
    Row,
    RowConstructor,
    Scroller,
    ScrollerDictionary,
    ScrollerConstructor,
    LinkDictionary,
    Link,
    LinkConstructor,
    Locals,
    blendColors,
    hsl,
    hsla,
    rgb,
    rgba,
  }
}
