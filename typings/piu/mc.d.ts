/*
 * Copyright (c) 2020 Shinya Ishikawa
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

declare module "piu/mc" {
  /* type aliases */
  type Coordinates = {
    top?: number;
    right?: number;
    bottom?: number;
    left?: number;
  };
  type Position = {
    x?: number;
    y?: number;
  };
  type Size = {
    width?: number;
    height?: number;
  };
  type Bounds = Position & Size;
  type Color = string;
  type ContentState = {
    state?: number;
    variant?: number;
  };
  type TimeProperty = {
    time?: number;
    duration?: number;
    fraction?: number;
    interval?: number;
    loop?: boolean;
  };
  type TouchProperty = {
    active?: boolean;
    backgroundTouch?: boolean;
    exclusiveTouch?: boolean;
    multipleTouch?: boolean;
  };

  // TODO: more accurate constructor parameters
  type TemplateStyle<P, C extends new (...args: any) => any> = {
    new (dictionary?: P): InstanceType<C>;
    template: TemplateStyleFactory<C>;
  };
  type TemplateStyleFactory<T extends new (...args: any) => any> = (
    param: ConstructorParameters<T>[0]
  ) => TemplateStyle<Partial<ConstructorParameters<T>[1]>, T>;

  type TemplateComponent<P, C extends new (...args: any) => any> = {
    new (dictionary: P): InstanceType<C>;
    template: TemplateComponentFactory<C>;
  };
  type TemplateComponentFactory<T extends new (...args: any) => any> = (
    func: (param: any) => ConstructorParameters<T>[1]
  ) => TemplateComponent<Parameters<typeof func>[0], T>;

  class Behavior {}

  class Content {
    // TODO: consider type parameter to avoid any
    public constructor(behaviorData: any, dictionary: ContentDictionary);
    public adjust(x: number, y: number): void;
    public bubble(id: string, ...extras: any[]): void;
    public captureTouch(id: string, x: number, y: number, ticks: number): void;
    public defer(id: string, ...extras: any[]): void;
    public delegate(id: string, ...extras: any[]): void;
    public distribute(id: string, ...extras: any[]): void;
    public focus(): void;
    public hit(x: number, y: number): Content | undefined;
    public measure(): Size;
    public moveBy(x: number, y: number): void;
    public render(): void;
    public sizeBy(width: number, height: number): void;
    public start(): void;
    public stop(): void;
    public onCreate(content: Content, data: object, context: object): void;
    public onDisplaying(content: Content): void;
    public onFinished(content: Content): void;
    public onTimeChanged(content: Content): void;
    public onTouchBegan(
      content: Content,
      id: string,
      x: number,
      y: number,
      ticks: number
    ): void;
    public onTouchCancelled(content: Content, id: string): void;
    public onTouchended(
      content: Content,
      id: string,
      x: number,
      y: number,
      ticks: number
    ): void;
    public onTouchMoved(
      content: Content,
      id: string,
      x: number,
      y: number,
      ticks: number
    ): void;
    // TODO: Avoid any
    public static template: TemplateComponentFactory<typeof Content>;
    public static getAll<T>(this: { new (): T }): T[];
    public readonly previous: Content | null;
    public readonly next: Content | null;
    public readonly container: Container | null;
    public readonly index: number;
    public name: string;
    public active: boolean;
    public anchor: string;
    public behavior: object;
    public coordinates: Coordinates;
    public bounds: Bounds;
    public backgroundTouch: boolean;
    public exclusiveTouch: boolean;
    public multipleTouch: boolean;

    public time: number;
    public duration: number;
    public fraction: number;
    public interval: number;
    public loop: boolean;
    public running: boolean;
    public offset: undefined | Position;
    public position: undefined | Position;
    public size: Size;
    public state: number;
    public variant: number;
    public skin: Skin | null;
    public style: Style | null;
    public visible: boolean;
    public x: number;
    public y: number;
    public width: number;
    public height: number;
  }
  interface ContentDictionary
    extends Coordinates,
      Size,
      ContentState,
      TimeProperty,
      TouchProperty {
    name?: string;
    anchor?: string;
    Behavior?: new () => Behavior;
    skin?: Skin;
    Skin?: new () => Skin;
    style?: Style;
    Style?: new () => Style;
    visible?: boolean;
  }

  class Style {
    public constructor(dictionary: StyleDictionary);
    public measure(string: string): Size;
    public static template: TemplateStyleFactory<typeof Style>;
  }
  interface StyleDictionaryBase {
    color: Color | Color[];
    font: string;
    horizontal?: string;
    top?: number;
  }
  interface TextStyleDictionary extends StyleDictionaryBase {
    leading?: number;
    right?: number;
    bottom?: number;
    left?: number;
  }
  interface LabelStyleDictionary extends StyleDictionaryBase {
    vertical?: string;
  }
  type StyleDictionary = LabelStyleDictionary | TextStyleDictionary;

  class Texture {
    public constructor(path: string);
    public constructor(dictionary: TextureDictionary);
    public readonly height: number;
    public readonly width: number;
    public static template: TemplateStyleFactory<typeof Texture>;
  }
  interface TextureDictionary {
    path: string;
  }

  class Skin {
    public constructor(dictionary: SkinDictionary);
    public static template: TemplateStyleFactory<typeof Skin>;
    public borders: Coordinates;
    public fill: Color | Color[];
    public stroke: Color | Color[];
    public texture: Texture;
    public color: Color;
    public bounds: Bounds;
    public height: number;
    public width: number;
    public states?: number;
    public variants?: number;
    public tiles?: Coordinates;
    public top?: number;
    public right?: number;
    public bottom?: number;
    public left?: number;
  }
  interface TextureSkinDictionary extends Coordinates, Bounds {
    texture?: Texture;
    Texture?: new () => Texture;
    color?: Color;
    states?: number;
    variants?: number;
    tiles?: Coordinates;
  }
  interface ColorSkinDictionary {
    borders?: Coordinates;
    fill?: Color | Color[];
    stroke?: Color | Color[];
  }
  type SkinDictionary = ColorSkinDictionary | TextureSkinDictionary;

  class Transition {
    public constructor(duration: number);
    public onBegin(container: Container, ...extras: any[]): void;
    public onEnd(container: Container, ...extras: any[]): void;
    public onStep(fraction: number): void;
    public duration?: number;
  }

  class Container extends Content {
    public constructor(behaviorData: any, dictionary: ContainerDictionary);
    public clip: boolean;
    public readonly first: Content | null;
    public readonly last: Content | null;
    public readonly length: number;
    public readonly transitioning: boolean;
    public add(content: Content): void;
    public content(at: number | string): Content | undefined;
    public empty(start?: number, stop?: number): void;
    public firstThat(id: string, ...extras: any[]): void;
    public insert(content: Content, before: Content): void;
    public lastThat(id: string, ...extras: any[]): void;
    public remove(content: Content): void;
    public replace(content: Content, by: Content): void;
    public run(transition: Transition, ...extras: any[]): void;
    public swap(content0: Content, content1: Content): void;
    public onTransitionBeginning(container: Container): void;
    public onTransitionEnded(container: Container): void;
    public static template: TemplateComponentFactory<typeof Container>;
  }
  interface ContainerDictionary extends ContentDictionary {
    clip?: boolean;
    contents: Content[];
  }

  class Label extends Content {
    public constructor(behaviorData: any, dictionary: LabelDictionary);
    public string: string;
    public static template: TemplateComponentFactory<typeof Label>;
  }
  interface LabelDictionary extends ContentDictionary {
    string: string;
  }

  class Port extends Content {
    public drawContent(
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    public drawLabel(
      string: string,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    public drawSkin(
      skin: Skin,
      x: number,
      y: number,
      width: number,
      height: number,
      variant?: number,
      state?: number
    ): void;
    public drawString(
      string: string,
      style: Style,
      color: Color,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    public drawStyle(
      string: string,
      style: Style,
      x: number,
      y: number,
      width: number,
      height: number,
      ellipsis?: boolean,
      state?: number
    ): void;
    public drawTexture(
      texture: Texture,
      color: Color,
      x: number,
      y: number,
      sx: number,
      sy: number,
      sw: number,
      sh: number
    ): void;
    public fillColor(
      color: Color,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    public fillTexture(
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
    public invalidate(
      x?: number,
      y?: number,
      width?: number,
      height?: number
    ): void;
    public measureString(string: string, style: Style): Size;
    public popClip(): void;
    public pushClip(
      x?: number,
      y?: number,
      width?: number,
      height?: number
    ): void;
    public onDraw(
      port: Port,
      x: number,
      y: number,
      width: number,
      height: number
    ): void;
    public static template: TemplateComponentFactory<typeof Port>;
  }

  class Text extends Content {
    public constructor(begaviorData: any, dictionary: TextDictionary);
    public blocks: {
      behavior: object | null;
      style: Style | null;
      spans: string | string[];
    }[];
    public string: string;
    public begin(): void;
    public beginBlock(style?: Style, behavior?: object): void;
    public beginSpan(style: Style, behavior?: object): void;
    public concat(string: string): void;
    public end(): void;
    public endBlock(): void;
    public endSpan(): void;
    public static template: TemplateComponentFactory<typeof Text>;
  }
  interface TextDictionary extends ContentDictionary {
    blocks?: {
      behavior?: Behavior;
      style?: Style;
      spans: string | string[];
    }[];
    string: string;
  }

  class Application extends Container {
    public constructor(behaviorData: any, dictionary: ApplicationDictionary);
    public displayListLength: number;
    public commandListLength: number;
    public touchCount: number;
    public static template: TemplateComponentFactory<typeof Application>;
  }
  interface ApplicationDictionary extends ContainerDictionary {
    displayListLength?: number;
    commandListLength?: number;
    touchCount?: number;
  }

  class Column extends Container {
    public static template: TemplateComponentFactory<typeof Column>;
  }

  class Layout extends Container {
    public onFitHorizontally(layout: Layout, width: number): void;
    public onFitVertically(layout: Layout, height: number): void;
    public onMeasureHorizontally(layout: Layout, width: number): void;
    public onMeasureVertically(layout: Layout, height: number): void;
    public static template: TemplateComponentFactory<typeof Layout>;
  }

  class Image extends Content {
    public constructor(behaviorData: any, dictionary: ImageDictionary);
    public readonly frameCount: never;
    public frameIndex: number;
    public static template: TemplateComponentFactory<typeof Image>;
  }
  interface ImageDictionary extends ContentDictionary {
    path: string;
  }

  class Die extends Layout {
    public set(x: number, y: number, width: number, height: number): Die;
    public sub(x: number, y: number, width: number, height: number): Die;
    public and(x: number, y: number, width: number, height: number): Die;
    public or(x: number, y: number, width: number, height: number): Die;
    public xor(x: number, y: number, width: number, height: number): Die;
    public fill(): Die;
    public empty(): Die;
    public cut(): void;
    public attach(content: Content): void;
    public detach(): void;
    public static template: TemplateComponentFactory<typeof Die>;
  }

  class Row extends Container {
    public static template: TemplateComponentFactory<typeof Row>;
  }

  class Scroller extends Container {
    public constructor(behaviorData: any, dictionary: ScrollerDictionary);
    public readonly constraint: Position;
    public loop: boolean;
    public scroll: Position;
    public tracking: boolean;
    public reveal(bounds: Bounds): void;
    public scrollBy(dx: number, dy: number): void;
    public scrollTo(x: number, y: number): void;
    public onScrolled(scroller: Scroller): void;
    public static template: TemplateComponentFactory<typeof Scroller>;
  }
  interface ScrollerDictionary extends ContainerDictionary {
    loop?: boolean;
  }

  class Timeline {
    public constructor();
    public duration: number;
    public fraction: number;
    public time: number;
    public from(
      target: object,
      fromProperties: object,
      duration: number,
      easing?: string,
      delay?: number
    ): Timeline;
    public on(
      target: object,
      onProperties: object,
      duration: number,
      easing?: number,
      delay?: number
    ): Timeline;
    public seekTo(time: number): void;
    public to(
      target: object,
      fromProperties: object,
      duration: number,
      easing?: string,
      delay?: number
    ): Timeline;
  }
}
