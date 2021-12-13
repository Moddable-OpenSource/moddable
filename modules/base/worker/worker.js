/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

class Worker @ "xs_worker_destructor" {
	constructor(module, options) @ "xs_worker";
	postMessage(message) @ "xs_worker_postfromworkerinstantiator";
	terminate() @ "xs_worker_terminate";
};
Object.freeze(Worker.prototype);

/*
	shared worker
*/

export class SharedWorker @ "xs_worker_destructor" {
	constructor(module, options) @ "xs_sharedworker";
};
Object.freeze(SharedWorker.prototype);

export default Worker;
