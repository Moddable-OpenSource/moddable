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

import { SimpleWorker  } from "simpleworker";

/*
 * coreworker.js
 * Runs INSIDE the worker context. Uses global onmessage/postMessage.
 * This is where you respond to main-thread messages.
 */

self.onmessage = SimpleWorker.prototype.onmessage.bind(self);
