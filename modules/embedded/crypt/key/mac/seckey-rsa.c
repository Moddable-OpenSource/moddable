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

#include "xsPlatform.h"
#include "seckey-rsa.h"

static const char *gHashNames[] = { "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512", NULL };
const uint16_t xs_crypt_rsaHashSizes[] = { 20, 28, 32, 48, 64 };

int xs_crypt_rsaHashIndex(const char *name)
{
	for (int i = 0; gHashNames[i]; i++) {
		if (0 == c_strcmp(name, gHashNames[i]))
			return i;
	}
	return -1;
}

SecKeyRef xs_crypt_rsaKey(const void *der, long derLength, int isPrivate, long bits)
{
	CFDataRef data = CFDataCreate(NULL, der, derLength);
	CFNumberRef size = CFNumberCreate(NULL, kCFNumberLongType, &bits);
	const void *keys[] = { kSecAttrKeyType, kSecAttrKeyClass, kSecAttrKeySizeInBits };
	const void *values[] = { kSecAttrKeyTypeRSA, isPrivate ? kSecAttrKeyClassPrivate : kSecAttrKeyClassPublic, size };
	CFDictionaryRef attributes = CFDictionaryCreate(NULL, keys, values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFErrorRef error = NULL;
	SecKeyRef key = (data && size && attributes) ? SecKeyCreateWithData(data, attributes, &error) : NULL;
	if (error)
		CFRelease(error);
	if (attributes)
		CFRelease(attributes);
	if (size)
		CFRelease(size);
	if (data)
		CFRelease(data);
	return key;
}
