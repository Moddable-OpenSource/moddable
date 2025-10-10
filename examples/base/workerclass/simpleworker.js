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