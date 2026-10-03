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

// on Pebble, a texture may be created from the id of a GBitmap resource
/// <reference path="./texture.d.ts" />
// the Piu QRCode, RoundRect and SVGImage contents are the same modules as on every other target; their typings declare the globals
/// <reference path="../piu/QRCode.d.ts" />
/// <reference path="../piu/RoundRect.d.ts" />
/// <reference path="../piu/SVGImage.d.ts" />

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

interface FirmwareVersion {
  major: number;
  minor: number;
  patch: number;
}

interface LaunchInfo {
  reason: number;
  arguments: number;
}

interface WakeupEvent {
  id: number;
  cookie: number;
}

type TimeEventType = "secondchange" | "minutechange" | "hourchange" | "daychange";

type HealthEventSubtype = "significant" | "movement" | "sleep" | "metric" | "heart rate";

type PebbleEventType = TimeEventType | "connected" | "resize" | "willFocus" | "didFocus" | "wakeup" | "health";

type TimeChangeCallback = (event: TimeChangeEvent) => void;

type ConnectedCallback = () => void;

type ResizeCallback = (progress: number) => void;

type FocusCallback = (inFocus: boolean) => void;

type WakeupCallback = (event: WakeupEvent) => void;

type HealthCallback = (subtype: HealthEventSubtype) => void;

interface watch {
  addEventListener(event: TimeEventType, callback: TimeChangeCallback): void;
  addEventListener(event: "connected", callback: ConnectedCallback): void;
  addEventListener(event: "resize", callback: ResizeCallback): void;
  addEventListener(event: "willFocus", callback: FocusCallback): void;
  addEventListener(event: "didFocus", callback: FocusCallback): void;
  addEventListener(event: "wakeup", callback: WakeupCallback): void;
  addEventListener(event: "health", callback: HealthCallback): void;
  removeEventListener(event: TimeEventType, callback: TimeChangeCallback): void;
  removeEventListener(event: "connected", callback: ConnectedCallback): void;
  removeEventListener(event: "resize", callback: ResizeCallback): void;
  removeEventListener(event: "willFocus", callback: FocusCallback): void;
  removeEventListener(event: "didFocus", callback: FocusCallback): void;
  removeEventListener(event: "wakeup", callback: WakeupCallback): void;
  removeEventListener(event: "health", callback: HealthCallback): void;
  light(enable?: boolean): void;
  exit(reason?: number): void;
  readonly connected: ConnectionState;
  readonly hour12: boolean;
  readonly model: number;
  readonly firmwareVersion: FirmwareVersion;
  readonly launch: LaunchInfo;
  readonly wake: WakeupEvent | undefined;
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
declare function setImmediate(callback: () => void): number;
declare function clearImmediate(id: number): void;

/*
  Modules and native functions
*/

/** imports a module of the application and returns its namespace */
declare function importNow(specifier: string): any;
/** the application's C functions, when it has any (see XS FFI) */
declare const Natives: Record<string, (...args: any[]) => any>;

/*
  Screen: Pebble's display adds whether it is color and round, and the area not obstructed by the system
*/

declare module "commodetto/Poco" {
	interface Screen {
		readonly color: boolean;
		readonly round: boolean;
		readonly frameBuffer: true;
		readonly unobstructed: {
			readonly x: number;
			readonly y: number;
			readonly width: number;
			readonly height: number;
		};
	}
}

/*
  Web
*/

declare const URL: typeof import('url').URL;
declare const URLSearchParams: typeof import('url').URLSearchParams;
declare const Headers: typeof import('headers').default;
declare const localStorage: import('webstorage').default;
declare const WebSocket: typeof import('web/websocket').default;
declare const fetch: typeof import('fetch').fetch;

/*
  Piu
*/

type Application = import("piu/MC-types").Application;
type Behavior = import("piu/MC-types").Behavior;
type Column = import("piu/MC-types").Column;
type Container = import("piu/MC-types").Container;
type Content = import("piu/MC-types").Content;
type ContentDictionary = import("piu/MC-types").ContentDictionary;
type Label = import("piu/MC-types").Label;
type Link = import("piu/MC-types").Link;
type Locals = import("piu/MC-types").Locals;
type Port = import("piu/MC-types").Port;
type Row = import("piu/MC-types").Row;
type Skin = import("piu/MC-types").Skin;
type Style = import("piu/MC-types").Style;
type Text = import("piu/MC-types").Text;
type Texture = import("piu/MC-types").Texture;
type Transition = import("piu/MC-types").Transition;

declare const Inverter: InverterConstructor;
declare const ScreenBuffer: ScreenBufferConstructor;

/*
  ECMA-419
*/

// device is typed by typings/host/pebble.d.ts

declare module "embedded:storage/files" {
  interface DirectoryOpenFileOptions {
    size?: number;
  }
}

