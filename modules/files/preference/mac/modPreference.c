/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "xs.h"
#include "mc.defines.h"

void xs_preference_delete(xsMachine *the)
{
	char buffer[1024];
	c_strcpy(buffer, PIU_DOT_SIGNATURE);
	c_strcat(buffer, "."); 
	c_strcat(buffer, xsToString(xsArg(0))); 
	CFStringRef domain = CFStringCreateWithCString(NULL, buffer, kCFStringEncodingUTF8);
	CFStringRef key = CFStringCreateWithCString(NULL, xsToString(xsArg(1)), kCFStringEncodingUTF8);
	CFPreferencesSetAppValue(key, NULL, domain);
	CFPreferencesAppSynchronize(domain);
	CFRelease(key);
	CFRelease(domain);
}

void xs_preference_get(xsMachine *the)
{
	char buffer[1024];
	c_strcpy(buffer, PIU_DOT_SIGNATURE);
	c_strcat(buffer, "."); 
	c_strcat(buffer, xsToString(xsArg(0))); 
	CFStringRef domain = CFStringCreateWithCString(NULL, buffer, kCFStringEncodingUTF8);
	CFStringRef key = CFStringCreateWithCString(NULL, xsToString(xsArg(1)), kCFStringEncodingUTF8);
	CFTypeRef value = (CFTypeRef)CFPreferencesCopyAppValue(key, domain);
	if (value) {
		CFTypeID type = CFGetTypeID(value);
		if (type == CFBooleanGetTypeID()) {
			xsResult = xsBoolean(CFBooleanGetValue(value));
		}
		else if (type == CFNumberGetTypeID()) {
			if (CFNumberIsFloatType(value)) {
				xsNumberValue number;
				CFNumberGetValue(value, kCFNumberDoubleType, &number);
				xsResult = xsNumber(number);
			}
			else {
				xsIntegerValue integer;
				CFNumberGetValue(value, kCFNumberLongType, &integer);
				xsResult = xsInteger(integer);
			}
		}
		else if (type == CFStringGetTypeID()) {
			CFRange range = CFRangeMake(0, CFStringGetLength(value));
			CFIndex length;
			CFStringGetBytes(value, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &length);
			xsResult = xsStringBuffer(NULL, length);
			CFStringGetBytes(value, range, kCFStringEncodingUTF8, 0, false, (UInt8 *)xsToString(xsResult), length, NULL);
		}
		else
			xsResult = xsArrayBuffer((void*)CFDataGetBytePtr(value), CFDataGetLength(value));
		CFRelease(value);
	}
	CFRelease(key);
	CFRelease(domain);
}

void xs_preference_keys(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

void xs_preference_set(xsMachine *the)
{
	CFTypeRef value = NULL;
	char buffer[1024];
	char keyBuffer[64];
	c_strcpy(buffer, PIU_DOT_SIGNATURE);
	c_strcat(buffer, "."); 
	c_strcat(buffer, xsToString(xsArg(0))); 
	
	xsToStringBuffer(xsArg(1), keyBuffer, sizeof(keyBuffer));

	switch (xsTypeOf(xsArg(2))) {
	case xsBooleanType:
		value = (xsToBoolean(xsArg(2))) ? kCFBooleanTrue : kCFBooleanFalse;
		break;
	case xsIntegerType: {
		xsIntegerValue integer = xsToInteger(xsArg(2));
		value = CFNumberCreate(NULL, kCFNumberLongType, &integer);
		} break;
	case xsNumberType: {
		xsNumberValue number = xsToNumber(xsArg(2));
		xsIntegerValue integer = (int)number;
		if (number != integer)
			xsUnknownError("float unsupported");
		value = CFNumberCreate(NULL, kCFNumberLongType, &integer);
		} break;
	case xsStringType:
		value = CFStringCreateWithCString(NULL, xsToString(xsArg(2)), kCFStringEncodingUTF8);
		break;
	case xsReferenceType:
		if (xsIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
			value = CFDataCreate(NULL, xsToArrayBuffer(xsArg(2)), xsGetArrayBufferLength(xsArg(2)));
		break;
	}
	if (value) {
		CFStringRef domain = CFStringCreateWithCString(NULL, buffer, kCFStringEncodingUTF8);
		CFStringRef key = CFStringCreateWithCString(NULL, keyBuffer, kCFStringEncodingUTF8);
		CFPreferencesSetAppValue(key, value, domain);
		CFPreferencesAppSynchronize(domain);
		CFRelease(key);
		CFRelease(domain);
		CFRelease(value);
	}
	else
		xsUnknownError("unsupported type");
}

