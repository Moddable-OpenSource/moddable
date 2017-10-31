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

#include "xsAll.h"
#include "stdio.h"
//#include "mc_event.h"
//#include "mc_env.h"
#include "lwip/tcp.h"

#if ESP32
	#include "rom/ets_sys.h"

	void delay(uint32_t);
#else
	#include "tinyprintf.h"
#endif

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void *)0x87654321)

void fxCreateMachinePlatform(txMachine* the)
{
#ifdef mxDebug
	the->connection = mxNoSocket;
	the->debugOnReceive = true;
#endif
#if !ESP32
	init_printf(the, fx_putc);
#endif
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

void fx_putc(void *refcon, char c)
{
	txMachine* the = refcon;

	if (the->inPrintf) {
		if (0 == c) {
			if (kSerialConnection == the->connection) {
				// write xsbug log trailer
				txU1 stop, state;
				const static const char *xsbugTrailer = "&#10;</log></xsbug>\r\n";
				const char *cp = xsbugTrailer;
				while (true) {
					char c = c_read8(cp++);
					if (!c) break;
					ESP_putc(c);
				}
			}
			the->inPrintf = false;
			return;
		}
	}
	else {
		if (0 == c)
			return;

		the->inPrintf = true;
		if (kSerialConnection == the->connection) {
			// write xsbug log header
			static const char *xsbugHeader = "\r\n<xsbug><log>";
			const char *cp = xsbugHeader;
			while (true) {
				char c = c_read8(cp++);
				if (!c) break;
				ESP_putc(c);
			}
		}
	}

	ESP_putc(c);
}

#define mxEndian16_Swap(a)         \
	((((txU1)a) << 8)      |   \
	(((txU2)a) >> 8))

#if mxLittleEndian
	#define mxMisaligned16_GetN(a)         \
		(((txU2)((txU1*)(a))[1] << 8) |  \
		((txU2)((txU1*)(a))[0] << 0))
	#define mxEndianU16_LtoN(a) (a)
#else
	#define mxMisaligned16_GetN(a)         \
		(((txU2)((txU1*)(a))[0] << 8) |  \
		((txU2)((txU1*)(a))[1] << 0))
	#define mxEndianU16_LtoN(a) ((txU2)mxEndian16_Swap(a))
#endif

txU2* TextUTF8ToUnicode16NE(const unsigned char *text, txU4 textByteCount, txU4 *encodedTextByteCount)
{
	txU4  length      = 0;
	const txU1 *p = text;
	txU2* out;
	txU2* encodedText;

	while (textByteCount--) {                                       /* Convert from byte count to number of characters */
		unsigned c = *p++;
		if ((c & 0xC0) != 0x80)
			length++;
	}

	encodedText = c_malloc((length + 1) * 2); /* Allocate Unicode16 memory, including a NULL terminator */
	if(!encodedText)
		return C_NULL;
	if (encodedTextByteCount) *encodedTextByteCount = length * 2;   /* Set output byte count, if count was requested */
 
	out = encodedText;
	while (length--) {
		txU2 uc;
		uc = *text++;
		if (0x0080 & uc) {                                            /* non-ASCII */
			const txUTF8Sequence *aSequence;
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
				if ((uc & aSequence->cmask) == aSequence->cval)
					break;
			}
			if (0 != aSequence->size) {
				txU4 aSize = aSequence->size - 1;
				while (aSize) {
					aSize--;
					uc = (uc << 6) | (*text++ & 0x3F);
				}
				uc &= aSequence->lmask;
			}
			else
				uc = '?';
		}
		*out++ = uc;
	}
	*out = 0; /* terminate string */
	return encodedText;
}


