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


static txBoolean fx_parseURLCanHaveUserInfoPort(txMachine* the, txSlot* parts);
static void fx_parseURLCopyPath(txMachine* the, txSlot* src, txSlot* dst);
static txInteger fx_parseURLDecode(txMachine* the, txStringStream* stream);
static txBoolean fx_parseURLEncode(txMachine* the, txInteger c, txStringStream* src, txStringStream* dst, const char* set);
static txBoolean fx_parseURLHasOpaquePath(txMachine* the, txSlot* parts);
static txBoolean fx_parseURLIsWindowsDriveLetter(txMachine* the, txString string, txBoolean startsWith);
static void fx_parseURLPushPath(txMachine* the, txSlot* path, txStringStream* dst, txBoolean flag);
static void fx_parseURLSetPart(txMachine* the, txSlot* target, txID id, txIndex index, txStringStream* dst); 
static void fx_parseURLShortenPath(txMachine* the, txSlot* path);
static txInteger fx_parseURLSpecialScheme(txMachine* the, txString scheme);
static void fx_parseQueryPlus(txMachine* the, txSlot* string, txInteger from, txInteger to);
static void fx_serializeQueryPlus(txMachine* the, txString theSet);

static const char ICACHE_RODATA_ATTR gxEmptySet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxUserInfoSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxPathSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 0x                    */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 1x                    */
	 1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
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
	 1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
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

#define mx_parseURLBufferSize 32
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
	HOST_STATE,
	HOSTNAME_STATE,
	PORT_STATE,
	PATH_STATE,
	QUERY_STATE,
	FRAGMENT_STATE,
};

void fx_parseURL(txMachine* the) 
{
	
	txStringStream _src;
	txStringStream* src = &_src;
	txStringStream _dst;
	txStringStream* dst = &_dst;

	txSlot* base = C_NULL;
	txInteger baseSchemeIndex = -1;

	txInteger override = 0;
	txSlot* result = C_NULL;
	txInteger schemeIndex = -1;
	
	txBoolean isSpecial = 0;
	txBoolean wasSpecial = 0;

	txIndex length;
	txSize tmp;
	txInteger c;
    
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
    	if (override == SCHEME_STATE) {
			goto SCHEME_START;
    	}
    	if (override == USERNAME_STATE) {
    		if (fx_parseURLCanHaveUserInfoPort(the, result))
				goto USERNAME;
 			goto END;
    	}
    	if (override == PASSWORD_STATE) {
    		if (fx_parseURLCanHaveUserInfoPort(the, result))
				goto PASSWORD;
 			goto END;
		}
    	if (override == HOST_STATE) {
	    	if (fx_parseURLHasOpaquePath(the, result))
 				goto END;
			goto HOST;
     	}
   		if (override == HOSTNAME_STATE) {
	    	if (fx_parseURLHasOpaquePath(the, result))
 				goto END;
			goto HOST;
    	}
    	if (override == PORT_STATE) {
     		if (fx_parseURLCanHaveUserInfoPort(the, result))
   				goto PORT;
 			goto END;
    	}
    	if (override == PATH_STATE) {
    		if (fx_parseURLHasOpaquePath(the, result))
 				goto END;
			mxPushInteger(0);
			mxPushSlot(result);
			mxSetID(mxID(_length));
			mxPop();
 			c = fx_parseURLDecode(the, src);
   			if (c != '/')
    			src->offset = 0;
  			goto PATH;
    	}
    	if (override == QUERY_STATE) {
			mxPush(mxEmptyString);
			mxPushSlot(result);
			mxSetID(xsID_query);
			mxPop();
 			c = fx_parseURLDecode(the, src);
   			if (c != '?')
    			src->offset = 0;
   			goto QUERY;
    	}
    	if (override == FRAGMENT_STATE) {
			mxPush(mxEmptyString);
			mxPushSlot(result);
			mxSetID(xsID_fragment);
			mxPop();
 			c = fx_parseURLDecode(the, src);
   			if (c != '#')
    			src->offset = 0;
   			goto FRAGMENT;
    	}
    	goto ERROR;
   }
    else {
		fxNewArray(the, 0);
		mxPullSlot(mxResult);
		result = mxResult;
		
		mxPush(mxEmptyString);
		mxPushSlot(result);
		mxSetID(xsID_username);
		mxPop();
		mxPush(mxEmptyString);
		mxPushSlot(result);
		mxSetID(xsID_password);
		mxPop();
		mxPushNull();
		mxPushSlot(result);
		mxSetID(xsID_host);
		mxPop();
		mxPushNull();
		mxPushSlot(result);
		mxSetID(xsID_port);
		mxPop();
		mxPush(mxEmptyString);
		mxPushSlot(result);
		mxSetID(xsID_query);
		mxPop();
		mxPush(mxEmptyString);
		mxPushSlot(result);
		mxSetID(xsID_fragment);
		mxPop();
	
    	if ((mxArgc > 1) && mxIsReference(mxArgv(1))) {
    		base = mxArgv(1);
			mxPushSlot(base);
			mxGetID(xsID_scheme);
			baseSchemeIndex = fx_parseURLSpecialScheme(the, the->stack->value.string);
			mxPop();
    	}
    }

