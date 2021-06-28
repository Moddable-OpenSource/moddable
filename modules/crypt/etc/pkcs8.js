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

import Crypt from "crypt";
import BER from "ber";

function howmany(x, y)
{
	return ((x + y - 1) / y) | 0;
}

function pkcs12gen(P, S, c, dkLen, hash, v)
{
	// assuming dkLen <= hash.outputSize
	var h = new hash();

	// D = id || id || ...
	var D = new Uint8Array(v);
	D.fill(1);	// id = 1 for key material
	h.update(D.buffer);

	var len = v * howmany(S.byteLength, v), l;
	for (l = 0; l + S.byteLength < len; l += S.byteLength)
		h.update(S);
	if (len - l > 0)
		h.update(S.slice(0, len - l));

	// convert the string to a wchar string for password
	var UP = new Uint8Array((P.length + 1) * 2);
	for (var i = 0, j = 0; i < P.length; i++) {
		UP[j++] = 0;
		UP[j++] = P.charCodeAt(i);
	}
	UP[j++] = 0;
	UP[j] = 0;
	var buf = UP.buffer;
	len = v * howmany(buf.byteLength, v);
	for (l = 0; l + buf.byteLength < len; l += buf.byteLength)
		h.update(buf);
	if (len - l > 0)
		h.update(buf.slice(0, len - l));

	var dk = h.close();
	for (c -= 1; --c >= 0;)
		dk = h.process(dk);
	return dk.slice(0, dkLen);
}

export default class PKCS8 {
	static parse(buf) {
		// RSA only
		var key = {};
		var ber = new BER(buf);
		if (ber.getTag() != 0x30)	// SEQUENCE
			throw new Error("PKCS8: not a sequence");
		ber.getLength()		// skip the sequence length
		ber.getInteger();	// skip the Version
		ber.next();		// skip the AlgorithmIdentifier
		ber.getTag();		// OCTET STRING that includes the private key in the SEQUENCE
		ber.getLength();
		{
			ber.getTag();		// SEQUENCE
			ber.getLength();
			ber.getInteger();	// skip the first INTEGER
			key.modulus = ber.getInteger();
			key.exponent = ber.getInteger();
			key.privExponent = ber.getInteger();
			key.prim1 = ber.getInteger();
			key.prim2 = ber.getInteger();
			key.exponent1 = ber.getInteger();
			key.exponent2 = ber.getInteger();
			key.coefficient = ber.getInteger();
		}
		return key;
	};
	static decrypt(buf, pass) {
		var ber = new BER(buf);
		if (ber.getTag() == 0x30) {
			ber.getLength();
			if (ber.getTag() == 0x30) {
				ber.getLength();
				var oid = ber.getObjectIdentifier();
				if (ber.getTag() == 0x30) {
					ber.getLength();
					var salt = ber.getOctetString();
					var iter = ber.getInteger();
					var data = ber.getOctetString();
					var cipher, hash, mode, v;
					switch (oid.toString()) {
					case [1,2,840,113549,1,12,1,1].toString():	// pbeWithSHA1And128BitRC4
						cipher = Crypt.RC4;
						hash = Crypt.SHA1;
						v = 512 / 8;
						break;
					default:
						throw new Error("PKCS8: unsupported encryption scheme: " + oid);
						break;
					}
					var key = pkcs12gen(pass, salt, iter.toNumber(), 16, hash, v);
					var enc = new cipher(key);
					if (mode)
						var enc = new mode(enc);
					enc.decrypt(data, data);
					return this.parse(data);
				}
			}
		}
		throw new Error("PKCS8: malformed input");
	};
};

Object.freeze(PKCS8.prototype);
