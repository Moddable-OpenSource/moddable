/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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
	worker
*/

class Worker extends Native("xs_worker_destructor") {
	constructor(module, options) { super(); native("xs_worker").call(this, module, options); }
	postMessage(message) { return native("xs_worker_postfromworkerinstantiator").call(this, message); }
	terminate() { return native("xs_worker_terminate").call(this); }
};

/*
	shared worker
*/

export class SharedWorker extends Native("xs_worker_destructor") {
	constructor(module, options) { super(); native("xs_sharedworker").call(this, module, options); }
};

export default Worker;
