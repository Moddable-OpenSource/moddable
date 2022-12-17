/*
 * Copyright (c) 2022  Shinya Ishikawa.
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

/*
 * Streaming from local audio samples.
 * 
 *   Streaming is not needed for playback: the full resource could be queued.
 *   ResourceStreamer is convenient for interoperability with other Streamer classes that have onPlayed callback, etc.
 */
import Resource from "Resource";

class ResourceStreamer {
  #audio;
  #stream;
  #playing = [];
  #bytesQueued = 0;
  #targetBytesQueued;
  #bytesPerSample = 2;
  #bytesPerBlock;
  #callbacks = [];
  #resource;

  constructor(options) {
    this.#resource = new Resource(options.path);
    this.#resource.position = 12; // skip maud header
    const sampleRate = options.audio.sampleRate;
    this.#targetBytesQueued = sampleRate * this.#bytesPerSample;
    this.#bytesPerBlock = Math.idiv(this.#targetBytesQueued, 8);
    if (this.#bytesPerBlock % this.#bytesPerSample)
      throw new Error("invalid bytesPerBlock");

    if (options.onPlayed) this.#callbacks.onPlayed = options.onPlayed;
    if (options.onError) this.#callbacks.onError = options.onError;
    if (options.onDone) this.#callbacks.onDone = options.onDone;

    const audio = options.audio.out;
    this.#audio = audio;
    this.#stream = options.audio.stream ?? 0;
    audio.callbacks ??= [];
    audio.callbacks[this.#stream] = (bytes) => {
      if (!bytes) {
        this.#callbacks.onDone?.();
        return;
      }

      this.#bytesQueued -= bytes;
      let played = this.#playing.shift();
      this.#callbacks.onPlayed?.(played);
      played = undefined;
      this.#fillQueue();
    };
    this.#fillQueue();
    this.#audio.start();
  }
  close() {
    this.#audio.enqueue(this.#stream, this.#audio.constructor.Flush);
    this.#audio.callbacks[this.#stream] = null;

    this.#audio = this.#playing = undefined;
  }
  #fillQueue() {
    while (
      this.#bytesQueued < this.#targetBytesQueued &&
      this.#resource.position < this.#resource.byteLength &&
      this.#audio.length(this.#stream) >= 2
    ) {
      const use = Math.min(
        this.#targetBytesQueued - this.#bytesQueued,
        this.#bytesPerBlock
      );
      const slice = this.#resource.slice(
        this.#resource.position,
        this.#resource.position + use,
        false
      );
      this.#resource.position += slice.byteLength;
      this.#audio.enqueue(
        this.#stream,
        this.#audio.constructor.RawSamples,
        slice,
        1,
        0,
        slice.byteLength / this.#bytesPerSample
      );
      this.#audio.enqueue(
        this.#stream,
        this.#audio.constructor.Callback,
        slice.byteLength
      );
      this.#bytesQueued += slice.byteLength;
      this.#playing.push(slice);
      if (this.#resource.position === this.#resource.byteLength)
        this.#audio.enqueue(0, this.#audio.constructor.Callback, 0);
    }
  }
}

export default ResourceStreamer;
