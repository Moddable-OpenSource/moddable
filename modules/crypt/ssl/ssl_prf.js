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
import Bin from "bin";
import SSLStream from "ssl/stream";
import HMAC from "hmac";
import {SHA384} from "ssl/constants";

function iceil(x, y)
{
	return Math.idiv(x, y) + (Math.irem(x, y) ? 1 : 0);
}

function p_hash(hash, secret, seed, sz)
{
	sz = iceil(sz, hash.outputSize);
	const hmac = new HMAC(hash, secret);
	let A = hmac.process(seed);		// start from A(1) = hmac(seed)
	let p = new SSLStream;
	while (--sz >= 0) {
		p.writeChunk(hmac.process(A, seed));
		if (sz > 0)
			A = hmac.process(A);
	}
	return p.getChunk();
}

function PRF(session, secret, label, seed, n, hash)
{
	let result;
	label = ArrayBuffer.fromString(label).concat(seed);
	if (session.protocolVersion <= 0x302)
		result = Bin.xor(
			p_hash(new Crypt.Digest("MD5"), secret.slice(0, iceil(secret.byteLength, 2)), label, n),
			p_hash(new Crypt.Digest("SHA1"), secret.slice(Math.idiv(secret.byteLength, 2)), label, n)
		);
	else {
		if (!hash)
			hash = session.chosenCipher.hashAlgorithm == SHA384 ? "SHA384" : "SHA256";
		result = p_hash(new Crypt.Digest(hash), secret, label, n);
	}
	return result.slice(0, n);
}

export default PRF;
