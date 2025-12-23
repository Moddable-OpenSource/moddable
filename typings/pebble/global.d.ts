/*
  console
*/

interface Console {
  log(...args: any[]): void;
}

declare const console:Console;

/*
  Pebble global
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

interface Pebble {
  addEventListener(event: TimeEventType, callback: TimeChangeCallback): void;
  addEventListener(event: "connected", callback: ConnectedCallback): void;
  removeEventListener(event: TimeEventType, callback: TimeChangeCallback): void;
  removeEventListener(event: "connected", callback: ConnectedCallback): void;
  readonly connected: ConnectionState;
  readonly color: boolean;
}

declare const Pebble: Pebble;

/*
  setTimeout and friends
*/

declare function setInterval(callback: () => void, delay: number): number;
declare function clearInterval(id: number): void;
declare function setTimeout(callback: () => void, delay: number): number;
declare function clearTimeout(id: number): void;
