/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import GAP from "gap";
import {Bytes} from "btutils";

class GAPWhitelist {
	static add(address, addressType) {
		this.#whitelistAdd(this.#whitelistEntry(address, addressType));
	}
	static remove(address, addressType) {
		this.#whitelistRemove(this.#whitelistEntry(address, addressType));
	}
	static clear() @ "xs_gap_whitelist_clear"
	
	static #whitelistEntry(address, addressType = GAP.AddressType.PUBLIC) {
		let entry = { addressType };
		
		if ("string" === typeof address) {
			entry.address = new Bytes(address.replaceAll(":", ""), true);
		}
		else if (address instanceof ArrayBuffer)
			entry.address = address;
		
		if (undefined === entry.address)
			throw new Error("unknown whitelist entry format");
			
		return entry;
	}
	static #whitelistAdd(entry) @ "xs_gap_whitelist_add"
	static #whitelistRemove(entry) @ "xs_gap_whitelist_remove"
	static #whitelistClear() @ "xs_gap_whitelist_clear"
}
Object.freeze(GAPWhitelist.prototype);

export default GAPWhitelist;
