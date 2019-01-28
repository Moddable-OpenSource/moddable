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
 */

#include "xs.h"
#include "xsesp.h"
#include "malloc.h"
#include <string.h>

// Utility functions for generating byte arrays formatted as various MQTT 3.1.1 messages.
// See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
// Note particularly the distinction between "fixed header", per-message-type "variable header", and
// "payload".

// This implementation is basic. It supports only publish, subscribe, and ping operations with
// QoS 0 over clean sessions. It does not support QoS 1 or 2 and concomitant publication ACKs, LWT
// messages, Retained messages, or resumed client sessions. However, note that many key server
// implementations (esp. AWS) don't support QoS 1 or 2, and that QoS 2 in particular specifies
// guaranteed-delivery behavior that is not actually possible to achieve in real systems, anyway.
// Meanwhile LWT and Retained messages, while handy, are easily replaceable by application code.
// So the limitations in this library are not as important as they might seem.
//
// That said, note that it would be relatively easy to upgrade this code to support some or all of
// these features, if desired.

typedef enum {
	RESERVED    = 0x00,
	CONNECT     = 0x10,
	CONNACK     = 0x20,
	PUBLISH     = 0x30,
	PUBACK      = 0x40, // unimplemented
	PUBREC      = 0x50, // unimplemented
	PUBREL      = 0x60, // unimplemented
	PUBCOMP     = 0x70, // unimplemented
	SUBSCRIBE   = 0x82, // == 0x80 | 0x02 -- message ID or-ed with required flag
	SUBACK      = 0x90, // unimplemented
	UNSUBSCRIBE = 0xA2, // == 0xA0 | 0x02 -- as above, incl. required flag
	UNSUBACK    = 0xB0, // unimplemented
	PINGREQ     = 0xC0,
	PINGRESP    = 0xD0,
	DISCONNECT  = 0xE0,
} msg_type;

#define PING_INTERVAL 5

// VLQ is a name someone made up for the ints-as-7-bit-octets-where-high-bit-indicates-more-octets int format
inline int to_vlq(uint8_t* buf, int size, uint32_t value) {
	int i = 0;
	for (int i = 0; value; ++i) {
		if (i > size)
			return -1;
		buf[i] = 0x7f & value;
		value >>= 7;
		if (value)
			buf[i] |= 0x80;
		else
			return (i + 1);
	}
}

inline size_t from_vlq(uint8_t *buf, uint32_t *n, size_t size) {
	*n = 0;
	size = size > 4 ? 4 : size;

	for (size_t i = 1; i <= size;) {
		*n |= ((*buf) & 0x7f);
		if ((*buf) & 0x80) {
			*n <<= 7;
			buf++;
			i++;
		} else {
			return i;
		}
	}

	*n = 0;
	return -1;
}

inline uint8_t* mqtt_string(uint8_t *in, size_t len) {
	uint8_t *out = calloc(1, len + 2);
	if (!out) {
		return NULL;
	}
	out[0] = (len >> 8) & 0xff;
	out[1] = len & 0xff;
	uint8_t buf[32];
	tfp_snprintf(buf, 32, "booga %x %x %d\n", out + 2, in, len);
	espMemCpy(out + 2, in, len);
	return out;
}

void mqtt_connect_msg(xsMachine* the) {
	// MQTT connect messages start with a standard(ish) header block
	static const uint8_t hdr[] ICACHE_XS6RO_ATTR = {
		0x00,0x04,'M','Q','T','T',	      // protocol name MQTT
		0x04,				   // protocol level 4
		0x02,				   // flags : CleanSession
		0x00, 0x00			      // no keepalive -- never drop on inactivity
	};
	size_t hdr_len = sizeof(hdr);

	// ...followed by the client ID (string)
	uint8_t* name;
	size_t name_len;
	if (xsStringType == xsTypeOf(xsArg(0))) {
		uint8_t* xsname = xsToString(xsArg(0));
		name_len = espStrLen(xsname);
		name = mqtt_string(xsname, name_len);
		if (!name) {
			xsUnknownError("mqtt: error allocating client ID in connect");
			return;
		}
		name_len += 2;
	} else {
		xsUnknownError("mqtt: client ID must be a string");
		return;
	}

	// now we know how long our payload will be, so prepare the final buffer
	size_t payload_len = hdr_len + name_len;
	uint8_t buf[512];
	size_t count = 1;
	buf[0] = CONNECT;

	// MQTT payload length header is variable length
	count += to_vlq(buf + 1, 511, payload_len);
	if (!count) { // i.e. if to_vlq() returned -1 on error
		if (name) free(name);
		xsUnknownError("mqtt: error encoding payload length");
		return;
	}

	// don't blow the stack
	if (count + payload_len > sizeof(buf)) {
		if (name) free(name);
		xsUnknownError("mqtt: payload buffer overflow creating connect message");
		return;
	}

	// memcpy the payload fragments into buf
	espMemCpy(buf + count, hdr, hdr_len);
	count += hdr_len;
	espMemCpy(buf + count, name, name_len);
	count += name_len;

	// tell XS6 to use that as the return value.
	xsResult = xsArrayBuffer(buf, count);

	if (name) free(name);
	// hdr & buf are on the stack and xsArrayBuffer is a malloc + memcpy, so no free() for these
}

