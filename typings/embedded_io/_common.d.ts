/*
* Copyright (c) 2022 Shinya Ishikawa
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


declare module "embedded:io/_common" {
  /**
   * Pin and port specifiers are host-dependent values (ECMA-419 9.1 and 9.2).
   * A host declares its own by merging `pin` and/or `port` into this interface, for example:
   *
   *     declare module "embedded:io/_common" {
   *       interface HostSpecifiers {
   *         pin: {port: string; pin: number};
   *         port: string;
   *       }
   *     }
   *
   * A host that declares nothing uses numbers.
   */
  export interface HostSpecifiers {}

  export type PinSpecifier = HostSpecifiers extends {pin: infer T} ? T : number
  export type PortSpecifier = HostSpecifiers extends {port: infer T} ? T : number

  /**
   * The constructor options that select the hardware of an IO class are host-dependent too.
   * ECMA-419 defines them as pins. A host that selects hardware another way, such as by name
   * from a devicetree, declares the options of a class by merging a property with the name
   * of the class into this interface, for example:
   *
   *     declare module "embedded:io/_common" {
   *       interface HostOptions {
   *         PWM: {port: PortSpecifier; channel?: number};
   *       }
   *     }
   *
   * These replace the hardware options ECMA-419 defines for the class: its other options are unchanged.
   */
  export interface HostOptions {}

  /**
   * The constructor options of IO class `Name`: the host's hardware options if it declares any,
   * otherwise `Hardware` (those of ECMA-419), together with `Rest`.
   */
  export type IOOptions<Name extends string, Hardware, Rest> =
    Flatten<(HostOptions extends Record<Name, infer T> ? T : Hardware) & Rest>

  type Flatten<T> = {[K in keyof T]: T[K]}
}