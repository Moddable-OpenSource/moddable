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

declare module "embedded:io/digitalbank" {
  type Mode = 
    | typeof DigitalBank.Input
    | typeof DigitalBank.InputPullUp
    | typeof DigitalBank.InputPullDown
    | typeof DigitalBank.InputPullUpDown
    | typeof DigitalBank.Output
    | typeof DigitalBank.OutputOpenDrain

  class DigitalBank {
    static readonly Input = 0
    static readonly InputPullUp = 1
    static readonly InputPullDown = 2
    static readonly InputPullUpDown = 3
    static readonly Output = 8
    static readonly OutputOpenDrain = 9

    constructor(dictionary: {
      pins: number,
      mode: Mode,
      rises?: number,
      falls?: number,
      onReadable?: (this: DigitalBank, trigger: number) => void;
    })

    close(): void;
    read(): void;
    write(value: number): void;
    onReadable: (trigger: number) => void;

    pins: number;
    mode: Mode;
    rises?: number;
    falls?: number;

    get format(): "number"
    set format(value: "number")
  }

  export default DigitalBank;
}