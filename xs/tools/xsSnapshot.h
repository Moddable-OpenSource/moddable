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
	char* signature;
	int signatureLength;
	txCallback* callbacks;
	int callbacksLength;
	int (*read)(void* stream, void* address, size_t size);
	int (*write)(void* stream, void* address, size_t size);
	void* stream;
	int error;
	txByte* firstChunk;
	txProjection* firstProjection;
	txSlot* firstSlot;
};

extern txMachine* fxReadSnapshot(txSnapshot* snapshot, txString theName, void* theContext);
extern int fxWriteSnapshot(txMachine* the, txSnapshot* snapshot);
