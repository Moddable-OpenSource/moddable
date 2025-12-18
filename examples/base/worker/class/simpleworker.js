/*
 * Copyright (c) 2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

/*
 * SimpleWorker: main-thread wrapper that subclasses Worker.
 * This class lives on the main side. The *worker-side* logic is in "coreworker.js".
 */

import Worker from "worker";

export class SimpleWorker extends Worker {
  static counter = 0;

  onmessage(msg) {
      trace(`simpleworker.js: counter: ${SimpleWorker.counter} message: ${JSON.stringify(msg)}\n`);
      this.postMessage({hello: "from simple worker", counter: ++SimpleWorker.counter});
  }
}
