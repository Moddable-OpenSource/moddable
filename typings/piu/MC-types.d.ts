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

  /* type aliases */
export type Coordinates = {
    top?: number;
    right?: number;
    bottom?: number;
    left?: number;
  };
export type Position = {
    x?: number;
    y?: number;
  };
export type Size = {
    width?: number;
    height?: number;
  };
export type Bounds = Position & Size;
export type Color = string | number;
export type ContentState = {
    state?: number;
    variant?: number;
  };
export type TimeProperty = {
    time?: number;
    duration?: number;
    fraction?: number;
    interval?: number;
    loop?: boolean;
  };
export type TouchProperty = {
    active?: boolean;
    backgroundTouch?: boolean;
    exclusiveTouch?: boolean;
    multipleTouch?: boolean;
  };

  // TODO: more accurate constructor parameters
export type TemplateStyle<P, C extends new (...args: any) => any> = {
    new (dictionary?: P): InstanceType<C>;
    template: TemplateStyleFactory<C>;
  };
export type TemplateStyleFactory<T extends new (...args: any) => any> = (
    param: ConstructorParameters<T>[0]
  ) => TemplateStyle<Partial<ConstructorParameters<T>[1]>, T>;

export type TemplateComponent<P, C extends new (...args: any) => any> = {
    new (behaviorData?: any, dictionary?: P): InstanceType<C>;
    template: TemplateComponentFactory<C>;
  };
export type TemplateComponentFactory<T extends new (...args: any) => any> = (
    func: (param: any) => ConstructorParameters<T>[1]
  ) => TemplateComponent<Parameters<typeof func>[0], T>;

export interface Behavior<T extends Content = Content> {
    onCreate?(content: T, data: object, context: any): void;
    onDisplaying?(content: T): void;
    onFinished?(content: T): void;
    onTimeChanged?(content: T): void;
    onTouchBegan?(content: T, id: number, x: number, y: number, ticks: number): void;
    onTouchCancelled?(content: T, id: number): void;
    onTouchended?(content: T, id: number, x: number, y: number, ticks: number): void;
    onTouchMoved?(content: T, id: number, x: number, y: number, ticks: number): void;
    // TODO: complete callbacks
  }

export interface BehaviorConstructor<T extends Content = Content> {
    new(): Behavior<T>;
  }

export interface Content {
    // TODO: consider type parameter to avoid any
    adjust(x: number, y: number): void;
    bubble(id: string, ...extras: any[]): void;
    captureTouch(id: string, x: number, y: number, ticks: number): void;
    defer(id: string, ...extras: any[]): void;
    delegate(id: string, ...extras: any[]): void;
    distribute(id: string, ...extras: any[]): void;
    focus(): void;
    hit(x: number, y: number): Content | undefined;
    measure(): Size;
    moveBy(x: number, y: number): void;
    render(): void;
    sizeBy(width: number, height: number): void;
    start(): void;
    stop(): void;

    readonly previous: Content | null;
    readonly next: Content | null;
    readonly container: Container | null;
    readonly index: number;
    name: string;
    active: boolean;
    anchor: string;
    behavior: object;
    coordinates: Coordinates;
    bounds: Bounds;
    backgroundTouch: boolean;
    exclusiveTouch: boolean;
    multipleTouch: boolean;

    time: number;
    duration: number;
    fraction: number;
    interval: number;
    loop: boolean;
    running: boolean;
    offset: undefined | Position;
    position: undefined | Position;
    size: Size;
    state: number;
    variant: number;
    skin: Skin | null;
    style: Style | null;
    visible: boolean;
    x: number;
    y: number;
    width: number;
    height: number;
  }
export interface ContentDictionary
    extends Coordinates,
      Size,
      ContentState,
      TimeProperty,
      TouchProperty {
    name?: string;
    anchor?: string;
    Behavior?: BehaviorConstructor;
    behavior?: Behavior;
    skin?: Skin | SkinDictionary;
    Skin?: SkinConstructor;
    style?: Style | StyleDictionary;
    Style?: StyleConstructor;
    visible?: boolean;
  }
export interface ContentConstructor {
    new(behaviorData?: any, dictionary?: ContentDictionary): Content;
    (behaviorData?: any, dictionary?: ContentDictionary): Content;
    readonly prototype: Content;

    template: TemplateComponentFactory<ContentConstructor>;
  }

export interface Style {
    measure(string: string): Size;
  }
export interface StyleDictionaryBase {
    color?: Color | Color[];
    font?: string;
    horizontal?: string;
    top?: number;
  }
export interface StyleConstructor {
    new(dictionary: StyleDictionary): Style;

    template<T>(this: T, dictionary: StyleDictionary): T;
  }

