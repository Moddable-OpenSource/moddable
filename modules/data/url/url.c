/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

#include "xsAll.h"
#ifdef kPocoRotation
	#include "mc.xs.h"
	#define MODDABLE_MODULE 1
#else
	#define xsID_scheme fxID(the, "scheme")
	#define xsID_username fxID(the, "username")
	#define xsID_password fxID(the, "password")
	#define xsID_host fxID(the, "host")
	#define xsID_port fxID(the, "port")
	#define xsID_path fxID(the, "path")
	#define xsID_query fxID(the, "query")
	#define xsID_fragment fxID(the, "fragment")
	extern void fx_parseURL(txMachine* the);
	extern void fx_serializeURL(txMachine* the);
	extern void fx_parseQuery(txMachine* the);
	extern void fx_serializeQuery(txMachine* the);
#endif


static void fx_parseURLCopyPart(txMachine* the, txSlot* src, txSlot* dst, txID id);
static void fx_parseURLCopyPath(txMachine* the, txSlot* src, txSlot* dst);
static txInteger fx_parseURLDecode(txMachine* the, txStringStream* stream);
static void fx_parseURLEncode(txMachine* the, txInteger c, txStringStream* dst);
static void fx_parseURLEmptyPart(txMachine* the, txSlot* target, txID id);
static txBoolean fx_parseURLHasOpaquePath(txMachine* the, txSlot* parts);
static txBoolean fx_parseURLHost(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst, txBoolean special);
static txBoolean fx_parseURLHostIPv4(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst);
static txBoolean fx_parseURLHostIPv6(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst);
static txBoolean fx_parseURLHostNotSpecial(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst);
static txBoolean fx_parseURLHostSpecial(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst);
static void fx_parseURLNullPart(txMachine* the, txSlot* target, txID id);
static txBoolean fx_parseURLIsPartEmpty(txMachine* the, txSlot* parts, txID id);
static txBoolean fx_parseURLIsWindowsDriveLetter(txMachine* the, txString string, txBoolean startsWith);
static void fx_parseURLPercentEncode(txMachine* the, txInteger c, txStringStream* dst, const char* set);
static void fx_parseURLPushPath(txMachine* the, txSlot* path, txStringStream* dst, txBoolean flag);
static void fx_parseURLSetPart(txMachine* the, txSlot* target, txID id, txIndex index, txStringStream* dst); 
static void fx_parseURLShortenPath(txMachine* the, txSlot* path);
static txInteger fx_parseURLSpecialScheme(txMachine* the, txString scheme);
static void fx_parseQueryPlus(txMachine* the, txSlot* string, txInteger from, txInteger to);
static void fx_serializeQueryPlus(txMachine* the, txString theSet);

static const char ICACHE_RODATA_ATTR gxControlSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxUserInfoSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxNotSpecialHostSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxSpecialHostSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxPathSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxQuerySet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};
static const char ICACHE_RODATA_ATTR gxSpecialQuerySet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxFragmentSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxFormSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,1,0,0,0,0,0,0,0,1,0,0,1,1,0,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0, 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char gxHexLower[] ICACHE_FLASH_ATTR = "0123456789abcdef";

#define mx_parseURLBufferSize 64
#define mx_parseURLSpecialSchemeCount 6
static const txString gx_parseURLSpecialSchemeNames[mx_parseURLSpecialSchemeCount] = {
	"file",
	"ftp",
	"http",
	"https",
	"ws",
	"wss",
};
static const txInteger gx_parseURLSpecialSchemePorts[mx_parseURLSpecialSchemeCount] = {
	0,
	21,
	80,
	443,
	80,
	443,
};

enum {
	SCHEME_STATE = 1,
	USERNAME_STATE,
	PASSWORD_STATE,
	HOST_PORT_STATE,
	HOST_STATE,
	PORT_STATE,
	PATH_STATE,
	QUERY_STATE,
	FRAGMENT_STATE,
	ORIGIN_STATE,
};