txU1* TextUnicode16LEToUTF8(const txU2 *text, txU4 textByteCount, txU4 *encodedTextByteCount)
{
	txU1 *encodedText = C_NULL;
	txU4 encodeByteCount = 0;
	txU4 characterCount = textByteCount >> 1;
	txU1 *encodedTextOut;
	txU2 c;

	encodedText = c_malloc(1 + ((characterCount << 1) * 3));
	if(!encodedText)
		return C_NULL;
	encodedTextOut = encodedText;
	while (characterCount--) {
		c = mxMisaligned16_GetN(text);
		text++;
		c = mxEndianU16_LtoN(c);

		if (0 == (c & ~0x007f)) {
			*encodedText++ = (txU1)c;
			encodeByteCount += 1;
		}
		else
			if (0 == (c & ~0x07ff)) {
				*encodedText++ = (txU1)(0xc0 | (c >> 6));
				*encodedText++ = (txU1)(0x80 | (c & 0x3f));
				encodeByteCount += 2;
			}
			else {
				*encodedText++ = (txU1)(0xe0 | (c >> 12));
				*encodedText++ = (txU1)(0x80 | ((c >> 6) & 0x3f));
				*encodedText++ = (txU1)(0x80 | (c & 0x3f));
				encodeByteCount += 3;
			}
	}
	*encodedText++ = 0;
	if (encodedTextByteCount) *encodedTextByteCount = encodeByteCount;
	return encodedTextOut;
}

txString fxStringToUpper(txMachine* the, txString theString)
{
	txString result = NULL;
	txU2 *unicodeText;
	txU4 unicodeBytes;
	txU1 *utf8Text;
	txU4 utf8Bytes;
	txU4 i;
	txU2 c;
	unicodeText = TextUTF8ToUnicode16NE((const txU1 *)theString, c_strlen(theString), &unicodeBytes);
	if(!unicodeText)
		return C_NULL;
	for (i = 0; i < unicodeBytes / 2; i++) {
		c = unicodeText[i];
		if (c < 0x080) {
			unicodeText[i] = c_toupper(c);
		}
		/*according to http://www.unicode.org/charts*/
		else if ((c >= 0xff41) && (c<=0xff5a))
			unicodeText[i] = c - 0x20;
		else if ( c >= 0x0561 && c < 0x0587 ) 
			unicodeText[i] = c - 0x30;
	}
 
	utf8Text = TextUnicode16LEToUTF8(unicodeText, unicodeBytes, &utf8Bytes);
	if(!utf8Text)
		return C_NULL;
	result = fxNewChunk(the, utf8Bytes + 1);
	c_memmove(result, utf8Text, utf8Bytes + 1);
	c_free(utf8Text);
	c_free(unicodeText);
 	return result;
}

txString fxStringToLower(txMachine* the, txString theString)
{
	txString result = NULL;
	txU2 *unicodeText;
	txU4 unicodeBytes;
	txU1 *utf8Text;
	txU4 utf8Bytes;
	txU4 i;
	unicodeText = TextUTF8ToUnicode16NE((const txU1 *)theString, c_strlen(theString), &unicodeBytes);
	if(!unicodeText)
		return C_NULL;

	for (i = 0; i < unicodeBytes / 2; i++) {
		txU2 c = unicodeText[i];
		if (c < 0x080) {
			unicodeText[i] = c_tolower(c);
		}
		/*according to http://www.unicode.org/charts*/
		else if ( c >= 0x0531 && c <= 0x0556 ) 
			unicodeText[i] = c + 0x30;
		else if ((c >= 0xff21) && (c<=0xff3a))
			unicodeText[i] = c + 0x20;

	}
 
	utf8Text = TextUnicode16LEToUTF8(unicodeText, unicodeBytes, &utf8Bytes);
	if(!utf8Text)
		return C_NULL;
	result = fxNewChunk(the, utf8Bytes + 1);
	c_memmove(result, utf8Text, utf8Bytes + 1);
	c_free(utf8Text);
	c_free(unicodeText);
 	return result;
}

#ifdef mxDebug

void fxAbort(txMachine* the)
{
	fxDisconnect(the);
	c_exit(0);
}

/*
	http://lwip.wikia.com/wiki/Raw/TCP
*/

static err_t didConnect(void * arg, struct tcp_pcb * tpcb, err_t err)
{
	txMachine* the = arg;

	if (err) {
		modLog("  fxConnect - didConnect - ERROR");
		the->connection = (txSocket)-1;
	}
	else {
		xmodLog("  fxConnect - didConnect - OK");
		the->connection = tpcb;
	}

	return ERR_OK;
}

