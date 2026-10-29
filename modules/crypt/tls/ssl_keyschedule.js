/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

/*
	TLS 1.3 key schedule (RFC 8446 section 7.1) over the ECMA-419 KeyDerivation, Digest and MAC classes

	Every secret is hash-length bytes. The schedule is a chain of HKDF-Extract steps whose salt is the previous
	secret "derived"; traffic secrets, keys, IVs and finished keys are HKDF-Expand-Label of a secret.
*/

import Digest from "embedded:crypt/digest";
import KeyDerivation from "embedded:crypt/keyderivation";
import MAC from "embedded:crypt/MAC";

const prefix = ArrayBuffer.fromString("tls13 ");

// HKDF-Extract(salt, ikm): an absent salt is a string of hash-length zeros, as RFC 5869 specifies
export function hkdfExtract(hash, salt, ikm)
{
	const options = {algorithm: "HKDF-Extract", hash, key: ikm};
	if (salt)
		options.salt = salt;
	const kdf = new KeyDerivation(options);
	const result = kdf.read(hashSize(hash));
	kdf.close();
	return result;
}

// HkdfLabel { uint16 length; opaque label<7..255>; opaque context<0..255> }: the info of HKDF-Expand-Label
export function hkdfLabel(label, context, length)
{
	const labelBytes = ArrayBuffer.fromString(label);
	const contextLength = context ? context.byteLength : 0;
	const info = new Uint8Array(2 + 1 + prefix.byteLength + labelBytes.byteLength + 1 + contextLength);
	let offset = 0;
	info[offset++] = length >> 8;
	info[offset++] = length & 0xff;
	info[offset++] = prefix.byteLength + labelBytes.byteLength;
	info.set(new Uint8Array(prefix), offset);
	offset += prefix.byteLength;
	info.set(new Uint8Array(labelBytes), offset);
	offset += labelBytes.byteLength;
	info[offset++] = contextLength;
	if (contextLength)
		info.set(new Uint8Array(context), offset);
	return info;
}

export function hkdfExpandLabel(hash, secret, label, context, length)
{
	const kdf = new KeyDerivation({algorithm: "HKDF-Expand", hash, key: secret, info: hkdfLabel(label, context, length)});
	const result = kdf.read(length);
	kdf.close();
	return result;
}

// Derive-Secret(secret, label, transcript): the context is the transcript hash
export function deriveSecret(hash, secret, label, transcriptHash)
{
	return hkdfExpandLabel(hash, secret, label, transcriptHash, hashSize(hash));
}

// the traffic key and IV of a traffic secret (section 7.3)
export function trafficKeys(hash, secret, keyLength)
{
	return {
		key: hkdfExpandLabel(hash, secret, "key", undefined, keyLength),
		iv: hkdfExpandLabel(hash, secret, "iv", undefined, 12),
	};
}

// verify_data of a Finished message (section 4.4.4): HMAC(finished_key, transcript hash)
export function finishedVerifyData(hash, baseKey, transcriptHash)
{
	const finishedKey = hkdfExpandLabel(hash, baseKey, "finished", undefined, hashSize(hash));
	const mac = new MAC({algorithm: "HMAC", hash, key: finishedKey});
	mac.write(transcriptHash);
	const result = mac.read();
	mac.close();
	return result;
}

// the hash of an empty transcript, the context of the "derived" secrets
export function emptyHash(hash)
{
	const digest = new Digest({algorithm: hash});
	const result = digest.read();
	digest.close();
	return result;
}

export function hashSize(hash)
{
	return ("SHA-384" === hash) ? 48 : 32;
}

/*
	the chain: early -> handshake -> master, each extracted from the previous secret's "derived" form
*/
export class KeySchedule {
	constructor(hash) {
		this.hash = hash;
		this.secret = undefined;		// the current secret of the chain
	}
	// Early Secret: HKDF-Extract(0, PSK), a PSK of hash-length zeros without one
	early(psk) {
		this.secret = hkdfExtract(this.hash, undefined, psk ?? new Uint8Array(hashSize(this.hash)));
		return this.secret;
	}
	// Handshake Secret: HKDF-Extract(Derive-Secret(early, "derived", ""), (EC)DHE)
	handshake(sharedSecret) {
		this.secret = hkdfExtract(this.hash, deriveSecret(this.hash, this.secret, "derived", emptyHash(this.hash)), sharedSecret);
		return this.secret;
	}
	// Master Secret: HKDF-Extract(Derive-Secret(handshake, "derived", ""), 0)
	master() {
		this.secret = hkdfExtract(this.hash, deriveSecret(this.hash, this.secret, "derived", emptyHash(this.hash)), new Uint8Array(hashSize(this.hash)));
		return this.secret;
	}
	// a traffic secret of the current chain secret: "c hs traffic", "s hs traffic", "c ap traffic", "s ap traffic", "exp master", "res master"
	traffic(label, transcriptHash) {
		return deriveSecret(this.hash, this.secret, label, transcriptHash);
	}
}
