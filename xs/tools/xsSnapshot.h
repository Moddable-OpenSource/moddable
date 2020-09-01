#include "xsAll.h"
#include "xsScript.h"

typedef struct sxProjection txProjection;
typedef struct sxSnapshot txSnapshot;

struct sxProjection {
	txProjection* nextProjection;
	txSlot* heap;
	txSlot* limit;
	size_t indexes[1];
};

struct sxSnapshot {
	txString signature;
	txSize signatureSize;
	txCallback* callbacks;
	txSize callbackCount;
	void* stream;
	int (*read)(void* stream, void* address, size_t size);
	int (*write)(void* stream, void* address, size_t size);
	int error;
	txByte* firstChunk;
	txProjection* firstProjection;
	txSlot* firstSlot;
};

extern txMachine* fxLoadMachine(txSnapshot* snapshot, txString theName, void* theContext);
extern int fxWriteSnapshot(txMachine* the, txSnapshot* snapshot);