void mqtt_publish_msg(xsMachine* the) {
	uint8_t *topic, *data;
	size_t topic_len, data_len;
	int id;

	if (xsStringType == xsTypeOf(xsArg(0))) {
		topic = xsToString(xsArg(0));
		topic_len = espStrLen(topic);
		topic = mqtt_string(topic, topic_len);
		if (!topic) {
			xsUnknownError("mqtt: error allocating topic in publish");
			return;
		}
		topic_len += 2;
	} else {
		xsUnknownError("mqtt: publish topic must be a string");
		return;
	}

	if (xsStringType == xsTypeOf(xsArg(1))) {
		data = xsToString(xsArg(1));
		data_len = espStrLen(data);
	} else {
		data = xsToArrayBuffer(xsArg(1));
		data_len = xsGetArrayBufferLength(xsArg(1));
	}

	if (xsIntegerType == xsTypeOf(xsArg(2))) {
		id = 0x0000ffff & xsToInteger(xsArg(2));
	} else {
		if (topic) free(topic);
		xsUnknownError("mqtt: publish packet id missing or malformed");
		return;
	}

	// prep some RAM to copy stuff into. can't use stack here b/c we don't know how big this will be
	uint8_t* msg = calloc(1, 1 + 4 + topic_len + 2 + data_len); // note: not all of this may be used
	if (!msg) {
		if (topic) free(topic);
		xsUnknownError("mqtt: OOM calloc()ing publish buffer");
		return;
	}

	// compute size of message we're about to send
	size_t count = 1;
	msg[0] = PUBLISH;
	count += to_vlq(msg + 1, 100, topic_len + data_len); // would need +2 more if we were QoS > 0
	if (!count) {
		if (topic) free(topic);
		if (msg) free(msg);
		xsUnknownError("mqtt: error recording publish message length");
		return;
	}

	// copy in topic name; note this is in "variable header" which is why it precedes packet ID vs. SUBSCRIBE
	espMemCpy(msg + count, topic, topic_len);
	count += topic_len;

	// copy in packet ID to end of variable header; but only if QoS > 0 which we don't support
	// msg[count++] = (uint8_t)(id >> 8);
	// msg[count++] = (uint8_t)id & 0xff;

	// actual payload bytes
	espMemCpy(msg + count, data, data_len);
	count += data_len;

	// tell XS6 to use msg as the return value.
	xsResult = xsArrayBuffer(msg, count);
	if (topic) free(topic);
	if (msg) free(msg);
}

void mqtt_subscribe_msg(xsMachine* the) {
	// fetch the topic string, first arg
	uint8_t *topic;
	size_t topic_len;
	if (xsStringType == xsTypeOf(xsArg(0))) {
		topic = xsToString(xsArg(0));
		topic_len = espStrLen(topic);

		topic = mqtt_string(topic, topic_len);

		if (!topic) {
			xsUnknownError("mqtt: error allocating topic in subscribe");
			return;
		}
		topic_len += 2;
	} else {
		xsUnknownError("mqtt: topic to subscribe to must be a string");
		return;
	}

	// fetch the packet ID, second arg
	int id;
	if (xsIntegerType == xsTypeOf(xsArg(1))) {
		id = 0x0000ffff & xsToInteger(xsArg(1));
	} else {
		xsUnknownError("mqtt: missing or malformed packet ID");
	}

	// format up the bytes
	uint8_t msg[512];
	size_t count = 1;
	msg[0] = SUBSCRIBE;

	// MQTT payload length header is variable length
	count += to_vlq(msg + 1, 511, topic_len + 3); // 2 bytes for packet ID, 1 byte for QoS at end
	if (!count) { // i.e. if to_vlq() returned -1 on error
		if (topic) free(topic);
		xsUnknownError("mqtt: error encoding payload length");
		return;
	}

	if (count + topic_len + 3 > sizeof(msg)) {
		if (topic) free(topic);
		xsUnknownError("mqtt: payload buffer overflow creating subscribe message");
		return;
	}

	msg[count++] = (uint8_t)(id >> 8);
	msg[count++] = (uint8_t)id & 0xff;
	espMemCpy(msg + count, topic, topic_len);
	count += topic_len;
	msg[count++] = 0;

	// tell XS6 to use that as the return value.
	xsResult = xsArrayBuffer(msg, count);
	if (topic) free(topic);
}