void fx_parseURL(txMachine* the) 
{
	txStringStream _src;
	txStringStream* src = &_src;
	txStringStream _dst;
	txStringStream* dst = &_dst;

	txInteger override = 0;
	txInteger overrideSchemeIndex = -1;
	txBoolean overrideSpecial = 0;

	txSlot* base = C_NULL;
	txInteger baseSchemeIndex = -1;
	txBoolean baseSpecial = 0;
	
	txSlot* result = C_NULL;
	txInteger schemeIndex = -1;
	txBoolean special = 0;

	txInteger atOffset = -1;
	txInteger colonOffset = -1;
	txInteger hostOffset = -1;
	txSize tmp;
	txBoolean insideBracket = 0;

	txIndex length;
	txInteger c;
	txNumber port;
    
    fxVars(the, 3);
	src->slot = mxArgv(0);
	src->offset = 0;
	src->size = mxStringLength(fxToString(the, mxArgv(0)));
	mxVarv(0)->value.string = fxNewChunk(the, mx_parseURLBufferSize);
	mxVarv(0)->kind = XS_STRING_KIND;
	dst->slot = mxVarv(0);
    dst->slot->value.string[0] = 0;
    dst->offset = 0;
    dst->size = mx_parseURLBufferSize;
	
    if (mxArgc > 2) {
 		*mxResult = *mxArgv(1);
 		result = mxResult;
 		
   		override = fxToInteger(the, mxArgv(2));
 		mxPushSlot(result);
		mxGetID(xsID_scheme);
		overrideSchemeIndex = fx_parseURLSpecialScheme(the, the->stack->value.string);
		mxPop();
		overrideSpecial = overrideSchemeIndex >= 0;

   		if (override == SCHEME_STATE) {
			goto XS_URL_SCHEME_START;
    	}
    	schemeIndex = overrideSchemeIndex;
    	special = overrideSpecial;
   		if (override == USERNAME_STATE) {
   			if ((schemeIndex == 0) || fx_parseURLIsPartEmpty(the, result, xsID_host))
  				goto XS_URL_END;
			goto XS_URL_USERNAME;
    	}
    	if (override == PASSWORD_STATE) {
   			if ((schemeIndex == 0) || fx_parseURLIsPartEmpty(the, result, xsID_host))
  				goto XS_URL_END;
			goto XS_URL_PASSWORD;
		}
    	if (override == HOST_PORT_STATE) {
	    	if (fx_parseURLHasOpaquePath(the, result))
 				goto XS_URL_END;
 			hostOffset = 0;
 		    if (schemeIndex == 0)
				goto XS_URL_FILE_HOST;
			goto XS_URL_HOST;
     	}
   		if (override == HOST_STATE) {
	    	if (fx_parseURLHasOpaquePath(the, result))
 				goto XS_URL_END;
  			hostOffset = 0;
		    if (schemeIndex == 0)
				goto XS_URL_FILE_HOST;
			goto XS_URL_HOST;
    	}
    	if (override == PORT_STATE) {
   			if ((schemeIndex == 0) || fx_parseURLIsPartEmpty(the, result, xsID_host))
  				goto XS_URL_END;
   			goto XS_URL_PORT;
    	}
    	if (override == PATH_STATE) {
    		if (fx_parseURLHasOpaquePath(the, result))
 				goto XS_URL_END;
			mxPushInteger(0);
			mxPushSlot(result);
			mxSetID(mxID(_length));
			mxPop();
 			c = fx_parseURLDecode(the, src);
  			if ((c != '/') && (!special || (c != '\\')))
    			src->offset = 0;
  			goto XS_URL_PATH;
    	}
    	if (override == QUERY_STATE) {
    		if (src->size == 0) {
    			fx_parseURLNullPart(the, result, xsID_query);
 				goto XS_URL_END;
    		}
    		fx_parseURLEmptyPart(the, result, xsID_query);
 			c = fx_parseURLDecode(the, src);
   			if (c != '?')
    			src->offset = 0;
   			goto XS_URL_QUERY;
    	}
    	if (override == FRAGMENT_STATE) {
    		if (src->size == 0) {
    			fx_parseURLNullPart(the, result, xsID_fragment);
 				goto XS_URL_END;
    		}
    		fx_parseURLEmptyPart(the, result, xsID_fragment);
 			c = fx_parseURLDecode(the, src);
   			if (c != '#')
    			src->offset = 0;
   			goto XS_URL_FRAGMENT;
    	}
    	goto XS_URL_ERROR;
	}
    else {
		fxNewArray(the, 0);
		mxPullSlot(mxResult);
		result = mxResult;
		
    	fx_parseURLEmptyPart(the, result, xsID_username);
    	fx_parseURLEmptyPart(the, result, xsID_password);
    	fx_parseURLNullPart(the, result, xsID_host);
    	fx_parseURLEmptyPart(the, result, xsID_port);
    	fx_parseURLNullPart(the, result, xsID_query);
    	fx_parseURLNullPart(the, result, xsID_fragment);
	
    	if ((mxArgc > 1) && mxIsReference(mxArgv(1))) {
    		base = mxArgv(1);
			mxPushSlot(base);
			mxGetID(xsID_scheme);
			baseSchemeIndex = fx_parseURLSpecialScheme(the, the->stack->value.string);
			mxPop();
			baseSpecial = baseSchemeIndex >= 0;
    	}
    }
    
	for (;;) {
		c = fx_parseURLDecode(the, src);
		if (c == C_EOF)
			break;
		if ((c > 0x20) && (c != 0x7F)) {
			fx_parseURLEncode(the, c, dst);
			break;
		}
	}
	if (c != C_EOF) {
		tmp = -1;
		for (;;) {
			c = fx_parseURLDecode(the, src);
			if (c == C_EOF)
				break;
			if ((c > 0x20) && (c != 0x7F))
				tmp = -1;
			else if (tmp < 0)
				tmp = dst->offset;
			if ((c != 9) && (c != 10) && (c != 13))
				fx_parseURLEncode(the, c, dst);
		}
		if (tmp >= 0) {
			dst->offset = tmp;
			dst->slot->value.string[tmp] = 0;
		}
	}
	src->slot = dst->slot;
	src->offset = 0;
	src->size = dst->size;
	mxArgv(0)->value.string = fxNewChunk(the, mx_parseURLBufferSize);
	mxArgv(0)->kind = XS_STRING_KIND;
	dst->slot = mxArgv(0);
    dst->slot->value.string[0] = 0;
    dst->offset = 0;
    dst->size = mx_parseURLBufferSize;
	

XS_URL_SCHEME_START:
	c = fx_parseURLDecode(the, src);
	if (('A' <= c) && (c <= 'Z')) {
		fx_parseURLEncode(the, c - ('A' - 'a'), dst);
		goto XS_URL_SCHEME;
	}
	if (('a' <= c) && (c <= 'z')) {
		fx_parseURLEncode(the, c, dst);
		goto XS_URL_SCHEME;
	}
	if (override)
		goto XS_URL_ERROR;
	src->offset = 0;
 	dst->offset = 0;
	goto XS_URL_NO_SCHEME;

XS_URL_SCHEME:
	c = fx_parseURLDecode(the, src);
	if (('A' <= c) && (c <= 'Z')) {
		fx_parseURLEncode(the, c - ('A' - 'a'), dst);
		goto XS_URL_SCHEME;
	}
	if ((('a' <= c) && (c <= 'z')) || (('0' <= c) && (c <= '9')) || (c == '+') || (c == '-') || (c == '.')) {
		fx_parseURLEncode(the, c, dst);
		goto XS_URL_SCHEME;
	}
	if ((c == ':') || (override && (c == C_EOF))) {
		schemeIndex = fx_parseURLSpecialScheme(the, dst->slot->value.string);
    	special = schemeIndex >= 0;
		if (override) {
    		if (overrideSpecial != special)
    			goto XS_URL_END;
    		if (schemeIndex == 0) {
    			if (!fx_parseURLIsPartEmpty(the, result, xsID_username))
    				goto XS_URL_END;
    			if (!fx_parseURLIsPartEmpty(the, result, xsID_password))
    				goto XS_URL_END;
    			if (!fx_parseURLIsPartEmpty(the, result, xsID_port))
    				goto XS_URL_END;
    		}
    		if ((overrideSchemeIndex == 0) && (schemeIndex != 0)) {
     			if (fx_parseURLIsPartEmpty(the, result, xsID_host))
    				goto XS_URL_END;
    		}
		}
		fx_parseURLSetPart(the, result, xsID_scheme, 0, dst);
		if (override) {
			if (special) { 
    			mxPushSlot(result);
    			mxGetID(xsID_port);
				port = (the->stack->value.string[0]) ? fxToInteger(the, the->stack) : 0;
				mxPop();
				if (port == gx_parseURLSpecialSchemePorts[schemeIndex])
    				fx_parseURLEmptyPart(the, result, xsID_port);
			}
			goto XS_URL_END;
		}
		if (schemeIndex == 0)
			goto XS_URL_FILE_START;
		if (special) {
			if (base && (baseSchemeIndex == schemeIndex))
    			goto XS_URL_RELATIVE;
			goto XS_URL_AUTHORITY_START;
		}
		
		tmp = src->offset;
		c = fx_parseURLDecode(the, src);
		if (c == '/') {
			tmp = src->offset;
			c = fx_parseURLDecode(the, src);
			if (c == '/')
				goto XS_URL_AUTHORITY_START;
			src->offset = tmp;
			c = '/';
			goto XS_URL_PATH;
		}
		src->offset = tmp;
		goto XS_URL_OPAQUE_PATH;
	}
	if (override)
		goto XS_URL_ERROR;
	src->offset = 0;
 	dst->offset = 0;
		
XS_URL_NO_SCHEME:
	if (!base)
		goto XS_URL_ERROR;
	fx_parseURLCopyPart(the, base, result, xsID_scheme);
	schemeIndex = baseSchemeIndex;
	special = baseSpecial;
	if (schemeIndex == 0)
		goto XS_URL_FILE_START;

    if (fx_parseURLHasOpaquePath(the, base)) {
		c = fx_parseURLDecode(the, src);
   		if (c == '#') {
			fx_parseURLCopyPart(the, base, result, xsID_path);
			fx_parseURLCopyPart(the, base, result, xsID_query);
			goto XS_URL_FRAGMENT;
  		}
  		goto XS_URL_ERROR;
    }
   	goto XS_URL_RELATIVE;

XS_URL_RELATIVE:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (special && (c == '\\'))) {
		c = fx_parseURLDecode(the, src);
		if ((c == '/') || (special && (c == '\\'))) {
			goto XS_URL_AUTHORITY_START;
		}
	}
	src->offset = tmp;
	
	fx_parseURLCopyPart(the, base, result, xsID_username);
	fx_parseURLCopyPart(the, base, result, xsID_password);
	fx_parseURLCopyPart(the, base, result, xsID_host);
	fx_parseURLCopyPart(the, base, result, xsID_port);

	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (special && (c == '\\')))
		goto XS_URL_PATH;
	fx_parseURLCopyPath(the, base, result);
	if (c == '?')
		goto XS_URL_QUERY;
	fx_parseURLCopyPart(the, base, result, xsID_query);
	if (c == '#')
		goto XS_URL_FRAGMENT;
	if (c != C_EOF) {
		fx_parseURLShortenPath(the, result);
		src->offset = tmp;
		goto XS_URL_PATH;
	}
	goto XS_URL_END;
	
