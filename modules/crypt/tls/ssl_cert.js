/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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
import X509 from "ssl/x509";
import Bin from "bin";
import BER from "ber";
import Key from "embedded:crypt/key";
import Signature from "embedded:crypt/signature";
import Digest from "embedded:crypt/digest";
import TLSError from "ssl/error";

const globalCerts = [];
const curveByOID = Object.freeze({"1,2,840,10045,3,1,7": "P-256", "1,3,132,0,34": "P-384"});

// X.509 and TLS carry ECDSA signatures as DER SEQUENCE {r, s}; Signature uses r || s of the curve's size
function digestOf(algorithm, data) {		// digest of data; the Digest is closed before this returns, so its context is not live during the verification
	const digest = new Digest({algorithm});
	digest.write(data);
	const result = digest.read();
	digest.close();
	return result;
}

export function ecdsaSignatureToRaw(der, key) {		// r || s, each the size of the key's curve; before a Signature is constructed, so a malformed signature throws without leaving one open
	const ber = new BER(der);
	if (0x30 !== ber.getTag())
		throw new Error("invalid ECDSA signature");
	ber.getLength();
	const curve = key.namedCurve;
	const half = ("P-384" === curve) ? 48 : (("P-521" === curve) ? 66 : 32);
	const result = new Uint8Array(half << 1);
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(ber.getInteger(), half)));
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(ber.getInteger(), half)), half);
	return result;
}

const basicConstraints = Object.freeze([2, 5, 29, 19]);

