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

declare module 'pins/digital/monitor' {
  type Input = 0
  type InputPullUp = 1
  type InputPullDown = 2
  type InputPullUpDown = 3
  type Output = 8
  type OutputOpenDrain = 9
  type Mode = Input | InputPullUp | InputPullDown | InputPullUpDown | Output | OutputOpenDrain
  type Rising = 1
  type Falling = 2
  type Edge = Rising | Falling
  class Monitor {
    public constructor(dictionary: { pin: number; port?: string; mode: Mode; edge: Edge })
    public onChanged(callback: () => void): void
    public read(): number
    public close(): void
    public readonly rises: number
    public readonly falls: number
  }
  export { Monitor as default }
}