SCHEME_START:
	c = fx_parseURLDecode(the, src);
	if (('A' <= c) && (c <= 'Z')) {
		fx_parseURLEncode(the, c - ('A' - 'a'), src, dst, gxEmptySet);
		goto SCHEME;
	}
	if (('a' <= c) && (c <= 'z')) {
		fx_parseURLEncode(the, c, src, dst, gxEmptySet);
		goto SCHEME;
	}
	if (override)
		goto ERROR;
	src->offset = 0;
 	dst->offset = 0;
	goto NO_SCHEME;

SCHEME:
	c = fx_parseURLDecode(the, src);
	if (('A' <= c) && (c <= 'Z')) {
		fx_parseURLEncode(the, c - ('A' - 'a'), src, dst, gxEmptySet);
		goto SCHEME;
	}
	if ((('a' <= c) && (c <= 'z')) || (c == '+') || (c == '-') || (c == '.')) {
		fx_parseURLEncode(the, c, src, dst, gxEmptySet);
		goto SCHEME;
	}
	if ((c == ':') || (override && (c == C_EOF))) {
		schemeIndex = fx_parseURLSpecialScheme(the, mxVarv(0)->value.string);
    	isSpecial = schemeIndex >= 0;
		if (override) {
    		mxPushSlot(result);
    		mxGetID(xsID_scheme);
    		wasSpecial = fx_parseURLSpecialScheme(the, the->stack->value.string) >= 0;
    		mxPop();
    		if (wasSpecial != isSpecial)
    			return;
		}
		fx_parseURLSetPart(the, result, xsID_scheme, 0, dst);
		if (override) {
// 			if (isSpecial) { 
//     			mxPushSlot(result);
//     			mxGetID(xsID_port);
// 				port = fxToInteger(the, the->stack);
// 				mxPop();
// 				if (port == gx_parseURLSpecialSchemePorts[special]) {
// 				
// 				}
// 			}
			goto END;
		}
		if (schemeIndex == 0)
			goto FILE_START;
		if (isSpecial) {
			if (base && (schemeIndex == baseSchemeIndex))
    			goto RELATIVE;
			goto AUTHORITY_START;
		}
		
		tmp = src->offset;
		c = fx_parseURLDecode(the, src);
		if (c == '/') {
			tmp = src->offset;
			c = fx_parseURLDecode(the, src);
			if (c == '/')
				goto AUTHORITY_START;
			src->offset = tmp;
			c = '/';
			goto PATH_START;
		}
		src->offset = tmp;
		goto OPAQUE_PATH;
	}
	if (override)
		goto ERROR;
	src->offset = 0;
 	dst->offset = 0;
		
