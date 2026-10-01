/*
 * Copyright (c) 2020-2025 Shinya Ishikawa
 * Copyright (c) 2026  Moddable Tech, Inc.
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
    (): InstanceType<C>;
    template: TemplateStyleFactory<C>;
  };
export type TemplateStyleFactory<T extends new (...args: any) => any> = (
    param: ConstructorParameters<T>[0]
  ) => TemplateStyle<Partial<ConstructorParameters<T>[1]>, T>;

export type TemplateComponent<P, C extends new (...args: any) => any> = {
    new (behaviorData?: any, dictionary?: P): InstanceType<C>;
    (behaviorData?: any, dictionary?: P): InstanceType<C>;
    template: TemplateComponentFactory<C>;
  };
export type TemplateComponentFactory<T extends new (...args: any) => any> = (
    func: (param: any) => ConstructorParameters<T>[1]
  ) => TemplateComponent<Parameters<typeof func>[0], T>;

/*
    Templates

    Content.template, Container.template, ... return a constructor that may be called
    with or without new.

    The instantiating data is untyped unless its type is given explicitly:

        const Item = Container.template<ItemData>($ => ({ ... }));

    Then $ has that type in the function, and the data is checked where Item is instantiated.
    It may be omitted there only when its type allows undefined.
*/
export type TemplateArguments<D, $> = undefined extends $ ? [behaviorData?: $, dictionary?: D] : [behaviorData: $, dictionary?: D];
export interface Template<C, D, $ = any> {
    new(...args: TemplateArguments<D, $>): C;
    (...args: TemplateArguments<D, $>): C;
    readonly prototype: C;

    template<$2 = $>(fn: (this: C, data: NoInfer<$2>, dictionary?: D) => D): Template<C, D, $2>;
  }

/*
    Skin.template, Style.template, and Texture.template return a function that creates
    the asset on first use. It is the value of the Skin, Style, and Texture properties
    of dictionaries.
*/
export type AssetTemplate<T> = {
    new(): T;
    (): T;
  };

/*
    Behavior

    The events Piu triggers are declared here. A behavior may implement any other
    method to receive events triggered by bubble, defer, delegate, distribute,
    firstThat, and lastThat.
*/
export interface Behavior<T extends Content = Content> {
    onCreate?(content: T, data: any, context: any): void;
    onDisplaying?(content: T): void;
    onFinished?(content: T): void;
    onTimeChanged?(content: T): void;
    onTouchBegan?(content: T, id: number, x: number, y: number, ticks: number): void;
    onTouchCancelled?(content: T, id: number): void;
    onTouchEnded?(content: T, id: number, x: number, y: number, ticks: number): void;
    onTouchMoved?(content: T, id: number, x: number, y: number, ticks: number): void;
    /*
        The events below were added after these typings were first published. They are declared
        loosely so that a behavior which already has a method of the same name, with whatever
        parameters, still type-checks. Piu calls them as:

            onDraw(port, x, y, width, height)
            onFitHorizontally(layout, width), onFitVertically(layout, height)                   layouts
            onMeasureHorizontally(content, width), onMeasureVertically(content, height)         layouts and ports
            onScrolled(content)             on a scroller and on each of its contents, each with its own content
            onTransitionBeginning(container), onTransitionEnded(container)
            onAdapt(layout)
            onFocused(application), onUnfocused(application)
            onKeyDown(application, key), onKeyUp(application, key)
            onMessage(application, message), onQuit(application)
    */
    onDraw?(...args: any[]): any;
    onFitHorizontally?(...args: any[]): any;
    onFitVertically?(...args: any[]): any;
    onMeasureHorizontally?(...args: any[]): any;
    onMeasureVertically?(...args: any[]): any;
    onScrolled?(...args: any[]): any;
    onTransitionBeginning?(...args: any[]): any;
    onTransitionEnded?(...args: any[]): any;
    onAdapt?(...args: any[]): any;
    onFocused?(...args: any[]): any;
    onUnfocused?(...args: any[]): any;
    onKeyDown?(...args: any[]): any;
    onKeyUp?(...args: any[]): any;
    onMessage?(...args: any[]): any;
    onQuit?(...args: any[]): any;
  }
