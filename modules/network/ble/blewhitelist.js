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

class BLEWhitelist {
	static add(value) {
		this.#whitelistAdd(this.#whitelistEntry(value));
	}
	static remove(value) {
		this.#whitelistRemove(this.#whitelistEntry(value));
	}
	static clear() @ "xs_gap_whitelist_clear"
	
	static #whitelistEntry(value) {
		let entry = {};
		if ("string" === typeof value) {
			entry.address = new Bytes(value.replaceAll(":", ""));
			entry.addressType = GAP.AddressType.PUBLIC;
		}
		else if ("object" === typeof value) {
			if ("address" in value) {
				if ("string" === typeof value.address)
					entry.address = new Bytes(value.address.replaceAll(":", ""));
				else if (value.address instanceof ArrayBuffer)
					entry.address = value.address;
				entry.addressType = value.addressType;
			}
			else if (value.address instanceof ArrayBuffer)
				entry.address = value;
		}
		
		if (undefined === entry.address)
			throw new Error("unknown whitelist entry format");
			
		if (undefined === entry.addressType)
			entry.addressType = GAP.AddressType.PUBLIC;
			
		return entry;
	}
	static #whitelistAdd(entry) @ "xs_gap_whitelist_add"
	static #whitelistRemove(entry) @ "xs_gap_whitelist_remove"
	static #whitelistClear() @ "xs_gap_whitelist_clear"
}
Object.freeze(BLEWhitelist.prototype);

export default BLEWhitelist;