NO_SCHEME:
	if (!base)
		goto ERROR;
	mxPushSlot(base);
	mxGetID(xsID_scheme);
	mxPushSlot(result);
	mxSetID(xsID_scheme);
	mxPop();
	schemeIndex = baseSchemeIndex;
	isSpecial = schemeIndex >= 0;
	if (schemeIndex == 0)
		goto FILE_START;

    if (fx_parseURLHasOpaquePath(the, base)) {
		c = fx_parseURLDecode(the, src);
   		if (c == '#') {
			mxPushSlot(base);
			mxGetID(xsID_path);
 			mxPushSlot(result);
 			mxSetID(xsID_path);
 			mxPop();
			mxPushSlot(base);
			mxGetID(xsID_query);
 			mxPushSlot(result);
 			mxSetID(xsID_query);
 			mxPop();
			goto FRAGMENT;
  		}
  		goto ERROR;
    }
   	goto RELATIVE;

RELATIVE:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (isSpecial && (c == '\\'))) {
		c = fx_parseURLDecode(the, src);
		if ((c == '/') || (isSpecial && (c == '\\'))) {
			goto AUTHORITY_START;
		}
	}
	src->offset = tmp;
	
	mxPushSlot(base);
	mxGetID(xsID_username);
	mxPushSlot(result);
	mxSetID(xsID_username);
	mxPop();
	mxPushSlot(base);
	mxGetID(xsID_password);
	mxPushSlot(result);
	mxSetID(xsID_password);
	mxPop();
	mxPushSlot(base);
	mxGetID(xsID_host);
	mxPushSlot(result);
	mxSetID(xsID_host);
	mxPop();
	mxPushSlot(base);
	mxGetID(xsID_port);
	mxPushSlot(result);
	mxSetID(xsID_port);
	mxPop();

	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (isSpecial && (c == '\\'))) {
		goto PATH_START;
	}
	
	fx_parseURLCopyPath(the, base, result);
	
	if (c == '?') {
		goto QUERY;
	}
	if (c == '#') {
		goto FRAGMENT;
	}
	if (c != C_EOF) {
		fx_parseURLShortenPath(the, result);
		src->offset = tmp;
		goto PATH;
	}
	goto END;
	
AUTHORITY_START:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (isSpecial && (c == '\\')))
		goto AUTHORITY_START;
	src->offset = tmp;

AUTHORITY:
	c = fx_parseURLDecode(the, src);
	if ((c == C_EOF) || (c == '/') || (isSpecial && (c == '\\')) || (c == '?') || (c == '#')) {
		src->offset = tmp;
		goto HOST;	
	}
	if (c == '@') {
		src->offset = tmp;
		goto USERNAME;	
	}
	goto AUTHORITY;
	
USERNAME:
	c = fx_parseURLDecode(the, src);
	if (override && (c == C_EOF)) {
		fx_parseURLSetPart(the, result, xsID_username, 0, dst);
		goto END;
	}
	if (!override && (c == ':')) {
		fx_parseURLSetPart(the, result, xsID_username, 0, dst);
		goto PASSWORD;
	}
	if (!override && (c == '@')) {
		fx_parseURLSetPart(the, result, xsID_username, 0, dst);
		goto HOST;
	}
	if (fx_parseURLEncode(the, c, src, dst, gxUserInfoSet))
		goto USERNAME;
	goto ERROR;	
	
PASSWORD:
	c = fx_parseURLDecode(the, src);
	if (override && (c == C_EOF)) {
		fx_parseURLSetPart(the, result, xsID_password, 0, dst);
		goto END;
	}
	if (!override && (c == '@')) {
		fx_parseURLSetPart(the, result, xsID_password, 0, dst);
		goto HOST;
	}
	if (fx_parseURLEncode(the, c, src, dst, gxUserInfoSet))
		goto PASSWORD;
	goto ERROR;	
	
