/*
* Copyright (c) 2025-2026 Moddable Tech, Inc.
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

/*
  console
*/

interface Console {
  log(...args: any[]): void;
}

declare const console:Console;

/*
  watch global
*/

interface TimeChangeEvent {
  date: Date;
}

interface ConnectionState {
  app: boolean;
  pebblekit: boolean;
}

type TimeEventType = "secondchange" | "minutechange" | "hourchange" | "daychange";

type PebbleEventType = TimeEventType | "connected";

type TimeChangeCallback = (event: TimeChangeEvent) => void;

type ConnectedCallback = () => void;

interface watch {
  addEventListener(event: TimeEventType, callback: TimeChangeCallback): void;
  addEventListener(event: "connected", callback: ConnectedCallback): void;
  removeEventListener(event: TimeEventType, callback: TimeChangeCallback): void;
  removeEventListener(event: "connected", callback: ConnectedCallback): void;
  readonly connected: ConnectionState;
}

declare const watch: watch;

/*
  Resource
*/

declare const Resource: typeof import('Resource').default;

/*
  setTimeout and friends
*/

declare function setInterval(callback: () => void, delay: number): number;
declare function clearInterval(id: number): void;
declare function setTimeout(callback: () => void, delay: number): number;
declare function clearTimeout(id: number): void;

/*
  Web
*/

declare const URL: typeof import('url').URL;
declare const URLSearchParams: typeof import('url').URLSearchParams;
declare const Headers: typeof import('headers').default;
declare const localStorage: import('web/webstorage').default;
declare const WebSocket: typeof import('web/websocket').default;
declare const fetch: typeof import('web/fetch').fetch;

/*
  Piu
*/

declare const Application: ApplicationConstructor;
declare const Behavior: BehaviorConstructor;
declare const Column: ColumnConstructor;
declare const Container: ContainerConstructor;
declare const Content: ContentConstructor;
declare const Label: LabelConstructor;
declare const Link: LinkConstructor;
declare const Port: PortConstructor;
declare const Row: RowConstructor;
declare const Skin: SkinConstructor;
declare const Style: StyleConstructor;
declare const Text: TextConstructor;
declare const Texture: TextureConstructor;
declare const Transition: TransitionConstructor;

declare const InverterConstructor: InverterConstructor;
declare const RoundRect: RoundRectConstructor;
declare const ScreenBufferConstructor: ScreenBufferConstructor;
declare const SVGImage: SVGImageConsructor;

/*
  ECMA-419
*/

// device is defined in typings/pebble/device.d.ts