XS_URL_AUTHORITY_START:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if (special && ((c == '/') || (c == '\\')))
		goto XS_URL_AUTHORITY_START;
	src->offset = tmp;

XS_URL_AUTHORITY:
	c = fx_parseURLDecode(the, src);
	if (c == '@') {
		atOffset = src->offset - 1;
	}
	else if (c == ':') {
		if (colonOffset < 0)
			colonOffset = src->offset - 1;
	}
	if ((c == C_EOF) || (c == '/') || (special && (c == '\\')) || (c == '?') || (c == '#')) {
		txInteger endOffset = src->offset - 1;
		src->offset = tmp;
		if (atOffset >= 0) {
			if (colonOffset > atOffset)
				colonOffset = -1;
			if (colonOffset >= 0) {
				while (src->offset < colonOffset) {
					c = fx_parseURLDecode(the, src);
					fx_parseURLPercentEncode(the, c, dst, gxUserInfoSet);
				}
				fx_parseURLSetPart(the, result, xsID_username, 0, dst);
				src->offset++;
				while (src->offset < atOffset) {
					c = fx_parseURLDecode(the, src);
					fx_parseURLPercentEncode(the, c, dst, gxUserInfoSet);
				}
				fx_parseURLSetPart(the, result, xsID_password, 0, dst);
				src->offset++;
			}
			else {
				while (src->offset < atOffset) {
					c = fx_parseURLDecode(the, src);
					fx_parseURLPercentEncode(the, c, dst, gxUserInfoSet);
				}
				fx_parseURLSetPart(the, result, xsID_username, 0, dst);
				src->offset++;
			}
			if (src->offset == endOffset)
				goto XS_URL_ERROR;
		}
		hostOffset = src->offset;
		goto XS_URL_HOST;	
	}
	goto XS_URL_AUTHORITY;
	
XS_URL_USERNAME:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_username, 0, dst);
		goto XS_URL_END;
	}
	fx_parseURLPercentEncode(the, c, dst, gxUserInfoSet);
	goto XS_URL_USERNAME;
	
XS_URL_PASSWORD:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_password, 0, dst);
		goto XS_URL_END;
	}
	fx_parseURLPercentEncode(the, c, dst, gxUserInfoSet);
	goto XS_URL_PASSWORD;

XS_URL_HOST:
	c = fx_parseURLDecode(the, src);
	if ((c == ':') && !insideBracket) {
		if (src->offset - 1 == hostOffset)
			goto XS_URL_ERROR;
		if (override == HOST_STATE)
			goto XS_URL_END;
		if (fx_parseURLHost(the, src, hostOffset, src->offset - 1, dst, special)) {
			fx_parseURLSetPart(the, result, xsID_host, 0, dst);
			goto XS_URL_PORT;
		}
		if (override)
			goto XS_URL_END;
		goto XS_URL_ERROR;
	}
	if ((c == C_EOF) || (c == '/') || (special && (c == '\\')) || (c == '?') || (c == '#')) {
		if (special && (src->offset - 1 == hostOffset))
			goto XS_URL_ERROR;
		//@@
		if (fx_parseURLHost(the, src, hostOffset, src->offset - 1, dst, special)) {
			fx_parseURLSetPart(the, result, xsID_host, 0, dst);
			if (c == C_EOF)
				goto XS_URL_END;
			if (c == '?')
				goto XS_URL_QUERY;
			if (c == '#')
				goto XS_URL_FRAGMENT;
			goto XS_URL_PATH;
		}
		if (override)
			goto XS_URL_END;
		goto XS_URL_ERROR;
	}
	if (c == '[')
		insideBracket = 1;
	if (c == ']')
		insideBracket = 0;
	goto XS_URL_HOST;
	
XS_URL_PORT:
	c = fx_parseURLDecode(the, src);
	if (('0' <= c) && (c <= '9')) {
		fx_parseURLEncode(the, c, dst);
		goto XS_URL_PORT;
	}
	if (override || (c == C_EOF) || (c == '/') || (special && (c == '\\')) || (c == '?') || (c == '#')) {
		if (dst->offset != 0) {
			mxPushSlot(dst->slot);
			port = fxToNumber(the, the->stack);
			mxPop();
			if (port > 65535)
				goto XS_URL_ERROR;
			if (special && ((txInteger)port == gx_parseURLSpecialSchemePorts[schemeIndex])) {
    			fx_parseURLEmptyPart(the, result, xsID_port);
    			dst->offset = 0;
    			dst->slot->value.string[0] = 0;
    		}
			else {
				fxIntegerToString(the, (txInteger)port, dst->slot->value.string, dst->size);
				fx_parseURLSetPart(the, result, xsID_port, 0, dst);
			}
		}
		if (override || (c == C_EOF))
			goto XS_URL_END;
		if (c == '?')
			goto XS_URL_QUERY;
		if (c == '#')
			goto XS_URL_FRAGMENT;
		goto XS_URL_PATH;
	}
	goto XS_URL_ERROR;	

XS_URL_FILE_START:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (c == '\\'))
		goto XS_URL_FILE_SLASH;
	if (base && (baseSchemeIndex == 0)) {
		fx_parseURLCopyPart(the, base, result, xsID_host);
		if (fx_parseURLIsWindowsDriveLetter(the, src->slot->value.string + tmp, 1)) {
			src->offset = tmp;
			goto XS_URL_PATH;
		}
		fx_parseURLCopyPath(the, base, result);
		if (c == '?')
			goto XS_URL_QUERY;
		fx_parseURLCopyPart(the, base, result, xsID_query);
		if (c == '#')
			goto XS_URL_FRAGMENT;
		if (c != C_EOF) {
			fx_parseURLShortenPath(the, result);
			src->offset = tmp;
			goto XS_URL_PATH;
		}
        goto XS_URL_END;
	}
    fx_parseURLEmptyPart(the, result, xsID_host);
	if (c == '?')
		goto XS_URL_QUERY;
	if (c == '#')
		goto XS_URL_FRAGMENT;
	if (c != C_EOF) {
		src->offset = tmp;
		goto XS_URL_PATH;
	}
	goto XS_URL_END;
	