HOST:
	c = fx_parseURLDecode(the, src);
	if (('A' <= c) && (c <= 'Z')) {
		fx_parseURLEncode(the, c - ('A' - 'a'), src, dst, gxEmptySet);
		goto HOST;
	}
	if ((('a' <= c) && (c <= 'z')) || (c == '-') || (c == '.')) {
		fx_parseURLEncode(the, c, src, dst, gxEmptySet);
		goto HOST;
	}
	if (c == ':') {
		if (isSpecial && (dst->offset == 0))
			goto ERROR;
		fx_parseURLSetPart(the, result, xsID_host, 0, dst);
		if (override == HOSTNAME_STATE)
			goto END;
		goto PORT;
	}
	if (override || (c == C_EOF) || (c == '/') || (isSpecial && (c == '\\')) || (c == '?') || (c == '#')) {
		if (isSpecial && (dst->offset == 0))
			goto ERROR;
		fx_parseURLSetPart(the, result, xsID_host, 0, dst);
		if (override)
			goto END;
		if ((c == C_EOF) || (c == '/') || (isSpecial && (c == '\\')) || (c == '?') || (c == '#'))
			goto PATH_START;
	}
	goto ERROR;
	
PORT:
	c = fx_parseURLDecode(the, src);
	if (('0' <= c) && (c <= '9')) {
		fx_parseURLEncode(the, c, src, dst, gxEmptySet);
		goto PORT;
	}
	if (override || (c == C_EOF) || (c == '/') || (isSpecial && (c == '\\')) || (c == '?') || (c == '#')) {
		if (dst->offset != 0) {
			fx_parseURLSetPart(the, result, xsID_port, 0, dst);
		}
		if (override)
			goto END;
		if ((c == C_EOF) || (c == '/') || (isSpecial && (c == '\\')) || (c == '?') || (c == '#'))
			goto PATH_START;
	}
	goto ERROR;	

FILE_START:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (c == '\\'))
		goto FILE_SLASH;
	src->offset = tmp;
	if (base && (baseSchemeIndex == 0)) {
		mxPushSlot(base);
		mxGetID(xsID_host);
		mxPushSlot(result);
		mxSetID(xsID_host);
		mxPop();
		
		if (fx_parseURLIsWindowsDriveLetter(the, src->slot->value.string + src->offset, 1))
			goto PATH_START;

		fx_parseURLCopyPath(the, base, result);

		if (c == '?')
			goto QUERY;
		if (c == '#')
			goto FRAGMENT;
			
		if (c != C_EOF) {
			fx_parseURLShortenPath(the, result);
			goto PATH;
		}
	}
	mxPush(mxEmptyString);
	mxPushSlot(result);
	mxSetID(xsID_host);
	goto PATH_START;
	
FILE_SLASH:
	tmp = src->offset;
	c = fx_parseURLDecode(the, src);
	if ((c == '/') || (c == '\\'))
		goto FILE_HOST;
	src->offset = tmp;
	if (base && (baseSchemeIndex == 0)) {
		mxPushSlot(base);
		mxGetID(xsID_host);
		mxPushSlot(result);
		mxSetID(xsID_host);
		mxPop();
		
		if (fx_parseURLIsWindowsDriveLetter(the, src->slot->value.string + src->offset, 1))
			goto PATH_START;
			
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
				goto PATH;
			}
		}
		goto PATH_START;
	}
	mxPush(mxEmptyString);
	mxPushSlot(result);
	mxSetID(xsID_host);
	goto PATH_START;

FILE_HOST:
	c = fx_parseURLDecode(the, src);
	if ((c == C_EOF) || (c == '/') || (c == '\\') || (c == '?') || (c == '#')) {
		if (!override && fx_parseURLIsWindowsDriveLetter(the, dst->slot->value.string, 0)) {
			mxPush(mxEmptyString);
			mxPushSlot(result);
			mxSetID(xsID_host);
			fx_parseURLPushPath(the,result, dst, 0);
			goto PATH;
		}
		if (!c_strcmp(dst->slot->value.string, "localhost")) {
			dst->slot->value.string[0] = 0;
			dst->offset = 0;
		}
		fx_parseURLSetPart(the, result, xsID_host, 0, dst);
		if (override)
			goto END;
		goto PATH_START;
	}
	if (fx_parseURLEncode(the, c, src, dst, gxEmptySet))
		goto FILE_HOST;
	goto ERROR;
	
