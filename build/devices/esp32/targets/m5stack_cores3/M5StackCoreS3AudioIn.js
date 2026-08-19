/*
 * Copyright (c) 2026 Shinya Ishikawa
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

import AudioIn from "embedded:io/audio/in-original";
import { acquireI2S, releaseI2S } from "M5StackCoreS3I2SBus";

export default class M5StackCoreS3AudioIn extends AudioIn {
  constructor(options) {
    acquireI2S("microphone", options && options.sampleRate);
    try {
      super(options);
    }
    catch (error) {
      releaseI2S("microphone");
      throw error;
    }
  }

  start() {
    acquireI2S("microphone", this.sampleRate);
    return super.start();
  }

  stop() {
    const result = super.stop();
    releaseI2S("microphone");
    return result;
  }

  close() {
    try {
      return super.close();
    }
    finally {
      releaseI2S("microphone");
    }
  }
}
