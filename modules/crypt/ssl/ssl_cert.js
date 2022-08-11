/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
import ECDSA from "ecdsa";
import Bin from "bin";
import BER from "ber";
import PKCS8 from "pkcs8"

class CertificateManager {
	#verify;
	#registeredCerts = [];
	#clientCertificates;
	#clientKey;

	constructor(options) {
		this.#verify = options.verify ?? true;
		if (options.certificate)
			this.register(options.certificate);
		this.#clientCertificates = options.clientCertificates;
		this.#clientKey = options.clientKey;
	}
	getCerts() {
		// return the self certs
		return [getResource("srvcert.der")];
	}
	getKey(cert) {
		// look for the key corresponding to the cert
		// first, search in the registed cert
		for (let i = 0; i < this.#registeredCerts.length; i++) {
			if (0 === Bin.comp(this.#registeredCerts[i], cert)) {
				const x509 = X509.decode(this.#registeredCerts[i]);
				return x509.spki;	// public key only
			}
		}

		// at the moment there is only one key
		if (this.#clientCertificates[0]) {
			if (0 === Bin.comp(this.#clientCertificates[0], cert))
				return PKCS8.parse(new Uint8Array(this.#clientKey));
		}
	}
	findPreferredCert(types, names) {
		return this.#clientCertificates ?? [];
	}
	getIndex(name, target) {
		if (!target)
			return -1;

		const data = new Resource(name);
		for (let i = 0; ((i + 1) * 20) <= data.byteLength; i++) {
			if (!Bin.comp(data.slice(i * 20, (i + 1) * 20, false), target))
				return i;
		}
		return -1;
	}
	findCert(fname, target) {
		let data = this.getIndex(fname, target);
		if (data < 0)
			return;	// undefined
		data = getResource("ca" + data + ".der");
		if (this.#verify) {
			const validity = X509.decodeTBS(X509.decode(new Uint8Array(data)).tbs).validity;
			const now = Date.now();
			if (!((validity.from < now) && (now < validity.to))) {
				trace("date validation failed on certificate resource\n");
				return;
			}
		}

		return X509.decodeSPKI(data);
	}
	verify(certs) {
		if (!this.#verify)
			return true;

		let length = certs.length - 1, x509, validity, now = Date.now(), spki;

		// this approach calls decodeSPKI once more than necessary in favor of minimizing memory use
		for (let i = 0; i < length; i++) {
			x509 = X509.decode(certs[i]);
			validity = X509.decodeTBS(x509.tbs).validity;
			if (!((validity.from < now) && (now < validity.to))) {
				trace("date validation failed on received certificate\n");
				continue;
			}

			if (!this._verify(X509.decodeSPKI(certs[i + 1]), x509))
				continue;
			x509 = undefined;

			let aki = X509.decodeAKI(certs[i + 1]);
			for (let j = 0; j < this.#registeredCerts.length; j++) {
				if (Bin.comp(X509.decodeSKI(this.#registeredCerts[j]), aki) == 0) {
					spki = X509.decodeSPKI(this.#registeredCerts[j]);
					if (spki && this._verify(spki, X509.decode(certs[i + 1])))
						return true;
					spki = undefined;
				}
			}

			spki = this.findCert("ca.ski", aki);
			aki = undefined;
			if (spki && this._verify(spki, X509.decode(certs[i + 1])))
				return true;
				// else fall thru
			spki = undefined;
		}

		x509 = X509.decode(certs[length]);
		validity = X509.decodeTBS(x509.tbs).validity;
		if (!((validity.from < now) && (now < validity.to))) {
			trace("date validation failed\n");
			return false;
		}

		spki = this.findCert("ca.ski", X509.decodeAKI(certs[length]));
		if (spki && this._verify(spki, x509))
			return true;
			// else fall thru

		for (let i = 0; i < this.#registeredCerts.length; i++) {
			spki = X509.decodeSPKI(this.#registeredCerts[i]);
			if (spki && this._verify(spki, x509))
				return true;
		}

		return false;
	}

	_verify(spki, x509) {
		let pk, hash;
		let sig = x509.sig;

		switch (x509.algo.toString()) {
		case "1,2,840,113549,1,1,4":	// PKCS-1 MD5 with RSA encryption
			hash = "MD5";
			pk = PKCS1_5;
			break;
		case "1,2,840,113549,1,1,5":	// PKCS-1 SHA1 with RSA encryption
			hash = "SHA1";
			pk = PKCS1_5;
			break;
		case "1,2,840,113549,1,1,11":	// PKCS-1 SHA256 with RSA encryption
			hash = "SHA256";
			pk = PKCS1_5;
			break;
		case "1,2,840,113549,1,1,12":	// PKCS-1 SHA384 with RSA encryption
			hash = "SHA384";
			pk = PKCS1_5;
			break;
		case "1,2,840,113549,1,1,13":	// PKCS-1 SHA512 with RSA encryption
			hash = "SHA512";
			pk = PKCS1_5;
			break;
		case "1,2,840,113549,1,1,14":	// PKCS-1 SHA224 with RSA encryption
			hash = "SHA224";
			pk = PKCS1_5;
			break;
		case "1,2,840,10040,4,3":
		case "1,3,14,3,2,27": {
			hash = "SHA1";
			pk = DSA;
			// need to decode the sig value into <r, s>
			const ber = new BER(x509.sig);
			if (ber.getTag() !== 0x30)
				throw new Error;
			ber.getLength();
			sig = (ArrayBuffer.fromBigInt(ber.getInteger())).concat(ArrayBuffer.fromBigInt(ber.getInteger()));				 
			} break;
		case "1,2,840,10045,4,3,1":
		case "1,2,840,10045,4,3,2":
		case "1,2,840,10045,4,3,3":
		case "1,2,840,10045,4,3,4": {	// ECDSA with SHA224, SHA256, SHA384, SHA512
			hash = ["SHA224", "SHA256", "SHA384", "SHA512"][x509.algo[6] - 1];
			pk = ECDSA;
			// need to decode the sig value into <r, s>
			const ber = new BER(sig);
			if (ber.getTag() !== 0x30)
				throw new Error;
			ber.getLength();
			sig = (ArrayBuffer.fromBigInt(ber.getInteger())).concat(ArrayBuffer.fromBigInt(ber.getInteger()));
			return (new pk(spki.pub, spki.curve, false, [] /* any oid */)).verify((new Crypt.Digest(hash)).process(x509.tbs), sig);
			} break;
		default:
			throw new Error("Cert: unsupported algorithm: " + x509.algo.toString());
		}
		return (new pk(spki, false, [] /* any oid */)).verify((new Crypt.Digest(hash)).process(x509.tbs), sig);
	}
	register(cert) {
		if (this.#verify) {
			let validity = X509.decodeTBS(X509.decode(new Uint8Array(cert)).tbs).validity, now = Date.now();
			if (!((validity.from < now) && (now < validity.to)))
				throw new Error("date validation failed");
		}

		this.#registeredCerts.push(cert);
	}
	getDH() {
		let dh = getResource("dh.der");
		let ber = new BER(dh);
		if (ber.getTag() == 0x30) {
			ber.getLength();
			let p = ber.getInteger();
			let g = ber.getInteger();
			return {p, g};
		}
	}
}

function getResource(name)
{
	return new Resource(name);
}

export default CertificateManager;