export interface TextStyleDictionary extends StyleDictionaryBase {
    leading?: number;
    right?: number;
    bottom?: number;
    left?: number;
  }
export interface LabelStyleDictionary extends StyleDictionaryBase {
    vertical?: string;
  }
export type StyleDictionary = LabelStyleDictionary | TextStyleDictionary;

export interface Texture {
    readonly height: number;
    readonly width: number;
  }
export interface TextureDictionary {
    path: string;
  }
export interface TextureConstructor {
    new(path: string): Texture;
    new(dictionary: TextureDictionary): Texture;
    readonly prototype: Texture;

    template: TemplateStyleFactory<TextureConstructor>;
  }

export interface Skin {
    borders: Coordinates;
    fill: Color | Color[];
    stroke: Color | Color[];
    texture: Texture;
    color: Color;
    bounds: Bounds;
    height: number;
    width: number;
    states?: number;
    variants?: number;
    tiles?: Coordinates;
    top?: number;
    right?: number;
    bottom?: number;
    left?: number;
  }
export interface SkinConstructor {
    new(dictionary: SkinDictionary): Skin;
    readonly prototype: Skin;

    template: TemplateStyleFactory<SkinConstructor>;
  }

export interface TextureSkinDictionary extends Coordinates, Bounds {
    texture?: Texture | TextureDictionary;
    Texture?: TextureConstructor;
    color?: Color | Color[];
    states?: number;
    variants?: number;
    tiles?: Coordinates;
  }
export interface ColorSkinDictionary {
    borders?: Coordinates;
    fill?: Color | Color[];
    stroke?: Color | Color[];
  }
export type SkinDictionary = ColorSkinDictionary | TextureSkinDictionary;

export interface Transition {
    onBegin(container: Container, ...extras: any[]): void;
    onEnd(container: Container, ...extras: any[]): void;
    onStep(fraction: number): void;
    duration?: number;
  }
export interface TransitionConstructor {
    new(duration: number, interpolator?: (value: number) => number, first?: any, last?: any): Transition
  }

export interface Container extends Content {
    clip: boolean;
    readonly first: Content | null;
    readonly last: Content | null;
    readonly length: number;
    readonly transitioning: boolean;
    add(content: Content): void;
    content(at: number | string): Content | undefined;
    empty(start?: number, stop?: number): void;
    firstThat(id: string, ...extras: any[]): void;
    insert(content: Content, before: Content): void;
    lastThat(id: string, ...extras: any[]): void;
    remove(content: Content): void;
    replace(content: Content, by: Content): void;
    run(transition: Transition, ...extras: any[]): void;
    swap(content0: Content, content1: Content): void;
    onTransitionBeginning(container: Container): void;
    onTransitionEnded(container: Container): void;
  }
export interface ContainerDictionary extends ContentDictionary {
    clip?: boolean;
    contents?: Content[];
  }
export interface ContainerConstructor {
    new(behaviorData?: any, dictionary?: ContainerDictionary): Container;
    (behaviorData?: any, dictionary?: ContainerDictionary): Container;

    template<T>(this: T, fn: (arg: any) => ContainerDictionary): T;
  }

export interface Label extends Content {
    string: string;
  }
export interface LabelDictionary extends ContentDictionary {
    string?: string;
  }
export interface LabelConstructor {
    new(behaviorData?: any, dictionary?: LabelDictionary): Label;
    (behaviorData?: any, dictionary?: LabelDictionary): Label;

    template<T>(this: T, fn: (arg: any) => LabelDictionary): T;
  }

