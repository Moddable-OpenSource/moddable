/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {Digest} from "crypt";
import Base64 from "base64";

// sample WebSocket handshake hash
let sha1 = new Digest("SHA1");
sha1.write("dGhlIHNhbXBsZSBub25jZQ==");
sha1.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
let result = Base64.encode(sha1.close());

trace(`Calculated hash: ${result}\n`);

let expect = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
trace(`Expected hash: ${expect}\n`);

if (result == expect)
	trace("PASS\n");
else
	trace("FAIL\n");