void mqtt_unsubscribe_msg(xsMachine* the) {
	uint8_t *topic;
	size_t topic_len;
	int id;

	if (xsStringType == xsTypeOf(xsArg(0))) {
		topic = xsToString(xsArg(0));
		topic_len = espStrLen(topic);
		topic = mqtt_string(topic, topic_len);
		if (!topic) {
			xsUnknownError("mqtt: error allocating topic in unsubscribe");
			return;
		}
		topic_len += 2;
	} else {
		xsUnknownError("mqtt: topic to unsubscribe to must be a string");
		return;
	}

	if (xsIntegerType == xsTypeOf(xsArg(1))) {
		id = 0x0000ffff & xsToInteger(xsArg(1));
	} else {
		if (topic) free(topic);
		xsUnknownError("mqtt: missing or malformed packet ID in unsubscribe");
		return;
	}

	// set header and add varint/VLQ message length
	uint8_t msg[512];
	size_t count = 1;
	msg[0] = UNSUBSCRIBE;
	count += to_vlq(msg + 1, 511, 2 + topic_len);
	if (!count) {
		if (topic) free(topic);
		xsUnknownError("mqtt: error formatting message length in unsubscribe");
		return;
	}

	// packet ID; end of variable header
	msg[count++] = (uint8_t)(id >> 8);
	msg[count++] = (uint8_t)id & 0xff;

	// topic ID
	espMemCpy(msg + count, topic, topic_len);
	count += topic_len;

	xsResult = xsArrayBuffer(msg, count);
	if (topic) free(topic);
}

void mqtt_ping_msg(xsMachine* the) {
	static const uint8_t hdr[] ICACHE_XS6RO_ATTR = { PINGREQ, 0x00 };
	xsResult = xsArrayBuffer((void *)hdr, sizeof(hdr));
}

void mqtt_close_msg(xsMachine* the) {
	static const uint8_t hdr[] ICACHE_XS6RO_ATTR = { DISCONNECT, 0x00 };
	xsResult = xsArrayBuffer((void *)hdr, sizeof(hdr));
}

void mqtt_decode_msg(xsMachine* the) {
	uint8_t *msg = xsToArrayBuffer(xsArg(0));
	uint32_t code = msg[0] & 0xf0;

	uint8_t *topic, *data_s, *data_b;
	size_t topic_len, data_len;

	xsResult = xsNewObject();

	uint32_t payload_len;
	uint8_t *p;
	size_t skip;
	uint8_t qos;
	switch (code) {
		case PUBLISH: // this is the big one
			qos = msg[0] & 0x06;
			p = (msg + 1);
			skip = from_vlq(p, &payload_len, xsGetArrayBufferLength(xsArg(0)) - 1);
			if (skip < 1) {
				xsUnknownError("mqtt: malformed publish packet from server");
				return;
			}
			p += skip;

			// decode length of topic (which must be next) from next 2 bytes
			topic_len = ((*(p++)) << 8);
			topic_len |= *(p++);
			payload_len -= 2;

			xsSet(xsResult, xsID("topic"), xsStringBuffer(p, topic_len));
			p += topic_len;
			payload_len -= topic_len;

			if (qos) {
				p += 2; // skip 2-byte packet identifier which we don't use
				payload_len -= 2;
			}

			xsSet(xsResult, xsID("data"), xsArrayBuffer(p, payload_len));
			xsSet(xsResult, xsID("code"), xsInteger(code));

			return;

		case CONNACK: // we surface this just so the ES6 class can see flags set by server
			xsSet(xsResult, xsID("returnCode"), xsInteger(msg[3]));
			xsSet(xsResult, xsID("code"), xsInteger(code));
			return;

		case PUBACK:
		case PUBREC:
		case PUBREL:
		case PUBCOMP:
		case SUBACK:
		case UNSUBACK:
		case PINGRESP:
			// we ignore these b/c we only do QoS 0 & don't do callbacks for sub & ping ACKs & such,
			// but let the caller know they happened anyway
			xsSet(xsResult, xsID("code"), xsInteger(0));
			return;

		case CONNECT:
		case PINGREQ:
		case DISCONNECT:
		case SUBSCRIBE:
		case UNSUBSCRIBE:
			// per spec these are client->server only; we'll never get these
			xsUnknownError("mqtt: server sent us a client-only message?!");
			return;

		default:
			xsUnknownError("mqtt: server sent an unrecognized message");
			return;
	}
}

void mqtt_array_to_string(xsMachine* the) {
	if (xsStringType == xsTypeOf(xsArg(0))) {
		xsResult = xsArg(0);
	} else {
		xsResult = xsStringBuffer(xsToArrayBuffer(xsArg(0)), xsGetArrayBufferLength(xsArg(0)));
	}
}

