/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 *
 *     Portions Copyright (C) 2010-2015 Marvell International Ltd.
 *     Portions Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Portions Copyright by Marvell International Ltd. and Kinoma, Inc. are 
 *     derived from KinomaJS/XS6 and used under the Apache 2.0 License.
 */

#include "xsAll.h"
#include "stdio.h"

#define XSDEBUG_NONE	0,0,0,0
#define XSDEBUG_SERIAL	127,0,0,7

#ifndef DEBUG_IP
	#define DEBUG_IP XSDEBUG_SERIAL
#endif

unsigned char gXSBUG[4] = {DEBUG_IP};

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void*)0x87654321)

void ESP_putc(int c);
int ESP_getc(void);
uint8_t ESP_isReadable();

void fxCreateMachinePlatform(txMachine* the)
{
#ifdef mxDebug
	the->connection = mxNoSocket;
	the->debugOnReceive = true;
#endif
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

#if 1
void fx_putc(void *unused, char c) {
	ESP_putc(c);
}
#else
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
#endif

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

uint8_t triggerDebugCommand(txMachine *the)
{
	if (the->debugOnReceive) {
		fxDebugCommand(the);
		return the->breakOnStartFlag;
	}

	return false;
}


void fxConnect(txMachine* the)
{
	extern unsigned char gXSBUG[4];

	if (isSerialIP(gXSBUG)) {
		the->connection = kSerialConnection;
		return;
	}

}

void fxDisconnect(txMachine* the)
{
}

txBoolean fxIsConnected(txMachine* the)
{
	return (mxNoSocket != the->connection) ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if (kSerialConnection == the->connection)
		return (txBoolean)ESP_isReadable();

	return 0;
}

void fxReceive(txMachine* the)
{
	the->debugOffset = 0;

	if (kSerialConnection == the->connection) {
		txU1 stop = 0, state = 0;

		while (stop < 2) {
			int c = ESP_getc();
			if (-1 == c) continue;

			the->debugBuffer[the->debugOffset++] = (txU1)c;

			if ((13 == c) || (10 == c)) {
				state++;
				if (2 == state)
					stop++;
			}
			else
				state = 0;
		}
	}

bail:
	the->debugBuffer[the->debugOffset] = 0;

}

void fxSend(txMachine* the, txBoolean more)
{
	if (kSerialConnection == the->connection) {
		char *c = the->echoBuffer;
		txInteger count = the->echoOffset;
		while (count--)
			ESP_putc(*c++);
	}
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

static int32_t gTimeZoneOffset = -8 * 60 * 60;      // Menlo Park
static int16_t gDaylightSavings = 60 * 60;          // summer time

void modSetTime(uint32_t seconds)
{
	struct timeval tv;
	struct timezone tz;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;

//  c_settimeofday(&tv, NULL);
//          //// NEED TO IMPLEMENT SETTIMEOFDAY
}

void modSetTimeZone(int32_t timeZoneOffset)
{
	gTimeZoneOffset = timeZoneOffset;
}

int32_t modGetTimeZone(void)
{
	return gTimeZoneOffset;
}

void modSetDaylightSavingsOffset(int32_t daylightSavings)
{
	gDaylightSavings = daylightSavings;
}

int32_t modGetDaylightSavingsOffset(void)
{
	return gDaylightSavings;
}

