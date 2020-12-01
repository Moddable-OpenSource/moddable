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
 */
import GAP from "gap";

const IOCapability = {
	NoInputNoOutput: 0,
	DisplayOnly: 1,
	KeyboardOnly: 2,
	KeyboardDisplay: 3,
	DisplayYesNo: 4
};
Object.freeze(IOCapability);

const Authorization = {
	None: 0,
	NoMITM: 1,
	MITM: 2,
	SignedNoMITM: 3,
	SignedMITM: 4
};
Object.freeze(Authorization);

export default class SM {
	static deleteAllBondings() @ "xs_ble_sm_delete_all_bondings"
	
	static deleteBonding(address, addressType = GAP.AddressType.PUBLIC) {
		SM.#deleteBonding(address, addressType);
	}

	static #deleteBonding() @ "xs_ble_sm_delete_bonding"
};
Object.freeze(SM.prototype);

export {SM, IOCapability, Authorization};
