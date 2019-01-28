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

import RNG from "rng";

let Crypt = {
	get Digest() {
		throw new Error("fix me");
	},
	get SHA1() {
		throw new Error("fix me");
	},
	get SHA256() {
		throw new Error("fix me");
	},
	get SHA512() {
		throw new Error("fix me");
	},
	get SHA224() {
		throw new Error("fix me");
	},
	get SHA384() {
		throw new Error("fix me");
	},
	get MD5() {
		throw new Error("fix me");
	},
	get BlockCipher() {
		throw new Error("fix me");
	},
	get AES() {
		throw new Error("fix me");
	},
	get DES() {
		throw new Error("fix me");
	},
	get TDES() {
		throw new Error("fix me");
	},
	get StreamCipher() {
		throw new Error("fix me");
	},
	get RC4() {
		throw new Error("fix me");
	},
	get Chacha() {
		throw new Error("fix me");
	},
	get Mode() {
		throw new Error("fix me");
	},
	get CBC() {
		throw new Error("fix me");
	},
	get CTR() {
		throw new Error("fix me");
	},
	get ECB() {
		throw new Error("fix me");
	},
	get HMAC() {
		throw new Error("fix me");
	},
	get HKDF() {
		throw new Error("fix me");
	},
	get RSA() {
		throw new Error("fix me");
	},
	get DSA() {
		throw new Error("fix me");
	},
	get ECDSA() {
		throw new Error("fix me");
	},
	get PKCS1() {
		throw new Error("fix me");
	},
	get PKCS1_5() {
		throw new Error("fix me");
	},
	get PKCS8() {
		throw new Error("fix me");
	},
	get OAEP() {
		throw new Error("fix me");
	},
	get Curve25519() {
		throw new Error("fix me");
	},
	get Ed25519() {
		throw new Error("fix me");
	},
	get Poly1305() {
		throw new Error("fix me");
	},
	get AEAD() {
		throw new Error("fix me");
	},
	get SRPServer() {
		throw new Error("fix me");
	},
	get BER() {
		throw new Error("fix me");
	},
	get X509() {
		throw new Error("fix me");
	},
	rng(n) {
		return RNG.get(n);
	},
};
export default Crypt;

Object.freeze(Crypt);

export default Crypt;
