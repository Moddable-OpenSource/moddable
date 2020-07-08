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
 */

#include "xsmc.h"
#include "xsHost.h"
#include "modTimer.h"
#include "mc.xs.h"			// for xsID_ values

#include "../socket/modSocket.h"

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

/*
	enable CONFIG_MBEDTLS_DEBUG in esp-idf/components/mbedtls/port/include/mbedtls/esp_config.h for mbedtls debugging logs
		warning: it sometimes crashes with CONFIG_MBEDTLS_DEBUG enabled

	certificate PEM file converted to xtensa OBJ using objcopy:
		~/esp32/xtensa-esp32-elf/bin/xtensa-esp32-elf-objcopy --input binary --binary-architecture xtensa --output elf32-xtensa-le server_root_cert.pem server_root_cert.pem.o

	Notes:
	
		Could make size of read and write buffers configurable in dictionary
*/

typedef struct xsSocketRecord xsSocketRecord;
typedef xsSocketRecord *xsSocket;

struct xsSocketRecord {
	xsSlot						obj;

	xsMachine					*the;

///@@ some/all of these go into a global to be shared?
    mbedtls_entropy_context		entropy;
    mbedtls_ctr_drbg_context	ctr_drbg;
    mbedtls_ssl_context			ssl;
    mbedtls_x509_crt			cacert;
    mbedtls_ssl_config			conf;

// this is the socket
    mbedtls_net_context			server_fd;
	modTimer					timer;

	uint8_t						useCount;
	uint8_t						done;
	uint8_t						didHandshake;
	uint8_t						doDisconnect;
	uint8_t						doError;
	uint8_t						verify;

	uint8_t						*readBuffer;
	int32_t						readBytes;

	int32_t						writeBytes;
	int32_t						unreportedSent;		// bytes sent to the socket but not yet reported to object as sent

	uint8_t						buf[1024];

	uint8_t						writeBuf[1024];
};

typedef struct xsListenerRecord xsListenerRecord;
typedef xsListenerRecord *xsListener;

#define kListenerPendingSockets (4)
struct xsListenerRecord {
	xsListener			next;

	xsSlot				obj;
};

static void socketStateMachine(modTimer timer, void *refcon, int refconSize);
static void doFlushWrite(xsSocket xss);
static void socketMsgError(xsSocket xss);

#ifdef MBEDTLS_DEBUG_C
static void mbedtls_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
	xsMachine *the = ctx;
    char *slash = rindex(file, '/');
    if (slash)
        file = slash + 1;

    switch (level) {
		case 1:
		case 2:
		case 3:
		case 4:
			xsLog("Debug %d: %s:%d %s", level, file, line, str);
			break;
		default:
			xsLog("Unexpected log level %d: %s", level, str);
			break;
    }
}
#endif

static void socketDownUseCount(xsMachine *the, xsSocket xss)
{
	xss->useCount -= 1;
	if (xss->useCount <= 0) {
		xsDestructor destructor = xsGetHostDestructor(xss->obj);
		xsmcSetHostData(xss->obj, NULL);
		(*destructor)(xss);
	}
}

void xs_socketmbedtls(xsMachine *the)
{
	xsSocket xss;
    int ret, port;
	char portStr[10], *cert;

	xss = c_calloc(sizeof(xsSocketRecord), 1);
	if (!xss)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, xss);
