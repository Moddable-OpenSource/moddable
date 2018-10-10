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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include <string.h>
#include <stdint.h>

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
int to_vlq(uint8_t* buf, int size, uint32_t value) {
	for (int i = 0; value; ++i) {
		if (i > size)
			break;
		buf[i] = 0x7f & value;
		value >>= 7;
		if (value)
			buf[i] |= 0x80;
		else
			return (i + 1);
	}
	return -1;
}

/* inline */ size_t from_vlq(uint8_t *buf, uint32_t *n, size_t size) {
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

/* inline */ void *mqtt_string(char *in, size_t len) {
	uint8_t *out = c_malloc(len + 2);
	if (!out)
		return NULL;
	out[0] = (len >> 8) & 0xff;
	out[1] = len & 0xff;
	c_memcpy(out + 2, in, len);
	return out;
}

void mqtt_connect_msg(xsMachine* the) {
	// MQTT connect messages start with a standard(ish) header block
	static const uint8_t hdr[] ICACHE_XS6RO_ATTR = {
		0x00,0x04,'M','Q','T','T',		// protocol name MQTT
		0x04,							// protocol level 4
		0x02,							// flags : CleanSession
		0x00, 0x00						// no keepalive -- never drop on inactivity
	};
	size_t hdr_len = sizeof(hdr);
	char *str;
	uint8_t* id, *user = NULL;
	size_t id_len, user_len = 0, password_len = 0;

	xsmcVars(2);

	// ...followed by the client ID (string)

	xsmcGet(xsVar(0), xsArg(0), xsID_id);
	str = xsmcToString(xsVar(0));
	id_len = c_strlen(str);
	id = mqtt_string(str, id_len);
	if (!id)
		xsUnknownError("mqtt: error allocating client ID in connect");
	id_len += 2;

	if (xsmcHas(xsArg(0), xsID_user)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_user);
		str = xsmcToString(xsVar(0));
		user_len = c_strlen(str);
		user = mqtt_string(str, user_len);
		user_len += 2;
	}

	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(1), xsArg(0), xsID_password);
		password_len = xsGetArrayBufferLength(xsVar(1));
	}

	// now we know how long our payload will be, so prepare the final buffer
	size_t payload_len = hdr_len + id_len + user_len;
	if (password_len)
		payload_len += password_len + 2;
	uint8_t buf[512];
	size_t count = 1;
	buf[0] = CONNECT;

	// MQTT payload length header is variable length
	count += to_vlq(buf + 1, 511, payload_len);
	if (!count) { // i.e. if to_vlq() returned -1 on	 error
		if (id) c_free(id);
		if (user) c_free(user);
		xsUnknownError("mqtt: error encoding payload length");
	}

	// don't blow the stack
	if (count + payload_len > sizeof(buf)) {
		if (id) c_free(id);
		if (user) c_free(user);
		xsUnknownError("mqtt: payload buffer overflow creating connect message");
	}

	// memcpy the payload fragments into buf
	c_memcpy(buf + count, hdr, hdr_len);
	if (user)
		buf[count + 7] |= 0x80;
	if (password_len)
		buf[count + 7] |= 0x40;
	count += hdr_len;
	c_memcpy(buf + count, id, id_len);
	count += id_len;

	if (user) {
		c_memcpy(buf + count, user, user_len);
		count += user_len;
	}

	if (password_len) {
		buf[count++] = password_len >> 8;
		buf[count++] = password_len & 0xff;
		xsmcGetArrayBufferData(xsVar(1), 0, buf + count, password_len);
		count += password_len;
	}

	xsResult = xsArrayBuffer(buf, count);

	if (id) c_free(id);
	if (user) c_free(user);
	// hdr & buf are on the stack and xsArrayBuffer is a malloc + memcpy, so no free() for these
}

void mqtt_publish_msg(xsMachine* the) {
	char *topic, *data;
	size_t topic_len, data_len;
	int id;

	id = 0x0000ffff & xsmcToInteger(xsArg(2));

	topic = xsmcToString(xsArg(0));
	topic_len = c_strlen(topic);
	topic = mqtt_string(topic, topic_len);
	if (!topic)
		xsUnknownError("mqtt: error allocating topic in publish");
	topic_len += 2;

	if (xsStringType == xsmcTypeOf(xsArg(1))) {
		data = xsmcToString(xsArg(1));
		data_len = c_strlen(data);
	} else {
		data = xsmcToArrayBuffer(xsArg(1));
		data_len = xsGetArrayBufferLength(xsArg(1));
	}

	// prep some RAM to copy stuff into. can't use stack here b/c we don't know how big this will be
	uint8_t* msg = c_calloc(1, 1 + 4 + topic_len + 2 + data_len); // note: not all of this may be used
	if (!msg) {
		if (topic) c_free(topic);
		xsUnknownError("mqtt: OOM calloc()ing publish buffer");
	}

	// compute size of message we're about to send
	size_t count = 1;
	msg[0] = PUBLISH;
	count += to_vlq(msg + 1, 100, topic_len + data_len); // would need +2 more if we were QoS > 0
	if (!count) {
		if (topic) c_free(topic);
		if (msg) c_free(msg);
		xsUnknownError("mqtt: error recording publish message length");
	}

	// copy in topic name; note this is in "variable header" which is why it precedes packet ID vs. SUBSCRIBE
	c_memcpy(msg + count, topic, topic_len);
	count += topic_len;

	// copy in packet ID to end of variable header; but only if QoS > 0 which we don't support
	// msg[count++] = (uint8_t)(id >> 8);
	// msg[count++] = (uint8_t)id & 0xff;

	// actual payload bytes
	c_memcpy(msg + count, data, data_len);
	count += data_len;

	// tell XS to use msg as the return value.
	xsResult = xsArrayBuffer(msg, count);
	if (topic) c_free(topic);
	if (msg) c_free(msg);
}

