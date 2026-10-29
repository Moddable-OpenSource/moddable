/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
import SSLStream from "ssl/stream";
import TLSError from "ssl/error";

//const warning = 1;
//const fatal = 2;
const close_notify = 0;
//const unexpected_message = 10;
//const bad_record_mac = 20;
//const decryption_failed = 21;
//const record_overflow = 22;
//const decompression_failure = 30;
//const handshake_failure = 40;
//const bad_certificate = 42;
//const unsupported_certificate = 43;
//const certificate_revoked = 44;
//const certificate_expired = 45;
//const certificate_unknown = 46;
//const illegal_parameter = 47;
//const unknown_ca = 48;
//const access_denied = 49;
//const decode_error = 50;
//const decrypt_error = 51;
//const export_restriction = 60;
//const protocol_version = 70;
//const insufficient_security = 71;
//const internal_error = 80;
//const user_canceled = 90;
//const no_negotiation = 100;
//const unsupported_extension = 110;           /* new */
//const certificate_unobtainable = 111;        /* new */
//const unrecognized_name = 112;               /* new */
//const bad_certificate_status_response = 113; /* new */
//const bad_certificate_hash_value = 114;      /* new */

const alert = {
	name: "alert",
	// global constants
	close_notify: close_notify,

	unpacketize(session, fragment) {
		session.traceProtocol(this);
		const s = new SSLStream(fragment);
		session.alert = {level: s.readChar(), description: s.readChar()};
		if (close_notify !== session.alert.description)
			throw new TLSError("alert: " + session.alert.level + ", " + session.alert.description);
		trace("SSL: close notify\n");
	},
	packetize(session, level, description) {
		session.traceProtocol(this);

		const s = new SSLStream();
		s.writeChar(level);
		s.writeChar(description);
		return recordProtocol.packetize(session, recordProtocol.alert, s.getChunk());
	},
};

Object.freeze(alert);

export default alert;
