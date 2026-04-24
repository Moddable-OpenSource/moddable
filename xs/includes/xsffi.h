#ifndef __XSALL__
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"

typedef int32_t txBoolean;
typedef uint8_t txFlag;
#ifdef mx32bitID
	typedef int32_t txID;
#else
	typedef int16_t txID;
#endif
typedef int32_t txInteger;
typedef double txNumber;
typedef char* txString;
typedef uint32_t txUnsigned;

typedef struct sxMachine txMachine;
typedef struct sxSlot txSlot;
typedef void (*txCallback)(txMachine* the);

struct sxSlot {
	void* data[4];
};
struct sxMachine {
	txSlot* stack;
	txSlot* scope;
	txSlot* frame;
};

#define mxThis XS->_this(the)
#define mxResult XS->result(the)
#define mxArgc XS->argc(the)
#define mxArgv(INDEX) XS->argv(the, (INDEX))

#endif

typedef struct sxAPI txAPI;
typedef void (*txBuildFFI)(txMachine* the, txAPI* api);

struct sxAPI {
	txSlot* (*_this)(txMachine* the);
	txInteger (*argc)(txMachine* the);
	txSlot* (*argv)(txMachine* the, txInteger index);
	void (*defineID)(txMachine* the, txID id, txFlag flag, txFlag mask);
	txID (*id)(txMachine* the, txString name);
	void (*integer)(txMachine* the, txSlot* slot, txInteger value);
	txSlot* (*newHostFunction)(txMachine* the, txCallback callback, txInteger length, txInteger name, txInteger profileID);
	void (*number)(txMachine* the, txSlot* slot, txNumber value);
	void (*pop)(txMachine* the);
	void (*push)(txMachine* the, txSlot* slot);
	txSlot* (*result)(txMachine* the);
	void (*string)(txMachine* the, txSlot* slot, char* value);
	void (*stringX)(txMachine* the, txSlot* slot, char* value);
	void** (*toArrayBufferHandle)(txMachine* the, txSlot* slot);
	txInteger (*toInteger)(txMachine* the, txSlot* slot);
	txNumber (*toNumber)(txMachine* the, txSlot* slot);
	char** (*toStringHandle)(txMachine* the, txSlot* slot);
	txUnsigned (*toUnsigned)(txMachine* the, txSlot* slot);
	void (*_unsigned)(txMachine* the, txSlot* slot, txUnsigned value);
};

extern txInteger fxArgc(txMachine*);

#if mxWindows
extern __declspec( dllexport ) void fxBuildFFI(txMachine* the, txAPI* api);
#else
extern void fxBuildFFI(txMachine* the, txAPI* api);
#endif