static err_t didReceive(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
	txMachine* the = arg;

    xmodLog("  didReceive - ENTER");

	if (NULL == p) {
		modLog("  didReceive - CONNECTION CLOSED");
		return ERR_OK;
	}

	if (NULL != the->reader) {
		modLog("  UNEXPECTED NETWORK PACKET");
		pbuf_free(the->reader);
		the->reader = NULL;		// unprocessed command. free the buffer so it doesn't linger forever.
	}

	the->reader = p;

    xmodLog("  didReceive - EXIT");

	return ERR_OK;
}

uint8_t triggerDebugCommand(txMachine *the)
{
	if (the->debugOnReceive) {
		fxDebugCommand(the);
		return the->breakOnStartFlag;
	}

	return false;
}

static err_t didReceiveMonitor(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
	txMachine* the = arg;
	err_t result = didReceive(arg, pcb, p, err);

	if (the->debugOnReceive && (ERR_OK == result) && the->reader) {
//		ESP_REPORT("  didReceiveMonitor - fxDebugCommand");
//		fxDebugCommand(the);
//@@ rather than call fxDebugCommand here, use an immediate timer to defer to main loop
//@@ otherwise, we can end up with (unexpected) re-entrant calls
	}

	return result;
}

static err_t didSend(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	txMachine* the = arg;
	the->pendingSendBytes -= len;
	return ERR_OK;
}

static void didError(void *arg, err_t err)
{
	modLog("XSBUG SOCKET ERROR!!!!!");
	modLogInt(err);
}

