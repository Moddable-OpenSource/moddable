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

import recordProtocol from "ssl/record";
import SetupCipher from "ssl/setup";
import SSLStream from "ssl/stream";
import Arith from "arith";

// type
const change_cipher_spec = 1;

let changeCipherSpec = {
	name: "changeCipherSpec",
	unpacketize(session, fragment) {
		session.traceProtocol(this);
		var type = (new SSLStream(fragment)).readChar();
		if (type == change_cipher_spec) {
			session.readSeqNum = new Arith.Integer(0);	// the specification is very ambiguous about the sequence number...
			return SetupCipher(session, !session.connectionEnd);		// tail call optimization
		}
		else
			throw new Error("SSL: changeCipherSpec: bad type");
	},
	packetize(session, type) {
		session.traceProtocol(this);
		var s = new SSLStream();
		if (type === undefined) type = change_cipher_spec;
		s.writeChar(type);
		var upper = recordProtocol.packetize(session, recordProtocol.change_cipher_spec, s.getChunk());
		session.writeSeqNum = new Arith.Integer(0);	// the specification is very ambiguous about the sequence number...
		SetupCipher(session, session.connectionEnd);
		return upper;
	},
};

Object.freeze(changeCipherSpec);

export default changeCipherSpec;