//@@ root this so no garbage collection
	xss->obj = xsThis;
	xss->useCount = 1;
	xss->the = the;

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_host)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_host);
		xsmcToStringBuffer(xsVar(0), xss->buf, sizeof(xss->buf));
	}
	else
		xsUnknownError("host required in dictionary");

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
	}
	else
		xsUnknownError("port required in dictionary");

	if (xsmcHas(xsArg(0), xsID_verify)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_verify);
		xss->verify = xsmcTest(xsVar(0));
	}
	else
		xss->verify = 1;

	mbedtls_ssl_init(&xss->ssl);
	mbedtls_x509_crt_init(&xss->cacert);
	mbedtls_ctr_drbg_init(&xss->ctr_drbg);

	mbedtls_ssl_config_init(&xss->conf);

	mbedtls_entropy_init(&xss->entropy);
	if((ret = mbedtls_ctr_drbg_seed(&xss->ctr_drbg, mbedtls_entropy_func, &xss->entropy, NULL, 0)) != 0)
		xsUnknownError("mbedtls_ctr_drbg_seed failed");

	if (!xsmcHas(xsArg(0), xsID_certificate))
		xsUnknownError("certificate required in dictionary");

	xsmcGet(xsVar(0), xsArg(0), xsID_certificate);
	cert = xsmcToString(xsVar(0));
	ret = mbedtls_x509_crt_parse(&xss->cacert, cert, c_strlen(cert) + 1);
	if (ret < 0)
		xsUnknownError("mbedtls_x509_crt_parse failed");

    if((ret = mbedtls_ssl_set_hostname(&xss->ssl, xss->buf)) != 0)
		xsUnknownError("mbedtls_ssl_set_hostname failed");

    if((ret = mbedtls_ssl_config_defaults(&xss->conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
		xsUnknownError("mbedtls_ssl_config_defaults failed");

	mbedtls_ssl_conf_authmode(&xss->conf, xss->verify ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&xss->conf, &xss->cacert, NULL);
	mbedtls_ssl_conf_rng(&xss->conf, mbedtls_ctr_drbg_random, &xss->ctr_drbg);

	if (xsmcHas(xsArg(0), xsID_min)) {
		xsNumberValue min;
		xsmcGet(xsVar(0), xsArg(0), xsID_min);
		min = xsmcToNumber(xsVar(0));
		mbedtls_ssl_conf_min_version(&xss->conf, (int)min, (int)(((min - c_floor(min)) * 10) + 0.5));
	}

#ifdef MBEDTLS_DEBUG_C
//	#define MBEDTLS_DEBUG_LEVEL 4
//	mbedtls_debug_set_threshold(MBEDTLS_DEBUG_LEVEL);
//	mbedtls_ssl_conf_dbg(&xss->conf, mbedtls_debug, the);
#endif

    if ((ret = mbedtls_ssl_setup(&xss->ssl, &xss->conf)) != 0)
		xsUnknownError("mbedtls_ssl_setup failed");

	mbedtls_net_init(&xss->server_fd);

	itoa(port, portStr, 10);
	ret = mbedtls_net_connect(&xss->server_fd, xss->buf, portStr, MBEDTLS_NET_PROTO_TCP);		// this blocks in mbedtls. for a long time, potentially...
	if (ret)
		xsUnknownError("mbedtls_net_connect failed");

	mbedtls_ssl_set_bio(&xss->ssl, &xss->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	mbedtls_net_set_nonblock(&xss->server_fd);

	xss->timer = modTimerAdd(0, 5, socketStateMachine, &xss, sizeof(xss));
}

void xs_socketmbedtls_destructor(void *data)
{
	xsSocket xss = data;

	if (xss) {
		mbedtls_ssl_close_notify(&xss->ssl);
		mbedtls_ssl_session_reset(&xss->ssl);
		mbedtls_net_free(&xss->server_fd);
		mbedtls_ssl_free(&xss->ssl);

		mbedtls_ssl_config_free(&xss->conf);
		mbedtls_ctr_drbg_free(&xss->ctr_drbg);
		mbedtls_entropy_free(&xss->entropy);
		mbedtls_x509_crt_free(&xss->cacert);

		modTimerRemove(xss->timer);
		c_free(xss);
	}
}

void xs_socketmbedtls_close(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);

	if ((NULL == xss) || xss->done)
		xsUnknownError("close on closed socket");

	xss->done = 1;
	socketDownUseCount(the, xss);
}

void xs_socketmbedtls_read(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	xsType dstType;
	int argc = xsmcArgc;
	uint16_t srcBytes;
	unsigned char *srcData;

	if ((NULL == xss) || xss->done)
		xsUnknownError("read on closed socket");

	if (!xss->readBuffer || !xss->readBytes) {
		if (0 == argc)
			xsResult = xsInteger(0);
		return;
	}

	srcData = xss->readBuffer;
	srcBytes = xss->readBytes;

	if (0 == argc) {
		xsResult = xsInteger(srcBytes);
		return;
	}

	// address limiter argument (count or terminator)
	if (argc > 1) {
		xsType limiterType = xsmcTypeOf(xsArg(1));
		if ((xsNumberType == limiterType) || (xsIntegerType == limiterType)) {
			uint16_t count = xsmcToInteger(xsArg(1));
			if (count < srcBytes)
				srcBytes = count;
		}
		else
		if (xsStringType == limiterType) {
			char *str = xsmcToString(xsArg(1));
			char terminator = espRead8(str);
			if (terminator) {
				unsigned char *t = strchr(srcData, terminator);
				if (t) {
					uint16_t count = (t - srcData) + 1;		// terminator included in result
					if (count < srcBytes)
						srcBytes = count;
				}
			}
		}
		else if (xsUndefinedType == limiterType)
			;
	}

	// generate output
	dstType = xsmcTypeOf(xsArg(0));

	if (xsNullType == dstType)
		xsResult = xsInteger(srcBytes);
	else if (xsReferenceType == dstType) {
		xsSlot *s1, *s2;

		s1 = &xsArg(0);

		xsmcVars(1);
		xsmcGet(xsVar(0), xsGlobal, xsID_String);
		s2 = &xsVar(0);
		if (s1->data[2] == s2->data[2])		//@@
			xsResult = xsStringBuffer(srcData, srcBytes);
		else {
			xsmcGet(xsVar(0), xsGlobal, xsID_Number);
			s2 = &xsVar(0);
			if (s1->data[2] == s2->data[2]) {		//@@
				xsResult = xsInteger(*srcData);
				srcBytes = 1;
			}
			else {
				xsmcGet(xsVar(0), xsGlobal, xsID_ArrayBuffer);
				s2 = &xsVar(0);
				if (s1->data[2] == s2->data[2])		//@@
					xsmcSetArrayBuffer(xsResult, srcData, srcBytes);
				else
					xsUnknownError("unsupported output type");
			}
		}
	}

	xss->readBuffer += srcBytes;
	xss->readBytes -= srcBytes;
}

void xs_socketmbedtls_write(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	int argc = xsmcArgc, i;
	uint8_t *dst;
	char *msg;
	size_t msgLen;
	uint16_t available, needed = 0;
	unsigned char pass, arg;

	if (!xss || xss->done)
		xsUnknownError("write on closed socket");

	available = sizeof(xss->writeBuf) - xss->writeBytes;
	if (0 == argc) {
		xsResult = xsInteger(available);
		return;
	}

	dst = xss->writeBuf + xss->writeBytes;
	for (pass = 0; pass < 2; pass++ ) {
		for (arg = 0; arg < argc; arg++) {
			xsType t = xsmcTypeOf(xsArg(arg));

			if (xsStringType == t) {
				char *msg = xsmcToString(xsArg(arg));
				int msgLen = espStrLen(msg);
				if (0 == pass)
					needed += msgLen;
				else {
					c_memcpy(dst, msg, msgLen);
					dst += msgLen;
				}
			}
			else if ((xsNumberType == t) || (xsIntegerType == t)) {
				if (0 == pass)
					needed += 1;
				else
					*dst++ = (unsigned char)xsmcToInteger(xsArg(arg));
			}
			else if ((xsReferenceType == t) && (xsmcIsInstanceOf(xsArg(arg), xsArrayBufferPrototype))) {
				int msgLen = xsmcGetArrayBufferLength(xsArg(arg));
				if (0 == pass)
					needed += msgLen;
				else {
					char *msg = xsmcToArrayBuffer(xsArg(arg));
					c_memcpy(dst, msg, msgLen);
					dst += msgLen;
				}
			}
			else
				xsUnknownError("unsupported type for write");
		}

		if ((0 == pass) && (needed > available))
			xsUnknownError("can't write all data");
	}

	xss->writeBytes = dst - xss->writeBuf;

	doFlushWrite(xss);
}

void socketStateMachine(modTimer timer, void *refcon, int refconSize)
{
	xsSocket xss = *(xsSocket *)refcon;
	xsMachine *the = xss->the;
	uint8_t doImmediate = 0;
	int ret;

	xss->useCount += 1;

	/*
		done
	*/
	if (xss->done) {

		if (xss->doDisconnect) {
			xsBeginHost(the);
				xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgDisconnect));
			xsEndHost(the);
		}
		if (xss->doError) {
			xsBeginHost(the);
				xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgError));
			xsEndHost(the);
		}

		modTimerRemove(xss->timer);
		xss->timer = NULL;

		goto done;
	}

	/*
		finish connecting
	*/
	if (!xss->didHandshake) {
		int flags;
		do {
			ret = mbedtls_ssl_handshake(&xss->ssl);
			if (ret && (ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
				socketMsgError(xss);
				goto done;
			}
		} while (ret);

		xss->didHandshake = true;

		xsBeginHost(the);
			xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgConnect));
		xsEndHost(the);

		if (xss->done)
			goto done;
	}

	/*
		try to read
	*/
	ret = mbedtls_ssl_read(&xss->ssl, xss->buf, sizeof(xss->buf));
	if (ret > 0) {
		xss->readBuffer = xss->buf;
		xss->readBytes = ret;

		xsBeginHost(the);
			xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(ret));
		xsEndHost(the);

		xss->readBuffer = NULL;
		xss->readBytes = 0;

		doImmediate = 1;
	}
	else {
		if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
			;
		else if ((MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY == ret) || (0 == ret)) {
			xss->done = 1;
			xss->doDisconnect = 1;
			goto done;
		}
		else
			socketMsgError(xss);
	}

	/*
		try to flush pending write
	*/
	if (xss->writeBytes) {
		doFlushWrite(xss);
		doImmediate = 1;
	}

	if (xss->unreportedSent) {
		int unreportedSent = xss->unreportedSent;
		xss->unreportedSent = 0;
		xsBeginHost(the);
			xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataSent), xsInteger(unreportedSent));
		xsEndHost(the);

		if (xss->unreportedSent)
			doImmediate = 1;
	}

	/*
		reschedule timer
	*/
	if (doImmediate)
		modTimerReschedule(xss->timer, 0, 5);