void mqtt_subscribe_msg(xsMachine* the) {
	char *topic;
	size_t topic_len;
	int id;

	// fetch the packet ID, second arg
	id = 0x0000ffff & xsmcToInteger(xsArg(1));

	// fetch the topic string, first arg
	topic = xsmcToString(xsArg(0));
	topic_len = c_strlen(topic);

	topic = mqtt_string(topic, topic_len);
	if (!topic)
		xsUnknownError("mqtt: error allocating topic in subscribe");
	topic_len += 2;

	// format up the bytes
	uint8_t msg[512];
	size_t count = 1;
	msg[0] = SUBSCRIBE;

	// MQTT payload length header is variable length
	count += to_vlq(msg + 1, 511, topic_len + 3); // 2 bytes for packet ID, 1 byte for QoS at end
	if (!count) { // i.e. if to_vlq() returned -1 on error
		if (topic) c_free(topic);
		xsUnknownError("mqtt: error encoding payload length");
	}

	if (count + topic_len + 3 > sizeof(msg)) {
		if (topic) c_free(topic);
		xsUnknownError("mqtt: payload buffer overflow creating subscribe message");
	}

	msg[count++] = (uint8_t)(id >> 8);
	msg[count++] = (uint8_t)id & 0xff;
	c_memcpy(msg + count, topic, topic_len);
	count += topic_len;
	msg[count++] = 0;

	// tell XS6 to use that as the return value.
	xsResult = xsArrayBuffer(msg, count);
	if (topic) c_free(topic);
}

void mqtt_unsubscribe_msg(xsMachine* the) {
	char *topic;
	size_t topic_len;
	int id;

	id = 0x0000ffff & xsmcToInteger(xsArg(1));

	topic = xsmcToString(xsArg(0));
	topic_len = c_strlen(topic);
	topic = mqtt_string(topic, topic_len);
	if (!topic)
		xsUnknownError("mqtt: error allocating topic in unsubscribe");
	topic_len += 2;

	// set header and add varint/VLQ message length
	uint8_t msg[512];
	size_t count = 1;
	msg[0] = UNSUBSCRIBE;
	count += to_vlq(msg + 1, 511, 2 + topic_len);
	if (!count) {
		if (topic) c_free(topic);
		xsUnknownError("mqtt: error formatting message length in unsubscribe");
	}

	// packet ID; end of variable header
	msg[count++] = (uint8_t)(id >> 8);
	msg[count++] = (uint8_t)id & 0xff;

	// topic ID
	c_memcpy(msg + count, topic, topic_len);
	count += topic_len;

	xsResult = xsArrayBuffer(msg, count);
	if (topic) c_free(topic);
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
	uint8_t *msg = xsmcToArrayBuffer(xsArg(0));
	uint32_t code = msg[0] & 0xf0;

	size_t topic_len;

	xsResult = xsNewObject();
	xsmcVars(1);

	uint32_t payload_len;
	uint8_t *p;
	size_t skip;
	uint8_t qos;
	switch (code) {
		case PUBLISH: // this is the big one
			qos = msg[0] & 0x06;
			p = (msg + 1);
			skip = from_vlq(p, &payload_len, xsGetArrayBufferLength(xsArg(0)) - 1);
 			if (skip < 1)
				xsUnknownError("mqtt: malformed publish packet from server");
			p += skip;

			// decode length of topic (which must be next) from next 2 bytes
			topic_len = ((*(p++)) << 8);
			topic_len |= *(p++);
			payload_len -= 2;

			xsmcSetStringBuffer(xsVar(0), (char *)p, topic_len);
			xsmcSet(xsResult, xsID_topic, xsVar(0));
			p += topic_len;
			payload_len -= topic_len;

			if (qos) {
				p += 2; // skip 2-byte packet identifier which we don't use
				payload_len -= 2;
			}

			xsmcSetArrayBuffer(xsVar(0), p, payload_len);
			xsmcSet(xsResult, xsID_data, xsVar(0));
			xsmcSetInteger(xsVar(0), code);
			xsmcSet(xsResult, xsID_code, xsVar(0));

			break;

		case CONNACK: // we surface this just so the ES6 class can see flags set by server
			xsmcSetInteger(xsVar(0), msg[3]);
			xsmcSet(xsResult, xsID_returnCode, xsVar(0));
			xsmcSetInteger(xsVar(0), code);
			xsmcSet(xsResult, xsID_code, xsVar(0));
			break;

		case PUBACK:
		case PUBREC:
		case PUBREL:
		case PUBCOMP:
		case SUBACK:
		case UNSUBACK:
		case PINGRESP:
			// we ignore these b/c we only do QoS 0 & don't do callbacks for sub & ping ACKs & such,
			// but let the caller know they happened anyway
			xsmcSetInteger(xsVar(0), 0);
			xsmcSet(xsResult, xsID_code, xsVar(0));
			break;

		case CONNECT:
		case PINGREQ:
		case DISCONNECT:
		case SUBSCRIBE:
		case UNSUBSCRIBE:
			// per spec these are client->server only; we'll never get these
			xsUnknownError("mqtt: server sent us a client-only message?!");
			break;

		default:
			xsUnknownError("mqtt: server sent an unrecognized message");
			break;
	}
}
