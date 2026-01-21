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

type TapDirection = "+x" | "-x" | "+y" | "-y" | "+z" | "-z";

interface AccelerometerOptions {
  onSample?: () => void;
  onTap?: (direction: TapDirection) => void;
  onDoubleTap?: (direction: TapDirection) => void;
}

interface AccelerometerConfigureOptions {
  hz?: 10 | 25 | 50 | 100;
}

interface AccelerometerSample {
  x: number;
  y: number;
  z: number;
}

declare class Accelerometer {
  constructor(options: AccelerometerOptions);
  close(): void;
  configure(options: AccelerometerConfigureOptions): void;
  sample(): AccelerometerSample;
}

export default Accelerometer;
