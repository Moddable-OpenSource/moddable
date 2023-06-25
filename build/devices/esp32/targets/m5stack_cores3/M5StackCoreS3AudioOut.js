/*
 * Copyright (c) 2023 Shinya Ishikawa
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import AudioOut from "pins/audioout-original";

/**
 * A special AudioOut implementation for M5Stack CoreS3
 * CoreS3 has an amplifier IC (AW88298).
 * The user must set the sample rate to the IC before playing audio.
 */
export default class M5StackCoreS3AudioOut extends AudioOut {
  constructor(options) {
    super(options);
    if (globalThis.amp == null) {
      trace("amp not found\n");
    } else {
      globalThis.amp.sampleRate = this.sampleRate;
    }
  }
}
