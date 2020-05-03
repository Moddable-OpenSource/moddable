/*
 * Copyright (c) 2020 Moddable Tech, Inc.
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

import Net from "net";

class Resolver {
	#onResolved;
	#onError;
	#target;
	constructor(options) {
		this.#target = options.target || this;
		this.#onResolved = options.onResolved || this.onResolved;
		this.#onError = options.onError || this.onError;
		Net.resolve(options.host, (host, address) => {
			if (address)
				this.#onResolved.call(this.#target, address);
			else
				this.#onError.call(this.#target);
		});
	}
	close() {
		this.#target = undefined;
		this.#onResolved = undefined;
		this.#onError = undefined;
	}
}
Object.freeze(Resolver.prototype);

export default Resolver;
