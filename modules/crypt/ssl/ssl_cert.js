/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

// for MC
import Resource from "Resource";
import Crypt from "crypt";
import X509 from "x509";
import PKCS1_5 from "pkcs1_5";
import DSA from "dsa";
import Bin from "bin";
import BER from "ber";
import PKCS8 from "pkcs8"

class CertificateManager {
	constructor(options) {
		this.registeredCerts = [];
		if (options.certificate)
			this.register(options.certificate);
	};
	getCerts() {
		// return the self certs
		return [getResource("srvcert.der")];
	};
	getKey(cert) {
		if (!cert) {
			// return the self key
			let Configuration = require.weak("config");
			return PKCS8.decrypt(getResource("srvkey.pk8"), Configuration.pk8Password);
		}
		// look for the key corresponding to the cert
		// first, search in the registed cert
		for (var i = 0; i < this.registeredCerts.length; i++) {
			if (Bin.comp(this.registeredCerts[i], cert) == 0) {
				var x509 = X509.decode(this.registeredCerts[i]);
				return x509.spki;	// public key only
			}
		}
		// at the moment there is only one key
		if (Bin.comp(getResource("clntcert.der"), cert) == 0) {
			let Configuration = require.weak("config");
			return PKCS8.decrypt(getResource("clntkey.pk8"), Configuration.pk8Password);
		}
	};
	findPreferedCert(types, names) {
		// MC has only one key pair...
		return [getResource("clntcert.der")];
	};
	getIndex(fname, target) {
		if (undefined === target)
			return -1;

		let f = new Resource(fname);
		for (let i = 0; ((i + 1) * 20) <= f.byteLength; i++) {
			if (Bin.comp(f.slice(i * 20, (i + 1) * 20), target) == 0)
				return i;
		}
		return -1;
	};
	findCert(fname, target) {
		var i = this.getIndex(fname, target);
		if (i < 0)
			return;	// undefined
		return X509.decodeSPKI(getResource("ca" + i + ".der"));
	};
	verify(certs) {
		let length = certs.length - 1;

		// this approach calls decodeSPKI once more than necessary in favor of minimizing memory use
		for (let i = 0; i < length; i++) {
			if (!this._verify(X509.decodeSPKI(certs[i + 1]), X509.decode(certs[i])))
				return false;

			let aki = X509.decodeAKI(certs[i + 1]);
			for (let j = 0; j < this.registeredCerts.length; j++) {
				if (Bin.comp(X509.decodeSKI(this.registeredCerts[j]), aki) == 0) {
					let spki = X509.decodeSPKI(this.registeredCerts[j]);
					if (spki && this._verify(spki, X509.decode(certs[i + 1])))
						return true;
				}
			}

			let spki = this.findCert("ca.ski", aki);
			aki = undefined;
			if (spki && this._verify(spki, X509.decode(certs[i + 1])))
				return true;
				// else fall thru
		}

		let x509 = X509.decode(certs[length]);
		let spki = this.findCert("ca.ski", X509.decodeAKI(certs[length]));
		if (spki && this._verify(spki, x509))
			return true;
			// else fall thru

		spki = this.findCert("ca.subject", new Crypt.Digest("SHA1")).process(X509.decodeTBS(x509.tbs).issuer);
		return spki && this._verify(spki, x509);
	};

	_verify(spki, x509) {
		var pk;
		var hash;
		var sig;
		switch (x509.algo.toString()) {
		case [1, 2, 840, 113549, 1, 1, 4].toString():	// PKCS-1 MD5 with RSA encryption
			hash = "MD5";
			pk = PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 5].toString():	// PKCS-1 SHA1 with RSA encryption
			hash = "SHA1";
			pk = PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 11].toString():	// PKCS-1 SHA256 with RSA encryption
			hash = "SHA256";
			pk = PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 12].toString():	// PKCS-1 SHA384 with RSA encryption
			hash = "SHA384";
			pk = PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 13].toString():	// PKCS-1 SHA512 with RSA encryption
			hash = "SHA512";
			pk = PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 14].toString():	// PKCS-1 SHA224 with RSA encryption
			hash = "SHA224";
			pk = PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 10040, 4, 3].toString():
		case [1, 3, 14, 3, 2, 27].toString():
			hash = "SHA1";
			pk = DSA;
			// needs to decode the sig value into <r, s>
			var ber = new BER(x509.sig);
			if (ber.getTag() == 0x30) {
				ber.getLength();
				var r = ber.getInteger();
				var s = ber.getInteger();
				sig = r.concat(s);
			}
			break;
		default:
			throw new Error("Cert: unsupported algorithm: " + x509.algo.toString());
			break;
		}
		var H = (new Crypt.Digest(hash)).process(x509.tbs);
		return (new pk(spki, false, [] /* any oid */)).verify(H, sig);
	};
	register(cert) {
		this.registeredCerts.push(cert);
	};
	getDH() {
		let dh = getResource("dh.der");
		let ber = new BER(dh);
		if (ber.getTag() == 0x30) {
			ber.getLength();
			let p = ber.getInteger();
			let g = ber.getInteger();
			return {p, g};
		}
	};
};

function getResource(name)
{
	return new Resource(name);
}

Object.freeze(CertificateManager.prototype);

export default CertificateManager;


/*
function dumpHex(label, values, width)
{
	values = new Uint8Array(values);

	trace(label + ", " + values.length + " bytes:");

	if (!width) width = 16;

	for (let i = 0; i < values.length; i++) {
		if (0 == (i % width))
			trace("\n");

		let str = values[i].toString(16);
		if (1 == str.length)
			str = "0" + str;
		trace(str + ":");
	}

	trace("\n");
}

*/
