/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

/*
	ECMA-419 Crypto MAC — PSA Crypto: HMAC (RFC 2104) composed over psa_hash, whose operations can be cloned, so
	read() is non-destructive as on the KCL backend (psa_mac operations cannot be cloned)
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "digest-psa.h"

typedef struct {
	psa_hash_operation_t running;		// the inner hash of the message so far
	psa_hash_operation_t inner;			// the inner hash after the key pad: the reset state
	psa_hash_operation_t outer;			// the outer hash after the key pad
	psa_algorithm_t alg;
	uint16_t outputSize;
	uint16_t blockSize;
	uint8_t hashIndex;
} MACRecord, *MAC;

static void psaCheck(xsMachine *the, psa_status_t status, const char *what)
{
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("%s failed %d", what, (int)status);
	}
}

static void macFinish(xsMachine *the, MAC mac, uint8_t *out)
{
	psa_hash_operation_t op = PSA_HASH_OPERATION_INIT;
	uint8_t inner[64];
	size_t actual;
	psa_status_t status;

	status = psa_hash_clone(&mac->running, &op);
	if (PSA_SUCCESS == status)
		status = psa_hash_finish(&op, inner, sizeof(inner), &actual);
	if (PSA_SUCCESS == status)
		status = psa_hash_clone(&mac->outer, &op);
	if (PSA_SUCCESS == status)
		status = psa_hash_update(&op, inner, mac->outputSize);
	if (PSA_SUCCESS == status)
		status = psa_hash_finish(&op, out, mac->outputSize, &actual);
	psa_hash_abort(&op);
	c_memset(inner, 0, sizeof(inner));
	psaCheck(the, status, "psa_hash");
}

void xs_mac_destructor(void *data)
{
	MAC mac = data;
	if (mac) {
		psa_hash_abort(&mac->running);
		psa_hash_abort(&mac->inner);
		psa_hash_abort(&mac->outer);
	}
}

void xs_mac_constructor(xsMachine *the)
{
	xsmcVars(1);
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_algorithm))
		xsUnknownError("algorithm required");
	if (0 != c_strcmp(xsmcToString(xsVar(0)), "HMAC"))
		xsUnknownError("unsupported algorithm");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_hash))
		xsUnknownError("hash required");
	const DigestAlgEntry *entry = xs_crypt_digestAlgorithm(xsmcToString(xsVar(0)));
	if (!entry)
		xsRangeError("unsupported hash");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_key))
		xsUnknownError("key required");
	void *key;
	xsUnsignedValue keyLen;
	xsmcGetBufferReadable(xsVar(0), &key, &keyLen);

	MACRecord init;
	c_memset(&init, 0, sizeof(init));
	init.running = (psa_hash_operation_t)PSA_HASH_OPERATION_INIT;
	init.inner = (psa_hash_operation_t)PSA_HASH_OPERATION_INIT;
	init.outer = (psa_hash_operation_t)PSA_HASH_OPERATION_INIT;
	init.alg = entry->alg;
	init.outputSize = entry->outputSize;
	init.blockSize = entry->blockSize;
	init.hashIndex = (uint8_t)(entry - xs_crypt_digestAlgorithms);

	psa_status_t status = psa_crypto_init();
	psaCheck(the, status, "psa_crypto_init");

	uint8_t block[128], hashed[64];
	size_t actual;
	if (keyLen > entry->blockSize) {		// a key longer than the block is replaced by its hash
		status = psa_hash_compute(entry->alg, key, keyLen, hashed, sizeof(hashed), &actual);
		psaCheck(the, status, "psa_hash_compute");
		key = hashed;
		keyLen = entry->outputSize;
	}
	for (int pass = 0; (pass < 2) && (PSA_SUCCESS == status); pass++) {
		psa_hash_operation_t *op = pass ? &init.outer : &init.inner;
		uint8_t pad = pass ? 0x5c : 0x36;
		for (uint32_t i = 0; i < entry->blockSize; i++)
			block[i] = ((i < keyLen) ? ((uint8_t *)key)[i] : 0) ^ pad;
		status = psa_hash_setup(op, entry->alg);
		if (PSA_SUCCESS == status)
			status = psa_hash_update(op, block, entry->blockSize);
	}
	if (PSA_SUCCESS == status)
		status = psa_hash_clone(&init.inner, &init.running);
	c_memset(block, 0, sizeof(block));
	c_memset(hashed, 0, sizeof(hashed));
	if (PSA_SUCCESS != status) {
		xs_mac_destructor(&init);
		psaCheck(the, status, "psa_hash_setup");
	}

	xsmcSetHostChunk(xsThis, &init, sizeof(init));
}

void xs_mac_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xs_mac_destructor(mac);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_mac_write(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	void *data;
	xsUnsignedValue size;

	xsmcGetBufferReadable(xsArg(0), &data, &size);
	mac = xsmcGetHostChunk(xsThis);
	psaCheck(the, psa_hash_update(&mac->running, data, size), "psa_hash_update");
}

void xs_mac_read(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	uint8_t *out;

	if (xsmcArgc > 0) {
		xsUnsignedValue outLen;
		xsmcGetBufferWritable(xsArg(0), (void **)&out, &outLen);
		if (outLen < mac->outputSize)
			xsRangeError("buffer too small");
		xsmcSetInteger(xsResult, mac->outputSize);
	}
	else
		out = xsmcSetArrayBuffer(xsResult, NULL, mac->outputSize);

	mac = xsmcGetHostChunk(xsThis);
	macFinish(the, mac, out);
}

void xs_mac_reset(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	psa_hash_abort(&mac->running);
	psaCheck(the, psa_hash_clone(&mac->inner, &mac->running), "psa_hash_clone");
}

void xs_mac_verify(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	void *tag;
	xsUnsignedValue tagLen;
	uint8_t expected[64];
	uint8_t diff = 0;

	xsmcGetBufferReadable(xsArg(0), &tag, &tagLen);
	mac = xsmcGetHostChunk(xsThis);
	macFinish(the, mac, expected);
	if (tagLen == mac->outputSize) {		// constant time over the tag
		for (uint32_t i = 0; i < mac->outputSize; i++)
			diff |= expected[i] ^ ((uint8_t *)tag)[i];
	}
	else
		diff = 1;
	c_memset(expected, 0, sizeof(expected));
	xsmcSetBoolean(xsResult, 0 == diff);
}

void xs_mac_get_hash(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetString(xsResult, (char *)xs_crypt_digestAlgorithms[mac->hashIndex].name);
}

void xs_mac_get_blockSize(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetInteger(xsResult, mac->blockSize);
}

void xs_mac_get_outputSize(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetInteger(xsResult, mac->outputSize);
}

void xs_mac_supports(xsMachine *the)
{
	xsmcVars(1);
	int supported = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_algorithm) && (xsStringType == xsmcTypeOf(xsVar(0))) && (0 == c_strcmp(xsmcToString(xsVar(0)), "HMAC"))) {
		supported = 1;
		if (xsmcGet(xsVar(0), xsArg(0), xsID_hash)) {
			const DigestAlgEntry *entry = (xsStringType == xsmcTypeOf(xsVar(0))) ? xs_crypt_digestAlgorithm(xsmcToString(xsVar(0))) : NULL;
			supported = NULL != entry;
		}
	}
	xsmcSetBoolean(xsResult, supported);
}
