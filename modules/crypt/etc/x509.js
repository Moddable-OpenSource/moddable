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

import BER from "ber";
import ECPoint from "ecp"
import Curve from "curve";
import {Digest} from "crypt";

const X509 = {
	decode(buf) {
		const ber = new BER(buf);
		if (ber.getTag() != 0x30)
			throw new Error("x509: malformed input");
		ber.getLength();
		return {
			tbs: ber.next(),
			algo: (new BER(ber.getSequence())).getObjectIdentifier(),
			sig: ber.getBitString(),
			spki: this.decodeSPKI(buf),
		};
	},
	decodeTBS(buf) {
		let tbs = {};
		let ber = new BER(buf);
		if (ber.getTag() != 0x30)
			throw new Error("x509: malformed TBS");
		ber.getLength();
		if (ber.peek() & 0x80) {
			ber.skip(1);
			tbs.version = ber.next();
		}
		tbs.serialNumber = ber.next();
		tbs.signature = ber.next();
		tbs.issuer = ber.next();

		let validity = new BER(ber.next());
		if (validity.getTag() != 0x30)
			throw new Error("x509: malformed TBS");
		validity.getLength();
		let from = validity.next();
		let to = validity.next();
		from = parseDate(String.fromArrayBuffer(from.buffer.slice(from.byteOffset + 2, from.byteOffset + 2 + from.length - 2)));
		to = parseDate(String.fromArrayBuffer(to.buffer.slice(to.byteOffset + 2, to.byteOffset + 2 + to.length - 2)));
		tbs.validity = {from, to};
		tbs.subject = ber.next();
		tbs.subjectPublicKeyInfo = ber.next();
		return tbs;
	},
	getSPK(spki) {		// Subject Public Key Info
		spki = this._decodeSPKI(spki);
		if (!spki)
			throw new Error("x509: no SPKI");
		spki = new BER(spki);
		if (spki.getTag() == 0x30) {
			spki.getLength();
			if (spki.getTag() == 0x30) {
				let endp = spki.i + spki.getLength();
				let algo = spki.getObjectIdentifier();
				let param = null;
				if (spki.i < endp) {
					param = spki.next();	// OPTIONAL: parameters -- NULL for RSA
				}
				let spk = spki.getBitString();
				if (spk) {
					spk.algo = algo;
					spk.param = param;
				}
				return spk;
			}
		}
	},
	decodeSPKI(spk) {
		spk = this.getSPK(spk);
		if (!spk)
			throw new Error("x509: bad SPKI");

		let ber;
		switch (spk.algo.toString()) {
			case "1,2,840,113549,1,1,1":
			case "1,3,14,3,2,11":
			case "2,5,8,1,1": {
				// PKCS1
				ber = new BER(spk);
				if (ber.getTag() !== 0x30)
					throw new Error("x509: bad SPKI");
				ber.getLength();
				return {
					modulus: ber.getInteger(),
					exponent: ber.getInteger(),
					algo: spk.algo,
				};
			}
			case "1,2,840,10045,2,1": {
				// ecPublicKey
				if (!spk.param)
					throw new Error("x509: support named curves only for now");
				ber = new BER(spk.param);
				switch (ber.getObjectIdentifier().toString()) {
					case "1,2,840,10045,3,1,7":
						return {
							curve: new Curve("secp256r1"),
							pub: ECPoint.fromOctetString(spk),
							algo: spk.algo,
						};
					default:
						throw new Error("x509: unsupported curve");
				}
			}
			default:
				trace("x509: " + spk.algo + " not supported\n");
				return {};
		}
		throw new Error("x509: bad SPKI");
	},
	decodeSKI(buf) {
		let ski = this.decodeExtension(buf, [2, 5, 29, 14]);
		if (ski) {
			// must be a OCTET STRING
			let ber = new BER(ski);
			return ber.getOctetString()
		}
		// make up SKI by SHA1(SPK)
		let spk = this.getSPK(buf);
		if (!spk)
			throw new Error("x509: no SPK!?");
		const digest = new Digest("SHA1");
		digest.write(spk);
		return digest.close();
	},
	decodeAKI(buf) {
		let aki = this.decodeExtension(buf, [2, 5, 29, 35]);
		if (aki) {
			let ber = new BER(aki);
			ber.getTag();	// SEQUENCE
			let len = ber.getLength();
			let endp = ber.i + len;
			while (ber.i < endp) {
				if ((ber.getTag() & 0x1f) == 0) {
					len = ber.getLength();
					return ber.getChunk(len);
				}
				ber.skip(ber.getLength());
			}
		}
	},
	_decodeSPKI(buf) @ "xs_x509_decodeSPKI",
	decodeExtension(buf, extid) @ "xs_x509_decodeExtension",
};
Object.freeze(X509);

function parseDate(date) {
	if (!date.endsWith("Z"))
		throw new Error("unexpected timezone");

	const parts = [];
	if (13 === date.length) {
		for (let i = 0, length = date.length; (i + 1) < length; i += 2)
			parts.push(parseInt(date.slice(i, i + 2)));
		parts[0] += (parts[0] < 50) ? 2000 : 1900;
	}
	else if (15 === date.length) {
		parts.push(parseInt(date.slice(0, 4)));
		for (let i = 4, length = date.length; (i + 1) < length; i += 2)
			parts.push(parseInt(date.slice(i, i + 2)));
	}
	else
		throw new Error("unexpected date");
	parts[1] -= 1;
	return Date.UTC.apply(null, parts);
}

export default X509;