XS_URL_FILE_SLASH:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (c == '\\')) {
		hostOffset = src->offset;
		goto XS_URL_FILE_HOST;
	}
	if (base && (baseSchemeIndex == 0)) {
		fx_parseURLCopyPart(the, base, result, xsID_host);
		if (fx_parseURLIsWindowsDriveLetter(the, src->slot->value.string + tmp, 1)) {
			src->offset = tmp;
			goto XS_URL_PATH;
		}
		mxPushSlot(base);
		mxGetID(mxID(_length));
		length = (txIndex)fxToLength(the, the->stack);
		mxPop();
		if (length > 0) {
			mxPushSlot(base);
			mxGetIndex(0);
			if (fx_parseURLIsWindowsDriveLetter(the, the->stack->value.string, 0)) {
				mxPushSlot(result);
				mxSetIndex(0);
				mxPop();
			}
		}
	}
	else
    	fx_parseURLEmptyPart(the, result, xsID_host);
	src->offset = tmp;
	goto XS_URL_PATH;

XS_URL_FILE_HOST:
	c = fx_parseURLDecode(the, src);
	if ((c == C_EOF) || (c == '/') || (c == '\\') || (c == '?') || (c == '#')) {
		if (!override && fx_parseURLIsWindowsDriveLetter(the, src->slot->value.string + hostOffset, 1)) {
 			txInteger endOffset = src->offset - 1;
   			fx_parseURLEmptyPart(the, result, xsID_host);
			src->offset = hostOffset;
			while (src->offset < endOffset) {
				txInteger d = fx_parseURLDecode(the, src);
				fx_parseURLPercentEncode(the, d, dst, gxPathSet);
			}
			src->offset++;
			fx_parseURLPushPath(the, result, dst, 0);
			goto XS_URL_PATH_START;
		}
		if (fx_parseURLHost(the, src, hostOffset, src->offset - 1, dst, special)) {
			if (!c_strcmp(dst->slot->value.string, "localhost")) {
				dst->slot->value.string[0] = 0;
				dst->offset = 0;
			}
			fx_parseURLSetPart(the, result, xsID_host, 0, dst);
			goto XS_URL_PATH_START;
		}
		if (override)
			goto XS_URL_END;
		goto XS_URL_ERROR;
	}
	goto XS_URL_FILE_HOST;
	
XS_URL_PATH_START:
	if (c == C_EOF)
		goto XS_URL_END;
	if (!override && (c == '?'))
		goto XS_URL_QUERY;
	if (!override && (c == '#'))
		goto XS_URL_FRAGMENT;
	
XS_URL_PATH:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLPushPath(the, result, dst, 1);
		goto XS_URL_END;
	}
	if ((c == '/') || (special && (c == '\\'))) {
		fx_parseURLPushPath(the, result, dst, 0);
		goto XS_URL_PATH;
	}
	if (!override && (c == '?')) {
		fx_parseURLPushPath(the, result, dst, 1);
		goto XS_URL_QUERY;
	}
	if (!override && (c == '#')) {
		fx_parseURLPushPath(the, result, dst, 1);
		goto XS_URL_FRAGMENT;
	}
	fx_parseURLPercentEncode(the, c, dst, gxPathSet);
	goto XS_URL_PATH;
	
XS_URL_OPAQUE_PATH:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_path, 0, dst);
		goto XS_URL_END;
	}
	if (!override && (c == '?')) {
		fx_parseURLSetPart(the, result, xsID_path, 0, dst);
		goto XS_URL_QUERY;
	}
	if (!override && (c == '#')) {
		fx_parseURLSetPart(the, result, xsID_path, 0, dst);
		goto XS_URL_FRAGMENT;
	}
	fx_parseURLPercentEncode(the, c, dst, gxControlSet);
	goto XS_URL_OPAQUE_PATH;
	
XS_URL_QUERY:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_query, 0, dst);
		goto XS_URL_END;
	}
	if (!override && (c == '#')) {
		fx_parseURLSetPart(the, result, xsID_query, 0, dst);
		goto XS_URL_FRAGMENT;
	}
	fx_parseURLPercentEncode(the, c, dst, (special) ? gxSpecialQuerySet : gxQuerySet);
	goto XS_URL_QUERY;
	
XS_URL_FRAGMENT:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_fragment, 0, dst);
		goto XS_URL_END;
	}
	fx_parseURLPercentEncode(the, c, dst, gxFragmentSet);
	goto XS_URL_FRAGMENT;
		
XS_URL_ERROR:
	mxTypeError("invalid URL");
	
XS_URL_END:
	return;
}

void fx_parseURLCopyPart(txMachine* the, txSlot* src, txSlot* dst, txID id)
{
	mxPushSlot(src);
	mxGetID(id);
	mxPushSlot(dst);
	mxSetID(id);
	mxPop();
}

void fx_parseURLCopyPath(txMachine* the, txSlot* src, txSlot* dst) 
{
	txIndex length, i;
		
	mxPushSlot(src);
	mxGetID(mxID(_length));
	length = (txIndex)fxToLength(the, the->stack);
	mxPop();
	i = 0;
	while (i < length) {
		mxPushSlot(src);
		mxGetIndex(i);
		mxPushSlot(dst);
		mxSetIndex(i);
		mxPop();
		i++;
	}
}

txInteger fx_parseURLDecode(txMachine* the, txStringStream* stream)
{	
	txInteger result;
	txString string = stream->slot->value.string + stream->offset;
	string = mxStringByteDecode(string, &result);
	if ((0xD000 <= result) && (result <= 0xDFFF))
		result = 0xFFFD;
	stream->offset = string - stream->slot->value.string;
	return result;
}

void fx_parseURLEncode(txMachine* the, txInteger c, txStringStream* dst)
{
	txSize length = mxStringByteLength(c);
	txString string;
	if (dst->offset + length >= dst->size) {
		txSize size = fxAddChunkSizes(the, dst->offset, length + 1);
		txString string = fxRenewChunk(the, dst->slot->value.string, size);
		if (!string) {
			string = (txString)fxNewChunk(the, size);
			c_memcpy(string, dst->slot->value.string, dst->offset);
		}
		dst->slot->value.string = string;
		dst->size = size;
	}
	string = dst->slot->value.string + dst->offset;
	string = mxStringByteEncode(string, c);
	dst->offset += length;
	*string = 0;
}

void fx_parseURLEmptyPart(txMachine* the, txSlot* target, txID id) 
{
	mxPush(mxEmptyString);
	mxPushSlot(target);
	mxSetID(id);
	mxPop();
}

txBoolean fx_parseURLHasOpaquePath(txMachine* the, txSlot* parts)
{
	mxPushSlot(parts);
	return mxHasID(xsID_path);
}

txBoolean fx_parseURLHost(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst, txBoolean special)
{
	txString p = src->slot->value.string;
	if ((to - from > 1) && (p[from] == '[') && (p[to - 1] == ']')) {
		return fx_parseURLHostIPv6(	the, src, from + 1, to - 1, dst);
	}
	if (special) {
		if (fx_parseURLHostSpecial(the, src, from, to, dst))
			return fx_parseURLHostIPv4(the, src, from, to, dst);
		return 0;
	}
	return fx_parseURLHostNotSpecial(the, src, from, to, dst);
}