PATH_START:
	if (c == C_EOF)
		goto END;
	if (!override && (c == '?'))
		goto QUERY;
	if (!override && (c == '#'))
		goto FRAGMENT;
	
PATH:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLPushPath(the, result, dst, 1);
		goto END;
	}
	if ((c == '/') || (isSpecial && (c == '\\'))) {
		fx_parseURLPushPath(the, result, dst, 0);
		goto PATH;
	}
	if (!override && (c == '?')) {
		fx_parseURLPushPath(the, result, dst, 1);
		goto QUERY;
	}
	if (!override && (c == '#')) {
		fx_parseURLPushPath(the, result, dst, 1);
		goto FRAGMENT;
	}
	if (fx_parseURLEncode(the, c, src, dst, gxPathSet))
		goto PATH;
	goto ERROR;	
	
OPAQUE_PATH:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_path, 0, dst);
		goto END;
	}
	if (!override && (c == '?')) {
		fx_parseURLSetPart(the, result, xsID_path, 0, dst);
		goto QUERY;
	}
	if (!override && (c == '#')) {
		fx_parseURLSetPart(the, result, xsID_path, 0, dst);
		goto FRAGMENT;
	}
	if (fx_parseURLEncode(the, c, src, dst, gxEmptySet))
		goto OPAQUE_PATH;
	goto ERROR;	
	
QUERY:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_query, 0, dst);
		goto END;
	}
	if (!override && (c == '#')) {
		fx_parseURLSetPart(the, result, xsID_query, 0, dst);
		goto FRAGMENT;
	}
	if (fx_parseURLEncode(the, c, src, dst, gxQuerySet))
		goto QUERY;
	goto ERROR;
	
FRAGMENT:
	c = fx_parseURLDecode(the, src);
	if (c == C_EOF) {
		fx_parseURLSetPart(the, result, xsID_fragment, 0, dst);
		goto END;
	}
	if (fx_parseURLEncode(the, c, src, dst, gxFragmentSet))
		goto FRAGMENT;
	goto ERROR;
		
ERROR:
	mxURIError("invalid URL");
	
END:
	return;
}

txBoolean fx_parseURLCanHaveUserInfoPort(txMachine* the, txSlot* parts)
{
	txBoolean result;
	mxPushSlot(parts);
	mxGetID(xsID_scheme);
	result = c_strcmp("file", the->stack->value.string) == 0;
	mxPop();
	if (result) {
		mxPushSlot(parts);
		mxGetID(xsID_host);
		result = the->stack->value.string[0] != 0;
		mxPop();
	}
	return result;
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
	stream->offset = string - stream->slot->value.string;
	return result;
}

txBoolean fx_parseURLEncode(txMachine* the, txInteger c, txStringStream* src, txStringStream* dst, const char* set)
{
	txSize length;
	txString string;
	if (c == '%') {
		string = src->slot->value.string + src->offset;
		if (!fxParseHexEscape(&string, &c))
			return 0;
		src->offset = string - src->slot->value.string;
	}
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
	return 1;
}

txBoolean fx_parseURLHasOpaquePath(txMachine* the, txSlot* parts)
{
	mxPushSlot(parts);
	return mxHasID(xsID_path);
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
			if (startsWith && ((c == '/') || (c == '?') || (c == '#')))
				return 1;
		}
	}
	return 0;
}

