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

import SSLStream from "ssl/stream";
import MAC from "embedded:crypt/MAC";
import {SHA384} from "ssl/constants";

function iceil(x, y)
{
	return Math.idiv(x, y) + (Math.irem(x, y) ? 1 : 0);
}

function p_hash(hash, secret, seed, sz)
{
	const hmac = new MAC({algorithm: "HMAC", hash, key: secret});
	sz = iceil(sz, hmac.outputSize);
	hmac.write(seed);
	let A = hmac.read();		// A(1) = HMAC(seed)
	hmac.reset();
	let p = new SSLStream;
	while (--sz >= 0) {
		hmac.write(A);
		hmac.write(seed);
		p.writeChunk(hmac.read());
		hmac.reset();
		if (sz > 0) {
			hmac.write(A);
			A = hmac.read();
			hmac.reset();
		}
	}
	hmac.close();
	return p.getChunk();
}

function PRF(session, secret, label, seed, n)
{
	const hash = (SHA384 === session.chosenCipher.hashAlgorithm) ? "SHA-384" : "SHA-256";
	const result = p_hash(hash, secret, ArrayBuffer.fromString(label).concat(seed), n);
	result.buffer.resize(n);		// p_hash > getChunk returns resizable buffer
	return result;
}

export default PRF;