export interface BehaviorConstructor<T extends Content = Content> {
    new(): Behavior<T>;
  }
// the type of the Behavior global: class X extends Behavior, or class X extends Behavior<Label>
export interface BehaviorClass {
    new<C extends Content = Content>(): Behavior<C>;
    readonly prototype: Behavior<any>;
  }

export interface Content {
    // TODO: consider type parameter to avoid any
    adjust(x: number, y: number): void;
    bubble(id: string, ...extras: any[]): any;
    captureTouch(id: number | string, x: number, y: number, ticks: number): void;
    defer(id: string, ...extras: any[]): void;
    delegate(id: string, ...extras: any[]): any;
    distribute(id: string, ...extras: any[]): void;
    focus(): void;
    hit(x: number, y: number): Content | undefined;
    measure(): Required<Size>;
    moveBy(x: number, y: number): void;
    render(): void;
    sizeBy(width: number, height: number): void;
    start(): void;
    stop(): void;

    readonly previous: Content | undefined;
    readonly next: Content | undefined;
    readonly container: Container | undefined;
    readonly application: Application | undefined;
    readonly index: number;
    readonly running: boolean;
    readonly type: string;
    name: string;
    active: boolean;
    /** @deprecated anchor is a property of dictionaries, not of contents */
    anchor: string;
    get behavior(): object | undefined;
    set behavior(it: object | null | undefined);
    coordinates: Coordinates & Size;
    get bounds(): Required<Bounds>;
    set bounds(it: Bounds);
    backgroundTouch: boolean;
    exclusiveTouch: boolean;
    multipleTouch: boolean;

    time: number;
    duration: number;
    fraction: number;
    interval: number;
    loop: boolean;
    get offset(): undefined | Required<Position>;
    set offset(it: undefined | Position);
    get position(): undefined | Required<Position>;
    set position(it: undefined | Position);
    get size(): Required<Size>;
    set size(it: Size);
    state: number;
    variant: number;
    get skin(): Skin | undefined;
    set skin(it: Skin | null | undefined);
    get style(): Style | undefined;
    set style(it: Style | null | undefined);
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
    anchor?: PropertyKey;
    Behavior?: new () => object;
    behavior?: object;
    skin?: Skin | SkinDictionary;
    Skin?: SkinConstructor | AssetTemplate<Skin>;
    style?: Style | StyleDictionary;
    Style?: StyleConstructor | AssetTemplate<Style>;
    visible?: boolean;
  }
export interface ContentConstructor extends Template<Content, ContentDictionary> {
    // as published: the function may return undefined, and the dictionary of the result is unchecked
    template<$2 = any>(fn: (this: Content, data: NoInfer<$2>, dictionary?: ContentDictionary) => ContentDictionary | undefined): Template<Content, any, $2>;
  }

export interface Style {
    measure(string: string): Required<Size>;

    // each is undefined when the style does not specify it
    readonly bottom: number | undefined;
    readonly color: Color[] | undefined;
    readonly family: string | undefined;
    readonly horizontal: string | undefined;
    readonly indentation: number | undefined;
    readonly leading: number | undefined;
    readonly left: number | undefined;
    readonly margins: Coordinates;
    readonly right: number | undefined;
    readonly size: string | undefined;				// "18px", "120%", "smaller", ...
    readonly stretch: string | undefined;
    readonly style: string | undefined;
    readonly top: number | undefined;
    readonly vertical: string | undefined;
    readonly weight: number | string | undefined;	// 100 to 900, "lighter", "bolder"
  }
export interface StyleDictionaryBase {
    archive?: ArrayBufferLike;
    color?: Color | Color[];
    decoration?: string;
    font?: string;
    horizontal?: string;
    indentation?: number;
    margins?: Coordinates;
    top?: number;
  }
export interface StyleConstructor {
    new(dictionary?: StyleDictionary): Style;
    readonly prototype: Style;

