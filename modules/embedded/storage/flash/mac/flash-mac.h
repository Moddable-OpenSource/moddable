struct xsFlashRecord {
	xsIntegerValue size;
	xsIntegerValue blocks;
	xsIntegerValue blockLength;
	xsBooleanValue readOnly;
	int fd;
	uint8_t* bytes;
};
typedef struct xsFlashRecord xsFlashRecord;
typedef struct xsFlashRecord *xsFlash;
