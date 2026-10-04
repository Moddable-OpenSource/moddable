/*
* Copyright (c) 2026 Moddable Tech, Inc.
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


declare module "embedded:provider/builtin" {
  export interface Device {
  }
  export interface DeviceIO {
  }
  // the host typing of a target with a network attaches DeviceNetwork to Device; the network configurations
  // in the build (config/dns, config/httpclient, ...) add their providers to it
  export interface DeviceNetwork {
  }

  export interface DeviceAnalog {
  }
  export interface DeviceI2C {
  }
  export interface DeviceSPI {
  }
  export interface DeviceSerial {
  }

  type Speed<T, K extends keyof T> = T extends unknown ? Omit<T, K> & Partial<Pick<T, K>> : never;
  export type AnalogBus = ConstructorParameters<typeof import("embedded:io/analog").default>[0] & { io: typeof import("embedded:io/analog").default };
  export type I2CBus = Speed<Omit<ConstructorParameters<typeof import("embedded:io/i2c").default>[0], "address">, "hz"> & { io: typeof import("embedded:io/i2c").default };
  export type SPIBus = Speed<ConstructorParameters<typeof import("embedded:io/spi").default>[0], "hz"> & { io: typeof import("embedded:io/spi").default };
  export type SerialBus = Speed<ConstructorParameters<typeof import("embedded:io/serial").default>[0], "baud"> & { io: typeof import("embedded:io/serial").default };

  // any function or class
  type Callable = ((...args: never) => unknown) | (abstract new (...args: never) => unknown);
  // values that are not remapped: functions, classes, arrays, and byte buffers
  type Leaf = Callable | readonly unknown[] | ArrayBufferLike | ArrayBufferView;

  // device after lockdown: frozen with every io resolved to a constructor
  type Frozen<T> = T extends Leaf ? T : T extends object ? { readonly [K in keyof T]: Frozen<T[K]> } : T;

  // device before lockdown (during preload)
  type Provider = { io: Callable };
  type IOName<C> = { [N in keyof DeviceIO]: DeviceIO[N] extends C ? N : never }[keyof DeviceIO];
  type ProviderConfig<T> = T extends Leaf ? T : T extends object ? { -readonly [K in keyof T]: K extends "io" ? T[K] | IOName<T[K]> : ProviderConfig<T[K]> } : T;
  export type DeviceConfig<T = Device> = T extends Leaf ? T : T extends Provider ? ProviderConfig<T> : T extends object ? { -readonly [K in keyof T]?: DeviceConfig<T[K]> } : T;

  const device: Frozen<Device>;
  export default device;
}