txBoolean fx_parseURLHostIPv4(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst)
{
	txS8 pieces[4];
	txS8 value;
	txS8 sum;
	txInteger pieceCount = 0;
	txInteger pieceIndex;
	txString p = dst->slot->value.string;
	txString q = dst->slot->value.string + dst->offset;
	while (p < q) {
		value = -1;
		if (*p == '0') {
			value = 0;
			p++;
			if ((p < q) && ((*p == 'x') || (*p == 'X'))) {
				p++;
				while (p < q) {
					if (('0' <= *p) && (*p <= '9'))
						value = (value * 16) + (*p - '0');
					else if (('a' <= *p) && (*p <= 'f'))
						value = (value * 16) + (10 + *p - 'a');
					else if (('A' <= *p) && (*p <= 'F'))
						value = (value * 16) + (10 + *p - 'A');
					else
						break;
					p++;

				}
			}
			else {
				while (p < q) {
					if (('0' <= *p) && (*p <= '7'))
						value = (value * 8) + (*p - '0');
					else if (('8' <= *p) && (*p <= '9'))
						value += 0x0000000100000000;
					else
						break;
					p++;
				}
			}
		}
		else if (('1' <= *p) && (*p <= '9')) {
			value = *p - '0';
			p++;
			while (p < q) {
				if (('0' <= *p) && (*p <= '9'))
					value = (value * 10) + (*p - '0');
				else
					break;
				p++;
			}
		} 
		if ((p < q) && (*p != '.')) {
			value = -1;
			while ((p < q) && (*p != '.'))
				p++;
		}
		if (pieceCount < 4)
			pieces[pieceCount] = value;
		else
			pieces[3] = value;
		pieceCount++;
		if (*p == '.')
			p++;
	}
	if (pieceCount == 0)
		return 1;
	pieceIndex = pieceCount - 1;
	if (pieceIndex > 3)
		pieceIndex = 3;
	if (pieces[pieceIndex] == -1)
		return 1;
	if (pieceCount > 4)
		return 0;
	sum = pieces[pieceIndex];
	value = 256;
	pieceCount = 4 - pieceCount;
	while (pieceCount > 0) {
		value <<= 8;
		pieceCount--;
	}
	if (sum >= value)
		return 0;
	pieceIndex--;
	while (pieceIndex >= 0) {
		value = pieces[pieceIndex];
		if ((value < 0) || (255 < value))
			return 0;
		pieceCount = 3 - pieceIndex;
		while (pieceCount > 0) {
			value <<= 8;
			pieceCount--;
		}
		sum += value;
		pieceIndex--;
	}
	if (sum > 0x00000000FFFFFFFF)
		return 0;
	pieces[0] = (sum & 0x00000000FF000000) >> 24;
	pieces[1] = (sum & 0x0000000000FF0000) >> 16;
	pieces[2] = (sum & 0x000000000000FF00) >> 8;
	pieces[3] = (sum & 0x00000000000000FF);
	dst->offset = 0;
	dst->slot->value.string[0] = 0;
	for (pieceIndex = 0; pieceIndex < 4; pieceIndex++) {
		value = pieces[pieceIndex];
		if (value >= 100) {
			fx_parseURLEncode(the, '0' + ((txInteger)value / 100), dst);
			value %= 100;
			fx_parseURLEncode(the, '0' + ((txInteger)value / 10), dst);
			value %= 10;
		}
		else if (value >= 10)  {
			fx_parseURLEncode(the, '0' + ((txInteger)value / 10), dst);
			value %= 10;
		}
		fx_parseURLEncode(the, '0' + (txInteger)value, dst);
		if (pieceIndex < 3)
			fx_parseURLEncode(the, '.', dst);
	}
	return 1;
}

txBoolean fx_parseURLHostIPv6(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst)
{
	txU2 pieces[8];
	txInteger pieceIndex = 0;
	txInteger compress = -1;
	txU2 value;
	txInteger length;
	txInteger numbersSeen;
	txString p = src->slot->value.string + from;
	txString q = src->slot->value.string + to;
	c_memset(pieces, 0, sizeof(pieces));
	if (p[0] == ':') {
		if (p[1] != ':')
			return 0;
		p += 2;
		pieceIndex++;
		compress = pieceIndex;
	}
	while (p < q) {
		if (pieceIndex == 8)
			return 0;
		if (*p == ':') {
			if (compress != -1)
				return 0;
            p++;
            pieceIndex++;
			compress = pieceIndex;
			continue;
		}
		value = 0;
		length = 0;
		while ((p < q) && (length < 4)) {
			if (('0' <= *p) && (*p <= '9'))
				value = (value * 16) + (*p - '0');
			else if (('a' <= *p) && (*p <= 'f'))
				value = (value * 16) + (10 + *p - 'a');
			else if (('A' <= *p) && (*p <= 'F'))
				value = (value * 16) + (10 + *p - 'A');
			else
				break;
			length++;
			p++;
		}
		if (*p == '.') {
			if (length == 0)
				return 0;
			p -= length;
			if (pieceIndex > 6)
				return 0;
			numbersSeen = 0;
			while (p < q) {
				txInteger ipv4Piece = -1;
				if (numbersSeen > 0) {
					if ((*p == '.') && (numbersSeen < 4))
						p++;
					else
						return 0;
				}
				if ((p < q) && ('0' <= *p) && (*p <= '9')) {
					while ((p < q) && ('0' <= *p) && (*p <= '9')) {
						if (ipv4Piece == 0)
							return 0;
						if (ipv4Piece < 0)
							ipv4Piece = *p - '0';
						else 
							ipv4Piece = (ipv4Piece * 10) + (*p - '0');
						if (ipv4Piece > 255)
							return 0;
						p++;
					}
					pieces[pieceIndex] = (pieces[pieceIndex] * 256) + ipv4Piece;
					numbersSeen++;
					if ((numbersSeen == 2) || (numbersSeen == 4))
						pieceIndex++;
				}
				else
					return 0;
			}	
			if (numbersSeen != 4)
				return 0;
			break;
		}
		else if (*p == ':') {
			p++;
			if (p == q)
				return 0;
		}
		else if (p < q) 
			return 0;
		pieces[pieceIndex] = value;
		pieceIndex++;
	}
	if (compress != -1) {
		txInteger swaps = pieceIndex - compress;
		pieceIndex = 7;
		while ((pieceIndex != 0) && (swaps > 0)) {
			txU2 piece = pieces[compress + swaps - 1];
			pieces[compress + swaps - 1] = pieces[pieceIndex];
			pieces[pieceIndex] = piece;
			pieceIndex--;
			swaps--;
		}
	}
	else if (pieceIndex != 8)
		return 0;
		
	compress = -1;
	length = 0;
	pieceIndex = 0;
	while (pieceIndex < 8) {
		if (pieces[pieceIndex] == 0) {
			txInteger zeroCompress = pieceIndex;
			txInteger zeroLength = 1;
			pieceIndex++;
			while (pieceIndex < 8) {
				if (pieces[pieceIndex] != 0)
					break;
				pieceIndex++;
				zeroLength++;
			}
			if (zeroLength > length) {
				compress = zeroCompress;
				length = zeroLength;
			}
			
		}
		pieceIndex++;
	}
	if (length < 2)
		compress = -1;
		
	txBoolean ignore0 = 0;
	fx_parseURLEncode(the, '[', dst);
	for (pieceIndex = 0; pieceIndex < 8; pieceIndex++) {
		value = pieces[pieceIndex];
		if (ignore0 && (value == 0))
			continue;
		else
			ignore0 = 0;
		if (compress == pieceIndex) {
			if (pieceIndex == 0)
				fx_parseURLEncode(the, ':', dst);
			fx_parseURLEncode(the, ':', dst);
			ignore0 = 1;
			continue;
		}
		txInteger flag = 0;
		txInteger shift = 12;
		while (shift > 0) {
			txInteger digit = (value >> shift) & 0x000F;
			flag |= digit;
			if (flag)
				fx_parseURLEncode(the, gxHexLower[digit], dst);
			shift -= 4;
		}
		fx_parseURLEncode(the, gxHexLower[value & 0x000F], dst);
		if (pieceIndex != 7)
			fx_parseURLEncode(the, ':', dst);
	}
	fx_parseURLEncode(the, ']', dst);
	return 1;
}

