/*
 * Copyright (c) 2022  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

// derived from print_ski.js

import { FILE, TOOL } from "tool";

import X509 from "x509";

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		if (!this.getenv("MODDABLE"))
			throw new Error("$MODDABLE undefined");
	}
	run() {
		const MODDABLE = this.getenv("MODDABLE");
		const dataPath = MODDABLE + "/modules/crypt/data"
	
		const paths = this.enumerateDirectory(dataPath);
		const certs = [];
		paths.forEach(path => {
			if (!(path.startsWith("ca") && path.endsWith(".der")))
				return;
			
			let index = path.substring(2, path.length - 4);
			if (index != parseInt(index))
				return;
			
			certs[index] = path;
		});
		
		for (let i = 0; i < certs.length; i++) {
			if (undefined === certs[i])
				throw new Error("certificates must be consecutively numbered")
		}
		
		const f1 = new FILE(`${dataPath}/ca.ski`, "wb");
		f1.write(new ArrayBuffer(20));
		for (let i = 0; i < certs.length; i++) {
			const crt = this.readFileBuffer(`${dataPath}/${certs[i]}`);
			let ski = X509.decodeSKI(crt);
			if (ski.byteLength > 20)
				throw new Error("SKI too long!");

			if (ski.byteLength < 20) {
				const buf = new Uint8Array(20);
				buf.set(new Uint8Array(ski), 20 - ski.byteLength);
			}
			else
				ski = new Uint8Array(ski);
			
			f1.writeBuffer(ski.buffer);
		}
		f1.close();
		
		trace(`Updated: ${dataPath}/ca.ski\n`);
	}
}
