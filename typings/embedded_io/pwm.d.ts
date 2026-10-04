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


declare module "embedded:io/pwm" {
  import { PinSpecifier, PortSpecifier, IOOptions } from "embedded:io/_common";
  class PWM implements Disposable {
    target?: any;
    constructor(options: IOOptions<"PWM", {
      pin: PinSpecifier;
      port?: PortSpecifier;	// Moddable SDK extension
    } | {
      from: PWM;	// Moddable SDK extension: take over the pin of an open instance, which is closed
    }, {
      hz?: number;
      resolution?: number;	// Moddable SDK extension
      format?: "number";
      target?: any;
    }>);
    readonly resolution: number;
    readonly hz: number;
    write(value: number): void;
    close(): void;
    get format(): "number"
    set format(value: "number")
    [Symbol.dispose](): void;
  }
  export default PWM;
}