txBoolean fx_parseURLHostNotSpecial(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst)
{
	txBoolean result = 1;
	txInteger offset = src->offset;
	src->offset = from;
	while (src->offset < to) {
		txInteger c = fx_parseURLDecode(the, src);
		if ((c < 0x80) && c_read8(gxNotSpecialHostSet + c)) {
			result = 0;
			break;
		}
		fx_parseURLPercentEncode(the, c, dst, gxControlSet);
	}
	src->offset = offset;
	return result;
}

txBoolean fx_parseURLHostSpecial(txMachine* the, txStringStream* src, txInteger from, txInteger to, txStringStream* dst)
{
	txBoolean result = 1;
	txInteger offset = src->offset;
	txInteger flag = 0;
	src->offset = from;
	while (src->offset < to) {
		txInteger c = fx_parseURLDecode(the, src);
		if (c < 0x80) {
			if (('A' <= c) && (c <= 'Z'))
				c -= ('A' - 'a');
			else if (c == '%')
				flag |= 1;
			else if (c_read8(gxSpecialHostSet + c)) {
				result = 0;
				break;
			}
		}
		else {
			
			flag |= 2;
		}
		fx_parseURLEncode(the, c, dst);
	}
	if (result && (flag & 1)) {
		txStringStream _tmp;
		txStringStream* tmp = &_tmp;
		mxTry(the) {
			mxPush(mxGlobal);
			mxDub();
			mxGetID(mxID(_decodeURIComponent));
			mxCall();
			mxPushSlot(dst->slot);
			mxRunCount(1);
			tmp->slot = the->stack;
			tmp->offset = 0;
			tmp->size = 0;
			dst->offset = 0;
			dst->slot->value.string[0] = 0;
			for (;;) {
				txInteger c = fx_parseURLDecode(the, tmp);
				if (c == C_EOF)
					break;
				if (c < 0x80) {
					if (c_read8(gxSpecialHostSet + c)) {
						result = 0;
						break;
					}
					if (('A' <= c) && (c <= 'Z'))
						c -= ('A' - 'a');
				}
				else
					flag |= 2;
				fx_parseURLEncode(the, c, dst);
			}
			mxPop();
		}
		mxCatch(the) {
			result = 0;
		}
	}
	if (result && (flag & 2)) {
		// punycode
		mxSyntaxError("no punycode yet");
		result = 0;
	}
	src->offset = offset;
	return result;
}

txBoolean fx_parseURLIsPartEmpty(txMachine* the, txSlot* parts, txID id)
{
	txBoolean result;
	mxPushSlot(parts);
	mxGetID(id);
	result = the->stack->value.string[0] == 0;
	mxPop();
	return result;
}

txBoolean fx_parseURLIsWindowsDriveLetter(txMachine* the, txString string, txBoolean startsWith)
{
	char c = *string++;
	if ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z'))) {
		c = *string++;
		if ((c == ':') || (c == '|')) {
			c = *string++;
			if (c == 0)
				return 1;
			if (startsWith && ((c == '/') || (c == '\\') || (c == '?') || (c == '#')))
				return 1;
		}
	}
	return 0;
}

void fx_parseURLNullPart(txMachine* the, txSlot* target, txID id) 
{
	mxPushNull();
	mxPushSlot(target);
	mxSetID(id);
	mxPop();
}

void fx_parseURLPercentEncode(txMachine* the, txInteger c, txStringStream* dst, const char* set)
{
	txSize length;
	txString string;
	if (c < 0x80) {
		if (c_read8(set + c))
			length = 3;
		else
			length = 1;
	}
	else if (c < 0x800) {
		length = 6;
	}
	else if (c < 0x10000) {
		length = 9;
	}
	else {
		length = 12;
	}
	if (dst->offset + length >= dst->size) {
		txSize size = fxAddChunkSizes(the, dst->offset, length + 1);
		txString string = fxRenewChunk(the, dst->slot->value.string, size);
		if (!string) {
			string = (txString)fxNewChunk(the, size);
			c_memcpy(string, dst->slot->value.string, dst->offset);
		}
		dst->slot->value.string = string;
		dst->size = size;
	}
	string = dst->slot->value.string + dst->offset;
	if (length == 1)
		*string++ = (char)c;
	else if (length == 3) {
		*string++ = '%';
		string = fxStringifyHexEscape(string, c);
	}
	else if (length == 6) {
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0xC0 | (c >> 6));
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0x80 | (c & 0x3F));
	}
	else if (length == 9) {
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0xE0 | (c >> 12));
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0x80 | ((c >> 6) & 0x3F));
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0x80 | (c & 0x3F));
	}
	else {
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0xF0 | (c >> 18));
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0x80 | ((c >> 12) & 0x3F));
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0x80 | ((c >> 6) & 0x3F));
		*string++ = '%';
		string = fxStringifyHexEscape(string, 0x80 | (c & 0x3F));
	}
	dst->offset += length;
	*string = 0;
}

void fx_parseURLPushPath(txMachine* the, txSlot* path, txStringStream* dst, txBoolean flag) 
{
	txSize size;
	txString string;
	if ((c_strcmp(dst->slot->value.string, "..") == 0)
		|| (c_strcmp(dst->slot->value.string, ".%2E") == 0)
		|| (c_strcmp(dst->slot->value.string, "%2E.") == 0)
		|| (c_strcmp(dst->slot->value.string, "%2E%2E") == 0)
		|| (c_strcmp(dst->slot->value.string, ".%2e") == 0)
		|| (c_strcmp(dst->slot->value.string, "%2e.") == 0)
		|| (c_strcmp(dst->slot->value.string, "%2e%2e") == 0)) {
		fx_parseURLShortenPath(the, path);
		if (flag) {
			dst->slot->value.string[0] = 0;
			dst->offset = 0;
		}
	}
	else if ((c_strcmp(dst->slot->value.string, ".") == 0)
		|| (c_strcmp(dst->slot->value.string, "%2E") == 0)
		|| (c_strcmp(dst->slot->value.string, "%2e") == 0)) {
		if (flag) {
			dst->slot->value.string[0] = 0;
			dst->offset = 0;
		}
	}
	else {
		txIndex length;
		mxPushSlot(path);
		mxGetID(mxID(_length));
		length = (txIndex)fxToLength(the, the->stack);
		mxPop();
		if ((length == 0) && fx_parseURLIsWindowsDriveLetter(the, dst->slot->value.string, 0))
			dst->slot->value.string[1] = ':';
		flag = 1;
	}
	if (flag) {
		mxPushSlot(path);
		mxDub();
		mxGetID(mxID(_push));
		mxCall();
		size = dst->offset + 1;
		string = fxRenewChunk(the, dst->slot->value.string, size);
		if (!string) {
			string = (txString)fxNewChunk(the, size);
			c_memcpy(string, dst->slot->value.string, size);
		}
		mxPushString(string);	
		mxRunCount(1);
		mxPop();
	}
	dst->slot->value.string = fxNewChunk(the, mx_parseURLBufferSize);
	dst->size = mx_parseURLBufferSize;
	dst->offset = 0;
	dst->slot->value.string[0] = 0;
}

