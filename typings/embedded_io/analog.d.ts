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


declare module "embedded:io/analog" {
  import { PinSpecifier, IOOptions } from "embedded:io/_common";
  class Analog implements Disposable {
    constructor(options: IOOptions<"Analog", {
      pin: PinSpecifier;
    }, {
      resolution?: number;
      format?: "number";
      target?: any;
    }>);
    readonly resolution: number;
    read(): number;
    close(): void;
    get format(): "number"
    set format(value: "number")
    target?: any;
    [Symbol.dispose](): void;
  }
  export default Analog;
}