    template<T>(this: T, dictionary: StyleDictionary): T & AssetTemplate<Style>;
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
    archive?: ArrayBufferLike;
  }
export interface TextureAlphaColorDictionary {
    alpha: string;
    color: string;
    archive?: ArrayBufferLike;
  }
export interface TextureConstructor {
    new(path: string): Texture;
    new(dictionary: TextureAlphaColorDictionary): Texture;
    new(dictionary: TextureDictionary): Texture;
    readonly prototype: Texture;

    template(path: string | TextureDictionary | TextureAlphaColorDictionary): TemplateStyle<Partial<undefined>, TextureConstructor>;
  }

export interface CLUT extends HostBuffer {
    readonly colors: number[];
  }
export interface CLUTConstructor {
    new(path: string): CLUT;
    readonly prototype: CLUT;
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
      x?: number;
    y?: number;
  }
export interface SkinConstructor {
    new(dictionary: SkinDictionary): Skin;
    readonly prototype: Skin;

    template: TemplateStyleFactory<SkinConstructor>;
  }

export interface TextureSkinDictionary extends Coordinates, Bounds {
    texture?: Texture | TextureDictionary | TextureAlphaColorDictionary;
    Texture?: TextureConstructor | AssetTemplate<Texture>;
    color?: Color | Color[];
    states?: number;
    variants?: number;
    tiles?: Coordinates;
  }
// left, right, top, and bottom are the borders when there is no borders property
export interface ColorSkinDictionary extends Coordinates {
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
    new(duration: number, interpolator?: ((value: number) => number) | null, first?: any, last?: any): Transition
    readonly prototype: Transition;
  }

export interface Container extends Content {
    clip: boolean;
    readonly first: Content | undefined;
    readonly last: Content | undefined;
    readonly length: number;
    readonly transitioning: boolean;
    add(content: Content): void;
    content(at: number | string): Content | undefined;
    empty(start?: number, stop?: number): void;
    firstThat(id: string, ...extras: any[]): any;
    insert(content: Content, before: Content): void;
    lastThat(id: string, ...extras: any[]): any;
    remove(content: Content): void;
    replace(content: Content, by: Content): void;
    // a falsy transition completes the transition that is running
    run(transition: Transition | null, ...extras: any[]): void;
    swap(content0: Content, content1: Content): void;
    /** @deprecated an event of Behavior, not a function of containers */
    onTransitionBeginning(container: Container): void;
    /** @deprecated an event of Behavior, not a function of containers */
    onTransitionEnded(container: Container): void;
  }
// falsy values are skipped, so cond && Content(...) is allowed
export type Contents = Content | null | undefined | false | 0 | "" | Contents[];
export interface ContainerDictionary extends ContentDictionary {
    clip?: boolean;
    contents?: Contents[];
  }
export interface ContainerConstructor extends Template<Container, ContainerDictionary> {
  }

export interface Label extends Content {
    string: string;
  }
export interface LabelDictionary extends ContentDictionary {
    ellipsis?: boolean;
    string?: string;
  }