txInteger fx_parseURLSpecialScheme(txMachine* the, txString scheme) 
{
	txInteger i;
	for (i = 0; i < mx_parseURLSpecialSchemeCount; i++) {
		if (!c_strcmp(scheme, gx_parseURLSpecialSchemeNames[i]))
			return i;
	}
	return -1;
}

void fx_parseURLSetPart(txMachine* the, txSlot* target, txID id, txIndex index, txStringStream* dst) 
{
	txSize size = dst->offset + 1;
	txString string = fxRenewChunk(the, dst->slot->value.string, size);
	if (!string) {
		string = (txString)fxNewChunk(the, size);
		c_memcpy(string, dst->slot->value.string, size);
	}
	mxPushString(string);	
	mxPushSlot(target);
	mxSetAll(id, index);
	mxPop();
	dst->slot->value.string = fxNewChunk(the, mx_parseURLBufferSize);
	dst->size = mx_parseURLBufferSize;
	dst->offset = 0;
	dst->slot->value.string[0] = 0;
}

void fx_parseURLShortenPath(txMachine* the, txSlot* path) 
{
	txIndex length;
	txBoolean flag;
	mxPushSlot(path);
	mxGetID(mxID(_length));
	length = (txIndex)fxToLength(the, the->stack);
	mxPop();
	if (length == 1) {
		mxPushSlot(path);
		mxGetIndex(0);
		flag = fx_parseURLIsWindowsDriveLetter(the, the->stack->value.string, 0);
		mxPop();
		if (flag)
			return;	
	}
	if (length > 0) {
		length--;
		mxPushInteger(length);
		mxPushSlot(path);
		mxSetID(mxID(_length));
		mxPop();
	}
}

void fx_serializeURL(txMachine* the) 
{
	txSlot* parts;
	txInteger schemeIndex;
	txIndex length;
	txInteger override = 0;
	txSlot* result;
	
    fxVars(the, 3);
	result = mxResult;
	fxCopyStringC(the, result, "");
	
	parts = mxArgv(0);
	
	mxPushSlot(parts);
	mxGetID(xsID_scheme);
	schemeIndex = fx_parseURLSpecialScheme(the, the->stack->value.string);
	mxPop();
	
	mxPushSlot(parts);
	mxGetID(mxID(_length));
	length = (txIndex)fxToLength(the, the->stack);
	mxPop();

	if (mxArgc > 1) {
   		override = fxToInteger(the, mxArgv(1));
    	if (override == SCHEME_STATE) {
			goto XS_URL_SCHEME;
    	}
    	if (override == USERNAME_STATE) {
			goto XS_URL_USERNAME;
    	}
    	if (override == PASSWORD_STATE) {
			goto XS_URL_PASSWORD;
		}
    	if (override == HOST_PORT_STATE) {
			goto XS_URL_HOST;
     	}
   		if (override == HOST_STATE) {
			goto XS_URL_HOST;
    	}
    	if (override == PORT_STATE) {
     		goto XS_URL_PORT;
    	}
    	if (override == PATH_STATE) {
  			goto XS_URL_PATH;
    	}
    	if (override == QUERY_STATE) {
   			goto XS_URL_QUERY;
    	}
    	if (override == FRAGMENT_STATE) {
   			goto XS_URL_FRAGMENT;
    	}
    	if (override == ORIGIN_STATE) {
   			goto XS_URL_ORIGIN;
    	}
    	goto XS_URL_END;
	}

XS_URL_SCHEME:
	mxPushSlot(parts);
	mxGetID(xsID_scheme);
	fxConcatString(the, result, the->stack);
	mxPop();
	fxConcatStringC(the, result, ":");
	if (override == SCHEME_STATE)
		goto XS_URL_END;

	mxPushSlot(parts);
	mxGetID(xsID_host);
	mxPullSlot(mxVarv(0));
	if (mxIsNull(mxVarv(0))) {
		if (length > 1) {
			mxPushSlot(parts);
			mxGetIndex(0);
			if (the->stack->value.string[0] == 0)
				fxConcatStringC(the, result, "/.");
			mxPop();
		}
		goto XS_URL_PATH;
	}
	
	fxConcatStringC(the, result, "//");

	mxPushSlot(parts);
	mxGetID(xsID_username);
	mxPullSlot(mxVarv(1));
	mxPushSlot(parts);
	mxGetID(xsID_password);
	mxPullSlot(mxVarv(2));
	if (mxVarv(1)->value.string[0] || mxVarv(2)->value.string[0]) {
		fxConcatString(the, result, mxVarv(1));
		if (mxVarv(2)->value.string[0]) {
			fxConcatStringC(the, result, ":");
			fxConcatString(the, result, mxVarv(2));
		}
		fxConcatStringC(the, result, "@");
	}
	fxConcatString(the, result, mxVarv(0));
	goto XS_URL_PORT;
	
XS_URL_USERNAME:
	mxPushSlot(parts);
	mxGetID(xsID_username);
	fxConcatString(the, result, the->stack);
	mxPop();
	goto XS_URL_END;
	
XS_URL_PASSWORD:
	mxPushSlot(parts);
	mxGetID(xsID_password);
	fxConcatString(the, result, the->stack);
	mxPop();
	goto XS_URL_END;
	
XS_URL_ORIGIN:
	if (schemeIndex < 0) {
		fxConcatStringC(the, result, "null");
		goto XS_URL_END;
	}
	mxPushSlot(parts);
	mxGetID(xsID_host);
	mxPullSlot(mxVarv(0));
	if (!mxVarv(0)->value.string[0]) {
		fxConcatStringC(the, result, "null");
		goto XS_URL_END;
	}
	mxPushSlot(parts);
	mxGetID(xsID_scheme);
	fxConcatString(the, result, the->stack);
	mxPop();
	fxConcatStringC(the, result, "://");
	fxConcatString(the, result, mxVarv(0));
	goto XS_URL_PORT;

XS_URL_HOST:
	mxPushSlot(parts);
	mxGetID(xsID_host);
	mxPullSlot(mxVarv(0));
	fxConcatString(the, result, mxVarv(0));
	if (override == HOST_STATE)
		goto XS_URL_END;

XS_URL_PORT:
	mxPushSlot(parts);
	mxGetID(xsID_port);
	mxPullSlot(mxVarv(0));
	if (mxVarv(0)->value.string[0]) {
		if (override != PORT_STATE)
			fxConcatStringC(the, result, ":");
		fxConcatString(the, result, mxVarv(0));
	}
	if ((override == HOST_PORT_STATE) || (override == PORT_STATE) || (override == ORIGIN_STATE))
		goto XS_URL_END;
	
XS_URL_PATH:
	mxPushSlot(parts);
	if (mxHasID(xsID_path)) {
		mxPushSlot(parts);
		mxGetID(xsID_path);
		fxConcatString(the, result, the->stack);
		mxPop();
	}
	else {
		if (length == 0) {
			if (schemeIndex >= 0)
				fxConcatStringC(the, result, "/");
		}
		else {
			txIndex index = 0;
			while (index < length) {
				fxConcatStringC(the, result, "/");
				mxPushSlot(parts);
				mxGetIndex(index);
				fxConcatString(the, result, the->stack);
				mxPop();
				index++;
			}
		}
	}
	if (override == PATH_STATE)
		goto XS_URL_END;

XS_URL_QUERY:
	mxPushSlot(parts);
	mxGetID(xsID_query);
	if (!mxIsNull(the->stack)) {
		if (!override || the->stack->value.string[0])
			fxConcatStringC(the, result, "?");
		fxConcatString(the, result, the->stack);
	}
	mxPop();
	if (override)
		goto XS_URL_END;
		
XS_URL_FRAGMENT:
	mxPushSlot(parts);
	mxGetID(xsID_fragment);
	if (!mxIsNull(the->stack)) {
		if (!override || the->stack->value.string[0])
			fxConcatStringC(the, result, "#");
		fxConcatString(the, result, the->stack);
	}
	mxPop();
		
XS_URL_END:
	return;		
}
	