function isCA(cert) {		// basicConstraints cA (RFC 5280 section 4.2.1.9): SEQUENCE { cA BOOLEAN DEFAULT FALSE, pathLenConstraint INTEGER OPTIONAL }
	const extension = X509.decodeExtension(cert, basicConstraints);
	if (!extension)
		return false;
	const ber = new BER(new BER(extension).getSequence());
	if (!ber.readable || (0x01 !== ber.getTag()))
		return false;
	ber.getLength();
	return 0 !== ber.getc();
}

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
	getKey(cert) {
		return new Key(this.keyOptions(cert));
	}
	keyOptions(cert) {		// Key options; the caller constructs the Key so the parse (the handshake's stack peak) runs one frame shallower
		if (cert) {		// peer's public key: RSA, or EC on a named curve
			const spk = X509.getSPK(cert);
			if ("1,2,840,113549,1,1,1" === spk.algo.toString())
				return {algorithm: "RSA", format: "pkcs1", data: spk};
			if ("1,2,840,10045,2,1" !== spk.algo.toString())
				throw new Error("unsupported public key algorithm");
			const namedCurve = curveByOID[new BER(spk.param).getObjectIdentifier().toString()];
			if (!namedCurve || !Key.supports({algorithm: "EC", namedCurve}))
				throw new TLSError(`certificate curve ${namedCurve ?? "unknown"} is not supported by this build`);
			return {algorithm: "EC", format: "raw", namedCurve, data: spk};
		}

		// our private key: PKCS#8 PrivateKeyInfo or PKCS#1 RSAPrivateKey, told apart by the element after the version
		const der = this.#clientKey;
		const ber = new BER(der);
		ber.getTag();
		ber.getLength();
		ber.getInteger();
		return {algorithm: "RSA", format: (0x30 === ber.getTag()) ? "pkcs8" : "pkcs1", data: der};
	}
	findPreferredCert() {
		return this.#clientCertificates ?? [];
	}
	getIndex(name, target) {
		if (!target)
			return -1;

		const data = new Resource(name);
		for (let i = 0, byteLength = data.byteLength; i < byteLength; i += 20) {
			if (!Bin.comp(data.slice(i, i + 20, false), target))
				return i / 20;
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

		return data;
	}
	verify(certs, options) {
		if (!this.#verify)
			return true;

		const length = certs.length;
		const tls_server_name = options.tls_server_name?.toLowerCase();
		if (!length || !tls_server_name) {
			trace(`certificate and tls_server_name required\n`);
			return false;
		}

		// the leaf certificate names the host: a dNSName (one wildcard, in place of the leftmost label) or an IPv4 iPAddress
		const names = X509.decodeSAN(certs[0]);
		let match = false;
		for (let j = 0; j < names?.length; j++) {
			let name = names[j];
			if (2 === name.tag) {			// dNSName
				name = name.value.toLowerCase();
				if (42 === name.charCodeAt(0)) {		// wildcard ("*")
					if (name.indexOf("*", 1) > 0)	// only one wild card
						continue;
					let position = tls_server_name.indexOf(".");
					if (position < 0) continue;
					match = name.slice(1) === tls_server_name.slice(position);
				}
				else
					match = name === tls_server_name;
			}
			else if (7 === name.tag) {		// iPAddress
				name = name.value;
				if (4 !== name.byteLength) continue;		// only handling IPv4 for now
				name = (new Uint8Array(name)).join(".");
				match = name === tls_server_name;
			}
			if (match) break;
		}
		if (!match) {
			trace(`subjectAltName match failed for ${tls_server_name}\n`);
			return false;
		}

		// every certificate is within its validity period and is signed either by a trusted certificate, which ends the chain,
		// or by the certificate that follows it, which must be a CA (RFC 5280 sections 6.1 and 4.2.1.9). The trusted certificates
		// are those registered (the certificate option) and the bundled roots, both found by the key identifier they sign with.
		const registeredCerts = this.#registeredCerts.concat(globalCerts);
		const now = Date.now();
		let x509, validity, issuer, aki;
		for (let i = 0; i < length; i++) {
			x509 = X509.decode(certs[i]);
			validity = X509.decodeTBS(x509.tbs).validity;
			if (!((validity.from < now) && (now < validity.to))) {
				trace("date validation failed on received certificate\n");
				return false;
			}

			aki = X509.decodeAKI(certs[i]);
			if (aki) {
				for (let j = 0; j < registeredCerts.length; j++) {
					if (0 === Bin.comp(X509.decodeSKI(registeredCerts[j]), aki)) {
						issuer = registeredCerts[j];
						break;
					}
				}
				if (!issuer)
					issuer = this.findCert("ca.ski", aki);
				aki = undefined;
			}
			if (issuer && this._verify(new Key(this.keyOptions(issuer)), x509))
				return true;		// signed by a trusted certificate
			issuer = undefined;

			if ((i + 1) < length) {
				if (!isCA(certs[i + 1])) {
					trace("issuer is not a certificate authority\n");
					return false;
				}
				if (!this._verify(new Key(this.keyOptions(certs[i + 1])), x509)) {
					trace("certificate not signed by its issuer\n");
					return false;
				}
				continue;
			}

			// the last certificate: a registered certificate may have signed it, or be it, without a key identifier to find it by
			for (let j = 0; j < registeredCerts.length; j++) {
				if (this._verify(new Key(this.keyOptions(registeredCerts[j])), x509))
					return true;
			}
		}

		trace("certificate chain does not end at a trusted certificate\n");
		return false;
	}

	_verify(key, x509) {		// the key is built by the caller, and locals are few: the signature verification beneath is the handshake's stack peak
		let hash;
		switch (x509.algo.toString()) {
		case "1,2,840,113549,1,1,4": hash = "MD5"; break;		// PKCS-1 with RSA encryption
		case "1,2,840,113549,1,1,5": hash = "SHA-1"; break;
		case "1,2,840,113549,1,1,11": hash = "SHA-256"; break;
		case "1,2,840,113549,1,1,12": hash = "SHA-384"; break;
		case "1,2,840,113549,1,1,13": hash = "SHA-512"; break;
		case "1,2,840,113549,1,1,14": hash = "SHA-224"; break;
		case "1,2,840,10045,4,3,1": hash = "SHA-224"; break;		// ECDSA with SHA224, SHA256, SHA384, SHA512
		case "1,2,840,10045,4,3,2": hash = "SHA-256"; break;
		case "1,2,840,10045,4,3,3": hash = "SHA-384"; break;
		case "1,2,840,10045,4,3,4": hash = "SHA-512"; break;
		default:
			throw new Error("Cert: unsupported algorithm: " + x509.algo.toString());
		}
		const digest = digestOf(hash, x509.tbs);		// what can throw comes before the Signature, which holds a key until closed
		const raw = ("EC" === key.algorithm) ? ecdsaSignatureToRaw(x509.sig, key) : x509.sig;
		const signature = ("EC" === key.algorithm) ? new Signature({algorithm: "ECDSA", key}) : new Signature({algorithm: "RSASSA-PKCS1-v1_5", key, hash});
		hash = signature.verify(digest, raw);
		signature.close();
		key.close();
		return hash;
	}
	register(cert) {
		if (this.#verify) {
			let validity = X509.decodeTBS(X509.decode(new Uint8Array(cert)).tbs).validity, now = Date.now();
			if (!((validity.from < now) && (now < validity.to)))
				throw new Error("date validation failed");
		}

		this.#registeredCerts.push(cert);
	}
	static register(cert) {
		for (let i = 0; i < globalCerts.length; i++) {
			if (!Bin.comp(globalCerts[i], cert))
				return;
		}
		globalCerts.push(cert);
	}
}

function getResource(name)
{
	return new Resource(name);
}

export default CertificateManager;