void fx_parseURLPushPath(txMachine* the, txSlot* path, txStringStream* dst, txBoolean flag) 
{
	txSize size;
	txString string;
	if (c_strcmp(dst->slot->value.string, "..") == 0) {
		fx_parseURLShortenPath(the, path);
		if (flag) {
			dst->slot->value.string[0] = 0;
			dst->offset = 0;
		}
	}
	else if (c_strcmp(dst->slot->value.string, ".") == 0) {
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
	txInteger override = 0;
	txSlot* result;
	txIndex length;
	
    fxVars(the, 3);
	*mxResult = mxEmptyString;
	result = mxResult;
	
	parts = mxArgv(0);
	if (mxArgc > 1) {
   		override = fxToInteger(the, mxArgv(1));
    	if (override == SCHEME_STATE) {
			goto SCHEME;
    	}
    	if (override == USERNAME_STATE) {
			goto USERNAME;
    	}
    	if (override == PASSWORD_STATE) {
			goto PASSWORD;
		}
    	if (override == HOST_STATE) {
			goto HOST;
     	}
   		if (override == HOSTNAME_STATE) {
			goto HOST;
    	}
    	if (override == PORT_STATE) {
     		goto PORT;
    	}
    	if (override == PATH_STATE) {
  			goto PATH;
    	}
    	if (override == QUERY_STATE) {
   			goto QUERY;
    	}
    	if (override == FRAGMENT_STATE) {
   			goto FRAGMENT;
    	}
    	goto END;
	}
	
SCHEME:
	mxPushSlot(parts);
	mxGetID(xsID_scheme);
	fxConcatString(the, result, the->stack);
	mxPop();
	fxConcatStringC(the, result, ":");
	if (override == SCHEME_STATE)
		goto END;

	mxPushSlot(parts);
	mxGetID(xsID_host);
	mxPullSlot(mxVarv(0));
	if (mxIsNull(mxVarv(0)))
		goto PATH;
		
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
	mxPushSlot(parts);
	mxGetID(xsID_port);
	mxPullSlot(mxVarv(0));
	if (mxIsNull(mxVarv(0)))
		goto PATH;
	fxConcatStringC(the, result, ":");
	fxConcatString(the, result, mxVarv(0));
	goto PATH;
	
USERNAME:
	mxPushSlot(parts);
	mxGetID(xsID_username);
	fxConcatString(the, result, the->stack);
	mxPop();
	goto END;
	
PASSWORD:
	mxPushSlot(parts);
	mxGetID(xsID_password);
	fxConcatString(the, result, the->stack);
	mxPop();
	goto END;

HOST:
	mxPushSlot(parts);
	mxGetID(xsID_host);
	mxPullSlot(mxVarv(0));
	if (mxIsNull(mxVarv(0)))
		goto END;
	fxConcatString(the, result, mxVarv(0));
	if (override == HOSTNAME_STATE)
		goto END;

PORT:
	mxPushSlot(parts);
	mxGetID(xsID_port);
	mxPullSlot(mxVarv(0));
	if (mxIsNull(mxVarv(0)))
		goto END;
	fxConcatString(the, result, mxVarv(0));
	goto END;
	
PATH:
	mxPushSlot(parts);
	if (mxHasID(xsID_path)) {
		mxPushSlot(parts);
		mxGetID(xsID_path);
		fxConcatString(the, result, the->stack);
		mxPop();
	}
	else {
		mxPushSlot(parts);
		mxGetID(mxID(_length));
		length = (txIndex)fxToLength(the, the->stack);
		mxPop();
		if (length == 0)
			fxConcatStringC(the, result, "/");
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
		goto END;

QUERY:
	mxPushSlot(parts);
	mxGetID(xsID_query);
	if (the->stack->value.string[0]) {
		fxConcatStringC(the, result, "?");
		fxConcatString(the, result, the->stack);
	}
	mxPop();
	if (override == QUERY_STATE)
		goto END;
		
FRAGMENT:
	mxPushSlot(parts);
	mxGetID(xsID_fragment);
	if (the->stack->value.string[0]) {
		fxConcatStringC(the, result, "#");
		fxConcatString(the, result, the->stack);
	}
		
END:
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