void fx_parseQuery(txMachine* the) 
{
	txSlot* result;
	txSlot* string;
	txSlot* pair;
	txSlot* push;
	txInteger offset, ampersand, equal;
	fxVars(the, 2);
	result = mxResult;
	string = mxArgv(0);
	pair = mxVarv(0);
	push = mxVarv(1);
	
	fxNewArray(the, 0);
	mxPullSlot(result);

	fxToString(the, string);

	mxPushSlot(result);
	mxGetID(mxID(_push));
	mxPullSlot(push);

	offset = 0;
	ampersand = 0;
	equal = 0;
	if (string->value.string[0] == '?') {
		offset++;
		ampersand++;
		equal++;
	}
	for (;;) {
		char c = string->value.string[offset];
		if ((c == 0) || (c == '&')) {
			if (offset > ampersand) {
				mxPushSlot(result);
				mxPushSlot(push);
				mxCall();
				
				fxNewInstance(the);
				mxPullSlot(pair);
				
				if (equal == ampersand)
					equal = offset;
				fx_parseQueryPlus(the, string, ampersand, equal);
				mxPushSlot(pair);
				mxSetID(mxID(_name));
				mxPop();
				
				equal++;
				
				if (equal < offset)
					fx_parseQueryPlus(the, string, equal, offset);
				else
					mxPush(mxEmptyString);
				mxPushSlot(pair);
				mxSetID(mxID(_value));
				mxPop();
                
                mxPushSlot(pair);
				mxRunCount(1);
				mxPop();
			}
			if (c == 0)
				break;
			ampersand = offset + 1;
			equal = offset + 1;
		}
		else if (c == '=') {
			equal = offset;
		}
		offset++;
	}
}

void fx_parseQueryPlus(txMachine* the, txSlot* string, txInteger from, txInteger to) 
{
	txString src;
	txString limit;
	txInteger length;
	txInteger c, d;
	txString dst;
	src = string->value.string + from;
	limit = string->value.string + to;
	length = 0;
	while (src < limit) {
		c = c_read8(src++);
		if (c == '%')
			fxParseHexEscape(&src, &d);
		length++;
	}		
	length += 1;
	mxPushUndefined();
	the->stack->value.string = fxNewChunk(the, length);
	the->stack->kind = XS_STRING_KIND;
	src = string->value.string + from;
	limit = string->value.string + to;
	dst = the->stack->value.string;
	while ((src < limit)) {
		c = c_read8(src++);
		if (c == '%') {
			if (fxParseHexEscape(&src, &d))
				*dst++ = (char)d;
			else
				*dst++ = '%';
		}
		else if (c == '+')
			*dst++ = ' ';
		else
			*dst++ = (char)c;
	}
	*dst = 0;
}

void fx_serializeQuery(txMachine* the) 
{
	txSlot* pairs;
	txSlot* pair;
	txSlot* result;
	
	txInteger length, index;
	fxVars(the, 3);
	pairs = mxVarv(0);
	pair = mxVarv(1);
	result = mxVarv(2);

	mxPushSlot(mxArgv(0));
	mxPullSlot(pairs);
	
	mxPush(mxEmptyString);
	mxPullSlot(result);

	mxPushSlot(pairs);
	mxGetID(mxID(_length));
	length = (txIndex)fxToLength(the, the->stack);
	mxPop();

	index = 0;
	while (index < length) {
		if (index > 0)
			fxConcatStringC(the, result, "&");
		mxPushSlot(pairs);
		mxGetIndex(index);
		mxPullSlot(pair);
				
		mxPushSlot(pair);
		mxGetID(mxID(_name));
		mxPullSlot(mxArgv(0));
		fx_serializeQueryPlus(the, (txString)gxFormSet);
		fxConcatString(the, result, mxResult);
		
		fxConcatStringC(the, result, "=");
		
		mxPushSlot(pair);
		mxGetID(mxID(_value));
		mxPullSlot(mxArgv(0));
		fx_serializeQueryPlus(the, (txString)gxFormSet);
		fxConcatString(the, result, mxResult);
		index++;
	}	
	
	*mxResult = *result;
}

void fx_serializeQueryPlus(txMachine* the, txString theSet)
{
	txString src;
	txInteger length;
	txBoolean same = 1;
	txInteger c;
	txString dst;

	src = fxToString(the, mxArgv(0));
	length = 0;
	while (((src = mxStringByteDecode(src, &c))) && (c != C_EOF)) {
		if (c < 0x80) {
			if (c == ' ') {
				length += 1;
				same = 0;
			}
			else if (c_read8(theSet + c))
				length += 1;
			else {
				length += 3;
				same = 0;
			}
		}
		else {
			if (c < 0x800)
				length += 6;
			else if ((0xD800 <= c) && (c <= 0xDFFF))
				mxURIError("invalid string");
			else if (c < 0x10000)
				length += 9;
			else
				length += 12;
			same = 0;
		}
	}
	length += 1;
	if (same) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = mxArgv(0)->kind;
		return;
	}
	mxResult->value.string = fxNewChunk(the, length);
	mxResult->kind = XS_STRING_KIND;
	src = mxArgv(0)->value.string;
	dst = mxResult->value.string;
	while (((src = mxStringByteDecode(src, &c))) && (c != C_EOF)) {
		if (c < 0x80) {
			if (c == ' ')
				*dst++ = '+';
			else if (c_read8(theSet + c))
				*dst++ = (char)c;
			else {
				*dst++ = '%';
				dst = fxStringifyHexEscape(dst, c);
			}
		}
		else if (c < 0x800) {
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0xC0 | (c >> 6));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | (c & 0x3F));
		}
		else if (c < 0x10000) {
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0xE0 | (c >> 12));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | ((c >> 6) & 0x3F));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | (c & 0x3F));
		}
		else {
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0xF0 | (c >> 18));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | ((c >> 12) & 0x3F));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | ((c >> 6) & 0x3F));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | (c & 0x3F));
		}
	}
	*dst = 0;
}