export interface Port extends Content {
    drawContent(
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    drawLabel(
      string: string,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    drawSkin(
      skin: Skin,
      x: number,
      y: number,
      width: number,
      height: number,
      variant?: number,
      state?: number
    ): void;
    drawString(
      string: string,
      style: Style,
      color: Color,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    drawStyle(
      string: string,
      style: Style,
      x: number,
      y: number,
      width: number,
      height: number,
      ellipsis?: boolean,
      state?: number
    ): void;
    drawTexture(
      texture: Texture,
      color: Color,
      x: number,
      y: number,
      sx: number,
      sy: number,
      sw: number,
      sh: number
    ): void;
    fillColor(
      color: Color,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    fillTexture(
      texture: Texture,
      color: Color,
      x: number,
      y: number,
      width: number,
      height: number,
      sx?: number,
      sy?: number,
      sw?: number,
      sh?: number
    ): void;
    invalidate(
      x?: number,
      y?: number,
      width?: number,
      height?: number
    ): void;
    measureString(string: string, style: Style): Size;
    popClip(): void;
    pushClip(
      x?: number,
      y?: number,
      width?: number,
      height?: number
    ): void;
    onDraw(
      port: Port,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
  }
export interface PortConstructor extends ContentConstructor {
  }

export interface TextBlock {
      behavior?: object | null;
      style?: Style | null;
      spans: (string | TextSpan)[] | string,
  }

export interface TextSpan extends TextBlock {
      link?: any
  }

export interface Text extends Content {
  blocks: TextBlock[],
  string: string;
  begin(): void;
  beginBlock(style?: Style, behavior?: object): void;
  beginSpan(style: Style, behavior?: object): void;
  concat(string: string): void;
  end(): void;
  endBlock(): void;
  endSpan(): void;
}
export interface TextDictionary extends ContentDictionary {
  blocks?: TextBlock[],
  string?: string;
}
export interface TextConstructor {
  new(behaviorData?: any, dictionary?: TextDictionary): Text;
  (behaviorData?: any, dictionary?: TextDictionary): Text;

  template<T>(this: T, fn: (arg: any) => TextDictionary): T;
}

export interface Application extends Container {
  displayListLength: number;
  commandListLength: number;
  touchCount: number;
  purge(): void;
}
export interface ApplicationDictionary extends ContainerDictionary {
  displayListLength?: number;
  commandListLength?: number;
  touchCount?: number;
  pixels?: number;
}
export interface ApplicationConstructor {
  new(behaviorData?: any, dictionary?: ApplicationDictionary): Application;

  template<T>(this: T, fn: (arg: any) => ApplicationDictionary): T;
}

export interface Column extends Container {
}
export interface ColumnConstructor extends ContainerConstructor {
}

export interface Layout extends Container {
  onFitHorizontally(layout: Layout, width: number): void;
  onFitVertically(layout: Layout, height: number): void;
  onMeasureHorizontally(layout: Layout, width: number): void;
  onMeasureVertically(layout: Layout, height: number): void;
}
export interface LayoutConstructor extends ContainerConstructor {
}

export interface Image extends Content {
  readonly frameCount: never;
  frameIndex: number;
}
export interface ImageDictionary extends ContentDictionary {
  path: string;
}
export interface ImageConstructor {
  new(behaviorData?: any, dictionary?: ImageDictionary): Image;
  (behaviorData?: any, dictionary?: ImageDictionary): Image;

  template<T>(this: T, fn: (arg: any) => ImageDictionary): T;
}

export interface Die extends Layout {
  set(x: number, y: number, width: number, height: number): Die;
  sub(x: number, y: number, width: number, height: number): Die;
  and(x: number, y: number, width: number, height: number): Die;
  or(x: number, y: number, width: number, height: number): Die;
  xor(x: number, y: number, width: number, height: number): Die;
  fill(): Die;
  empty(): Die;
  cut(): void;
  attach(content: Content): void;
  detach(): void;
}
export interface DieConstructor extends LayoutConstructor {
}

export interface Row extends Container {
}
export interface RowConstructor extends ContainerConstructor {
}

export interface Scroller extends Container {
  readonly constraint: Position;
  loop: boolean;
  scroll: Position;
  tracking: boolean;
  reveal(bounds: Bounds): void;
  scrollBy(dx: number, dy: number): void;
  scrollTo(x: number, y: number): void;
  onScrolled(scroller: Scroller): void;
}
export interface ScrollerDictionary extends ContainerDictionary {
  loop?: boolean;
}
export interface ScrollerConstructor {
  new(behaviorData?: any, dictionary?: ScrollerDictionary): Scroller;
  (behaviorData?: any, dictionary?: ScrollerDictionary): Scroller;

  template<T>(this: T, fn: (arg: any) => ScrollerDictionary): T;
}

export interface LinkDictionary {
  Behavior?: BehaviorConstructor;
}
export interface Link {
  readonly container: Container;
  state: number;
  captureTouch(id: number, x: number, y: number, ticks: number): void;
}
export interface LinkConstructor {
  new(behaviorData?: any, dictionary?: LinkDictionary): Link;
  (behaviorData?: any, dictionary?: LinkDictionary): Link;

  template<T>(this: T, fn: (arg: any) => LinkDictionary): T;
}

export interface Locals {
  new(name?: string, language?: string): Locals
  language: string;
  get(textToLocalize: string): string
}

export interface blendColors {
  (a: number, c1: number, c2: number): Extract<Color, number>
}
export interface hsl {
  (h: number, s: number, l: number): Extract<Color, number>
}
export interface hsla {
  (h: number, s: number, l: number, a: number): Extract<Color, number>
}
export interface rgb {
  (r: number, g: number, b: number): Extract<Color, number>
}
export interface rgba {
  (r: number, g: number, b: number, a: number): Extract<Color, number>
}


export {};
