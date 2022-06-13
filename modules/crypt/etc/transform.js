/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

import Base64 from "base64";
import BER from "ber";

class Transform {
	static pemToDER(source, head = "-----BEGIN CERTIFICATE-----", tail = "-----END CERTIFICATE-----") {
		if ("string" !== typeof source)
			source = String.fromArrayBuffer(source);

		const start = source.indexOf(head);
		const end = (start < 0) ? -1 : source.indexOf(tail, start);
		if ((start < 0) || (end < 0))
			throw new Error("no delimeter");

		return Base64.decode(source.slice(start + head.length, end))
	}
	static pemToPKC8(source) {
		return this.pemToDER(source, "-----BEGIN RSA PRIVATE KEY-----", "-----END RSA PRIVATE KEY-----");
	}
	static pkcs8ToDER(source) {	// https://datatracker.ietf.org/doc/html/rfc5208#section-5
		return BER.encode([
				0x30,
					[0x02, 0n],	// version
					[0x30,	// privateKeyAlgorithm
						[0x06, [1, 2, 840, 113549, 1, 1, 1]],	// PKCS#1 OID  
						[0x05]	// NULL
					],
					[0x04, source]		// privateKey
			]);
	}
}

export default Transform;
