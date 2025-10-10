import { SimpleWorker  } from "simpleworker";

/*
 * coreworker.js
 * Runs INSIDE the worker context. Uses global onmessage/postMessage.
 * This is where you respond to main-thread messages.
 */

self.onmessage = SimpleWorker.prototype.onmessage.bind(self);