export interface LabelConstructor extends Template<Label, LabelDictionary> {
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
      height: number,
      ellipsis?: boolean
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
      width?: number,
      height?: number
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
    measureString(string: string, style: Style): Required<Size>;
    popClip(): void;
    pushClip(
      x?: number,
      y?: number,
      width?: number,
      height?: number
    ): void;
    /** @deprecated an event of Behavior, not a function of ports */
    onDraw(
      port: Port,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
  }
export interface PortConstructor extends Template<Port, ContentDictionary> {
    // as published: the function may return undefined, and the dictionary of the result is unchecked
    template<$2 = any>(fn: (this: Port, data: NoInfer<$2>, dictionary?: ContentDictionary) => ContentDictionary | undefined): Template<Port, any, $2>;
  }

export interface TextBlock {
      /** @deprecated not read by Piu: use link */
      behavior?: object | null;
      style?: Style | null;
      link?: Link | null;
      spans: (string | TextSpan)[] | string,
  }

export interface TextSpan extends TextBlock {
      link?: any
  }

export interface Text extends Content {
  // a string in blocks is a block of one span in the style of the text
  get blocks(): TextBlock[];
  set blocks(it: (TextBlock | string)[]);
  string: string;
  begin(): void;
  // link, when present, receives the touch events of the block or span
  beginBlock(style?: Style | null, link?: Link | null): void;
  beginSpan(style?: Style | null, link?: Link | null): void;
  concat(string: string): void;
  end(): void;
  endBlock(): void;
  endSpan(): void;
}
export interface TextDictionary extends ContentDictionary {
  blocks?: (TextBlock | string)[],
  string?: string;
}
export interface TextConstructor extends Template<Text, TextDictionary> {
}

export interface Application extends Container {
  displayListLength: number;
  commandListLength: number;
  touchCount: number;
  // set with the data of a color table (a CLUT or a Resource); reads back as the 16 colors in use
  get clut(): HostBuffer | undefined;
  set clut(it: HostBuffer);
  rotation: number | undefined;
  animateColors(colors: Color[]): void;
  keyDown(key: any): void;
  keyUp(key: any): void;
  postMessage(json: any): void;
  purge(): void;
}
export interface ApplicationDictionary extends ContainerDictionary {
  displayListLength?: number;
  commandListLength?: number;
  touchCount?: number;
  regionLength?: number;
  pixels?: number;
}
export interface ApplicationConstructor extends Template<Application, ApplicationDictionary> {
}

export interface Column extends Container {
}
export interface ColumnConstructor extends Template<Column, ContainerDictionary> {
}

export interface Layout extends Container {
  /** @deprecated an event of Behavior, not a function of layouts */
  onFitHorizontally(layout: Layout, width: number): void;
  /** @deprecated an event of Behavior, not a function of layouts */
  onFitVertically(layout: Layout, height: number): void;
  /** @deprecated an event of Behavior, not a function of layouts */
  onMeasureHorizontally(layout: Layout, width: number): void;
  /** @deprecated an event of Behavior, not a function of layouts */
  onMeasureVertically(layout: Layout, height: number): void;
}
export interface LayoutConstructor extends Template<Layout, ContainerDictionary> {
}

export interface Image extends Content {
  readonly frameCount: number;
  frameIndex: number;
}
export interface ImageDictionary extends ContentDictionary {
  path: string;
  archive?: ArrayBufferLike;
}
export interface ImageConstructor extends Template<Image, ImageDictionary> {
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
export interface DieDictionary extends ContainerDictionary {
  regionLength?: number;
}
export interface DieConstructor extends Template<Die, DieDictionary> {
}

export interface Row extends Container {
}
export interface RowConstructor extends Template<Row, ContainerDictionary> {
}

export interface Scroller extends Container {
  readonly constraint: Required<Position>;
  loop: boolean;
  looping: boolean;
  get scroll(): Required<Position>;
  set scroll(it: Position);
  tracking: boolean;
  reveal(bounds: Bounds, centerVertically?: boolean, centerHorizontally?: boolean): void;
  scrollBy(dx: number, dy: number): void;
  scrollTo(x: number, y: number): void;
  /** @deprecated an event of Behavior, not a function of scrollers */
  onScrolled(scroller: Scroller): void;
}
export interface ScrollerDictionary extends ContainerDictionary {
  loop?: boolean;
  looping?: boolean;
}
export interface ScrollerConstructor extends Template<Scroller, ScrollerDictionary> {
}

export interface LinkDictionary {
  anchor?: PropertyKey;
  Behavior?: new () => object;
  behavior?: object;
}
export interface Link {
  readonly container: Container | undefined;
  state: number;
  captureTouch(id: number, x: number, y: number, ticks: number): void;
}
export interface LinkConstructor extends Template<Link, LinkDictionary> {
}

export interface Locals {
  new(name?: string, language?: string): Locals
  language: string;
  get(textToLocalize: string): string
}

export interface blendColors {
  (a: number, c1: Color, c2: Color): Extract<Color, number>
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