void fxConnect(txMachine* the)
{
	struct tcp_pcb *pcb;
	err_t err;
#if ESP32
	ip_addr_t ipaddr;
#else
	struct ip_addr ipaddr;
#endif
	extern unsigned char gXSBUG[4];

	if (the->context) {
		uint8_t **context = the->context;
		if (context[2])
			return;
	}

	if (isSerialIP(gXSBUG)) {
		the->connection = kSerialConnection;
		return;
	}

	if (0 == gXSBUG[0]) {
		modLog("  fxConnect - NO XSBUG ADDRESS");
		return;
	}

    xmodLog("  fxConnect - ENTER");

    xmodLog("  fxConnect - call tcp_new");
	pcb = tcp_new();
	the->reader = NULL;

    xmodLog("  fxConnect - call tcp_bind");
	err = tcp_bind(pcb, IP_ADDR_ANY, 0);
	if (err) modLog("  fxConnect - tcp_bind ERROR");

	xmodLog("  fxConnect - make address - 192.168.1.30");
#if ESP32
	IP4_ADDR(ip_2_ip4(&ipaddr), gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#else
	IP4_ADDR(&ipaddr, gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#endif

	tcp_arg(pcb, the);

    modLog("  fxConnect - connect to XSBUG");
	err = tcp_connect(pcb, &ipaddr, 5002, didConnect);
	if (err) {
		modLog("  fxConnect - tcp_connect ERROR");
		return;
	}

	while (!the->connection)
		delay(100);

	if ((txSocket)-1 == the->connection) {
		the->connection = NULL;
		modLog("  fxConnect - couldn't connect");
	}

	tcp_recv(pcb, didReceiveMonitor);
	tcp_sent(pcb, didSend);
	tcp_err(pcb, didError);
	tcp_nagle_disable(pcb);

    xmodLog("  fxConnect - EXIT");
}

void fxDisconnect(txMachine* the)
{
	xmodLog("  fxDisconnect - ENTER");
	if (the->connection) {
		if (kSerialConnection != the->connection)
			tcp_close((struct tcp_pcb *)the->connection);
		the->connection = NULL;
	}
	xmodLog("  fxDisconnect - EXIT");
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if (kSerialConnection == the->connection)
		return (txBoolean)ESP_isReadable();
	else
		return the->connection && the->reader;
}

void fxReceive(txMachine* the)
{
	struct tcp_pcb *pcb = the->connection;

	xmodLog("  fxReceive - ENTER");

	the->debugOffset = 0;

	if (!pcb) return;

	if (kSerialConnection != pcb) {
		struct pbuf *p;

		while (NULL == the->reader)
			delay(100);

		p = the->reader;
		if (p->next)
			modLog("  fxReceive - MULTIPLE PACKETS!!");

		if (p->len >=  sizeof(the->debugBuffer)) {
			modLog("  fxReceive - more than we can handle!!!");
			goto bail;
		}
		c_memmove(the->debugBuffer, p->payload, p->len);
		the->debugOffset = p->len;
		the->debugBuffer[the->debugOffset] = 0;		// note: pin input to debugSize - 1 to leave room for this byte

		tcp_recved(pcb, p->tot_len);
		pbuf_free(the->reader);
		the->reader = NULL;
	}
	else {
		while (the->debugOffset < (sizeof(the->debugBuffer) - 3)) {
			int c = ESP_getc();
			if (-1 == c) {
				modDelayMilliseconds(2);
				c = ESP_getc();
				if (-1 == c)
					break;
			}

			the->debugBuffer[the->debugOffset++] = (txU1)c;
		}
	}

bail:
	the->debugBuffer[the->debugOffset] = 0;

	xmodLog("  fxReceive - EXIT with:");
	xmodLog(the->debugBuffer);
}

void fxSend(txMachine* the, txBoolean more)
{
	struct tcp_pcb *pcb = the->connection;

	xmodLog("  fxSend - ENTER");

	if (!pcb) return;

	if (kSerialConnection != pcb) {
		int length = the->echoOffset;
		const char *bytes = the->echoBuffer;
		err_t err;

		xmodLog("  fxSend - about to loop");

		while (length) {
			u16_t available = tcp_sndbuf(pcb);
			if (0 == available) {
				xmodLog("  fxSend - need to wait");
				tcp_output(pcb);
				delay(100);
				continue;
			}

			if (available > length)
				available = length;

			while (true) {
				the->pendingSendBytes += available;
				// symchronous write so copy flag is unnecessary
				err = tcp_write(pcb, bytes, available, (more && (available == length)) ? TCP_WRITE_FLAG_MORE : 0);
				if (ERR_MEM == err) {
					the->pendingSendBytes -= available;
					modLog("  fxSend - wait for send memory:");
					tcp_output(pcb);
					delay(100);
					continue;
				}
				if (err) {
					the->pendingSendBytes -= available;
					modLog("  fxSend - tcp_write ERROR:");
					modLogInt(err);
				}
				break;
			}

			while (the->pendingSendBytes) {
				err = tcp_output(pcb);
				if (err)
					modLog("  fxSend - tcp_output ERROR:");
				delay(10);
			}

			length -= available;
			bytes += available;
		}
	}
	else {
		char *c = the->echoBuffer;
		txInteger count = the->echoOffset;
		while (count--)
			ESP_putc(*c++);
	}

	xmodLog("  fxSend - EXIT");
}

#endif /* mxDebug */

/* PROFILE */

#ifdef mxProfile

static txBoolean fxGetProfilePath(txMachine* the, char* thePath);

void fxCloseProfileFile(txMachine* the)
{
	if (the->profileFile) {
		fclose(the->profileFile);
		the->profileFile = NULL;
	}
}

txBoolean fxGetProfilePath(txMachine* the, char* thePath)
{
	(void)strcpy(thePath, mc_get_special_dir("temporaryDirectory"));
	return 1;
}

void fxOpenProfileFile(txMachine* the, char* theName)
{
	char aPath[PATH_MAX];

	if (fxGetProfilePath(the, aPath)) {
		strcat(aPath, theName);
		the->profileFile = fopen(aPath, "wb");
	}
	else
		the->profileFile = NULL;
}

void fxWriteProfileFile(txMachine* the, void* theBuffer, txInteger theSize)
{
	if (the->profileFile)
		fwrite(theBuffer, theSize, 1, the->profileFile);
}

#endif /* mxProfile */


void qsort(
   void *base,
   size_t num,
   size_t width,
   int (*compare )(const void *, const void *)
)
{
	//@@
}

#if !ESP32
double hack_fmod(double a, double b)
{
    return (a - b * floor(a / b));
}
#endif

#if ESP32
void delay(uint32_t msec) {
	vTaskDelay(msec);
}
#endif

