/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

static int
_getBerLen(unsigned char **pp, unsigned char *endp)
{
	unsigned char *p = *pp;
	int l;

	if (p >= endp)
		return -1;
	if ((l = c_read8(p++)) > 0x80) {
		int n = l - 0x80;
		for (l = 0; --n >= 0;) {
			if (p >= endp)
				return -1;
			l = (l << 8) | c_read8(p++);
		}
	}
	*pp = p;
	return l;
}

static int
oid_encode(unsigned char *enc, unsigned int *in, int insize)
{
	unsigned char *out = enc;
	unsigned int x;
	int i, n;

	if (insize <= 0)
		return 0;
	*out++ = in[0] * 40 + (insize > 1 ? in[1] : 0);
	for (i = 2; i < insize; i++) {
		x = in[i];
		for (n = 1; x >>= 7; n++)
			;
		x = in[i];
		while (--n >= 1)
			*out++ = (x >> (n * 7)) | 0x80;
		*out++ = x & 0x7f;
	}
	return out - enc;
}

static int
x509_decode(unsigned char **pp, size_t *szp, unsigned int *extid, size_t extidlen, int spki)
{
	size_t sz = *szp;
	unsigned char *p = *pp, *endp = p + sz, *endTBS, *endEXT, *extnID;
	int l, extnIDLen, oidLen;
	unsigned char oid[32];
#define getTag()	(p < endp ? (int)c_read8(p++) : -1)
#define getBerLen()	_getBerLen(&p, endp)
#define nextTag()	(getTag(), l = getBerLen(), p += l)

	if (getTag() != 0x30)
		return 0;
	if ((l = getBerLen()) < 0)
		return 0;
	if (p + l > endp)
		return 0;
	/* TBSCertficate */
	if (getTag() != 0x30)
		return 0;
	if ((l = getBerLen()) < 0)
		return 0;
	if ((endTBS = p + l) > endp)
		return 0;
	if (c_read8(p) & 0x80) {
		/* EXPLICT Version */
		p++;
		nextTag();
	}
	nextTag();	/* serialNumber */
	nextTag();	/* signature */
	nextTag();	/* issuer */
	nextTag();	/* validity */
	nextTag();	/* subject */
	/* subjectpublicKeyInfo */
	if (spki) {
		*pp = p;
		getTag();
		l = getBerLen();
		p += l;
		*szp = p - *pp;
	}
	else
		nextTag();
	if (extid == NULL)
		return 1;
	/* OPTIONAL */
	oidLen = oid_encode(oid, extid, extidlen);
	while (p < endTBS) {
		int tag = getTag();
		if ((l = getBerLen()) < 0)
			return 0;
		switch (tag & 0x1f) {
		case 1:	/* issuerUniqueID */
		case 2:	/* subjectUniqueID */
			p += l;
			continue;	/* goto the next tag */
		case 3:	/* extensions */
			break;	/* fall thru */
		default:
			return 0;
		}
		/* must be a SEQUENCE of [1..MAX] */
		if (getTag() != 0x30)
			return 0;
		if ((l = getBerLen()) < 0)
			return 0;
		endEXT = p + l;
		while (p < endEXT) {
			/* must be a SEQUENCE of {extnID, critical, extnValue} */
			if (getTag() != 0x30)
				return 0;
			if ((l = getBerLen()) < 0)
				return 0;
			/* extnID: OBJECT ID */
			if (getTag() != 0x06)
				return 0;
			if ((extnIDLen = getBerLen()) < 0)
				return 0;
			extnID = p;
			p += extnIDLen;
			/* critical: BOOLEAN */
			if (c_read8(p) == 0x01)
				nextTag();
			/* extnValue: OCTET STRING */
			if (getTag() != 0x04)
				return 0;
			if ((l = getBerLen()) < 0)
				return 0;
			if (extnIDLen == oidLen && c_memcmp(extnID, oid, extnIDLen) == 0) {
				*pp = p;
				getTag();
				l = getBerLen();
				p += l;
				*szp = p - *pp;
				return 1;
			}
			p += l;
		}
	}
	return 0;
}

void
xs_x509_decodeExtension(xsMachine *the)
{
	unsigned char *p, *savep;
	size_t sz;
	xsUnsignedValue count;
	int idlen;
	unsigned int extid[32];
	int i;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(1), xsID_length);
	idlen = xsmcToInteger(xsVar(0));
	if (idlen > (int)sizeof(extid))
		xsUnknownError("too long");

	for (i = 0; i < idlen; i++) {
		xsmcGetIndex(xsVar(0), xsArg(1), i);
		extid[i] = xsmcToInteger(xsVar(0));
	}

	xsmcGetBufferReadable(xsArg(0), (void **)&p, &count);
	sz = count;
	savep = p;
	if (x509_decode(&p, &sz, extid, idlen, 0)) {
		int offset = p - savep;
		xsmcSetArrayBuffer(xsResult, NULL, sz);
		xsmcGetBufferReadable(xsArg(0), (void **)&p, &count);
		c_memcpy(xsmcToArrayBuffer(xsResult), p + offset, sz);
	}
}

void
xs_x509_decodeSPKI(xsMachine *the)
{
	unsigned char *p, *savep;
	size_t sz;
	uint32_t count;

	xsmcGetBufferReadable(xsArg(0), (void **)&p, &count);
	sz = count;
	savep = p;

	if (x509_decode(&p, &sz, NULL, 0, 1)) {
		int offset = p - savep;
		xsmcSetArrayBuffer(xsResult, NULL, sz);
		xsmcGetBufferReadable(xsArg(0), (void **)&p, &count);
		c_memcpy(xsmcToArrayBuffer(xsResult), p + offset, sz);
	}
}