done:
	socketDownUseCount(the, xss);
}

void doFlushWrite(xsSocket xss)
{
	int ret = mbedtls_ssl_write(&xss->ssl, xss->writeBuf, xss->writeBytes);
	if (ret <= 0) {
		if ((MBEDTLS_ERR_SSL_WANT_READ != ret) && (MBEDTLS_ERR_SSL_WANT_WRITE != ret))
			socketMsgError(xss);
		//@@ check for disconnect
		return;
	}

	if (ret > 0) {
		if (ret < xss->writeBytes)
			c_memcpy(xss->writeBuf, xss->writeBuf + ret, xss->writeBytes - ret);
		xss->writeBytes -= ret;
		xss->unreportedSent += ret;
	}
}

void socketMsgError(xsSocket xss)
{
	xss->done = 1;
	xss->doError = 1;
}

// to accept an incoming connection: let incoming = new Socket({listener});
void xs_listenermbedtls(xsMachine *the)
{
	xsListener xsl;
}

void xs_listenermbedtls_destructor(void *data)
{
	xsListener xsl = data;

}

void xs_listenermbedtls_close(xsMachine *the)
{
	xsListener xsl = xsmcGetHostData(xsThis);

	if (NULL == xsl)
		xsUnknownError("close on closed listener");
}
