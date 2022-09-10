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

#include "piuPC.h"

#include <sys/event.h>

static void PiuFile_getFileInfoAux(xsMachine* the, NSURL* url);

void PiuSystem_get_applicationPath(xsMachine* the)
{
	NSString* path = [[NSBundle mainBundle] bundlePath];
	xsResult = xsString([path UTF8String]);
}

void PiuSystem_get_localDirectory(xsMachine* the)
{
	NSFileManager* manager = [NSFileManager defaultManager];
	NSURL* url = [manager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nil];
	url = [url URLByAppendingPathComponent:@PIU_DOT_SIGNATURE isDirectory:YES];
	[manager createDirectoryAtURL:url withIntermediateDirectories:NO attributes:nil error:nil];
	xsResult = xsString([url fileSystemRepresentation]);
}

void PiuSystem_get_platform(xsMachine* the)
{
	xsResult = xsString("mac");
}

void PiuSystem_buildPath(xsMachine* the)
{
 	xsIntegerValue argc = xsToInteger(xsArgc);
	NSString* directory = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* name = [NSString stringWithUTF8String:xsToString(xsArg(1))];
	NSString* path = [directory stringByAppendingPathComponent:name];
	if ((argc > 2) && xsTest(xsArg(2))) {
		NSString* extension = [NSString stringWithUTF8String:xsToString(xsArg(2))];
		path = [path stringByAppendingPathExtension:extension];
	}
	xsResult = xsString([path UTF8String]);
}

void PiuSystem_copyFile(xsMachine* the)
{
	NSError* error = nil;
	NSString* from = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* to = [NSString stringWithUTF8String:xsToString(xsArg(1))];
	NSFileManager* manager = [NSFileManager defaultManager];
	[manager removeItemAtPath:to error:&error];
	if ([manager copyItemAtPath:from toPath:to error:&error] == NO)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
}

void PiuSystem_deleteDirectory(xsMachine* the)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	if ([[NSFileManager defaultManager] removeItemAtPath:path error:&error] == NO)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
}

void PiuSystem_deleteFile(xsMachine* the)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	if ([[NSFileManager defaultManager] removeItemAtPath:path error:&error] == NO)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
}

void PiuSystem_ensureDirectory(xsMachine* the)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	if ([[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:&error] == NO)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
}

void PiuSystem_fileExists(xsMachine* the)
{
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	xsResult = ([[NSFileManager defaultManager] fileExistsAtPath:path]) ? xsTrue : xsFalse;
}

void PiuSystem_getFileInfo(xsMachine* the)
{
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
		NSURL* url = [NSURL fileURLWithPath:path];
		PiuFile_getFileInfoAux(the, url);
	}
}

void PiuFile_getFileInfoAux(xsMachine* the, NSURL* url)
{
	NSDate* date = nil;
	NSString* string = nil;
	NSNumber* number = nil;
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_path, xsString([url fileSystemRepresentation]), xsDefault);
	[url getResourceValue:&string forKey:NSURLLocalizedNameKey error:NULL];
    xsDefine(xsResult, xsID_name, xsString([string UTF8String]), xsDefault);
	[url getResourceValue:&date forKey:NSURLContentModificationDateKey error:NULL];
	xsDefine(xsResult, xsID_date, xsNumber(1000 * [date timeIntervalSince1970]), xsDefault);
	[url getResourceValue:&number forKey:NSURLIsDirectoryKey error:NULL];
	if ([number boolValue]) {
		xsDefine(xsResult, xsID_directory, xsTrue, xsDefault);
	}
	else {
		[url getResourceValue:&number forKey:NSURLIsSymbolicLinkKey error:NULL];
		if ([number boolValue]) {
			xsDefine(xsResult, xsID_symbolicLink, xsTrue, xsDefault);
		}
		[url getResourceValue:&number forKey:NSURLFileSizeKey error:NULL];
		xsDefine(xsResult, xsID_size, xsInteger([number intValue]), xsDefault);
	}
}

void PiuSystem_getPathDirectory(xsMachine* the)
{
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* directory = [path stringByDeletingLastPathComponent];
	xsResult = xsString([directory UTF8String]);
}

void PiuSystem_getPathExtension(xsMachine* the)
{
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* extension = [path pathExtension];
	xsResult = xsString([extension UTF8String]);
}

void PiuSystem_getPathName(xsMachine* the)
{
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* name =  [path lastPathComponent];
	xsResult = xsString([name UTF8String]);
}

void PiuSystem_getSymbolicLinkInfo(xsMachine* the)
{
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
		NSURL* url = [NSURL fileURLWithPath:path];
		PiuFile_getFileInfoAux(the, [url URLByResolvingSymlinksInPath]);
	}
}

void PiuSystem_readFileBuffer(xsMachine* the)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSData* data = [NSData dataWithContentsOfFile:path options:NSDataReadingMappedAlways error:&error];
	if (!data)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
	xsResult = xsArrayBuffer((xsStringValue)[data bytes], (xsIntegerValue)[data length]);
}

void PiuSystem_readFileString(xsMachine* the)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSData* data = [NSData dataWithContentsOfFile:path options:NSDataReadingMappedAlways error:&error];
	if (!data)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
	xsResult = xsStringBuffer((xsStringValue)[data bytes], (xsIntegerValue)[data length]);
}

void PiuSystem_readPreferenceString(xsMachine* the)
{
	NSString* key = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* value =  [[NSUserDefaults standardUserDefaults] stringForKey:key];
	if (value)
		xsResult = xsString([value UTF8String]);
}

void PiuSystem_renameDirectory(xsMachine* the)
{
	NSError* error = nil;
	NSString* from = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* directory = [from stringByDeletingLastPathComponent];
	NSString* to = [directory stringByAppendingPathComponent:[NSString stringWithUTF8String:xsToString(xsArg(1))]];
	if ([[NSFileManager defaultManager] moveItemAtPath:from toPath:to error:&error] == NO)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
}

void PiuSystem_renameFile(xsMachine* the)
{
	PiuSystem_renameDirectory(the);
}

void PiuSystem_writeFileBuffer(xsMachine* the)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	void* buffer = xsToArrayBuffer(xsArg(1));
	NSUInteger length = xsGetArrayBufferLength(xsArg(1));
	NSData* data = [NSData dataWithBytesNoCopy:buffer length:(NSUInteger)length freeWhenDone:NO];
	if ([data writeToFile:path options:NSDataWritingAtomic error:&error] == NO)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
}

void PiuSystem_writeFileString(xsMachine* the)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	void* buffer = xsToString(xsArg(1));
	NSUInteger length = c_strlen(buffer);
	NSData* data = [NSData dataWithBytesNoCopy:buffer length:length freeWhenDone:NO];
	if ([data writeToFile:path options:NSDataWritingAtomic error:&error] == NO)
		xsUnknownError("%s", [[error localizedDescription] UTF8String]);
}


void PiuSystem_writePreferenceString(xsMachine* the)
{
	NSString* key = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSString* value = [NSString stringWithUTF8String:xsToString(xsArg(1))];
	[[NSUserDefaults standardUserDefaults] setObject:value forKey:key];
}

void PiuSystem_DirectoryIteratorCreate(xsMachine* the)
{
	NSString* path = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	NSURL* url = [NSURL fileURLWithPath:path];
	NSArray *keys = [NSArray arrayWithObjects: NSURLContentModificationDateKey, NSURLFileSizeKey, NSURLIsDirectoryKey, NSURLLocalizedNameKey, NSURLIsSymbolicLinkKey, nil];
	NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager] enumeratorAtURL:url includingPropertiesForKeys:keys
		options:(NSDirectoryEnumerationSkipsSubdirectoryDescendants | NSDirectoryEnumerationSkipsHiddenFiles) errorHandler:NULL];
	xsSetHostData(xsThis, enumerator);
	[enumerator retain];
}

void PiuSystem_DirectoryIteratorDelete(void* it)
{
	NSDirectoryEnumerator *enumerator = it;
	[enumerator release];
}

void PiuSystem_DirectoryIterator_next(xsMachine* the)
{
	NSDirectoryEnumerator *enumerator = xsGetHostData(xsThis);
	NSURL* url = [enumerator nextObject];
	if (url)
		PiuFile_getFileInfoAux(the, url);
}

typedef struct PiuDirectoryNotifierStruct PiuDirectoryNotifierRecord, *PiuDirectoryNotifier;

struct PiuDirectoryNotifierStruct {
	PiuHandlePart;
	xsMachine* the;
	xsSlot* path;
	xsSlot* callback;
	FSEventStreamRef stream;
};

static void PiuSystem_DirectoryNotifierCallback(ConstFSEventStreamRef streamRef, void *userData, size_t c,
		void *paths,  const FSEventStreamEventFlags flags[], const FSEventStreamEventId ids[]);
static void PiuSystem_DirectoryNotifierMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuDirectoryNotifierHooks ICACHE_RODATA_ATTR = {
	PiuSystem_DirectoryNotifierDelete,
	PiuSystem_DirectoryNotifierMark,
	NULL
};

void PiuSystem_DirectoryNotifierCallback(ConstFSEventStreamRef stream, void *it, size_t c,
		void *paths, const FSEventStreamEventFlags flags[], const FSEventStreamEventId ids[])
{
	PiuDirectoryNotifier* self = it;
	xsBeginHost((*self)->the);
	xsVars(2);
	xsCallFunction1(xsReference((*self)->callback), xsReference((*self)->reference), *((*self)->path));
	xsEndHost((*self)->the);
}

void PiuSystem_DirectoryNotifierCreate(xsMachine* the)
{
	PiuDirectoryNotifier* self;
	
	xsSetHostChunk(xsThis, NULL, sizeof(PiuDirectoryNotifierRecord));
	self = PIU(DirectoryNotifier, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	(*self)->path = PiuString(xsArg(0));
	(*self)->callback = xsToReference(xsArg(1));;
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuDirectoryNotifierHooks);
	
    FSEventStreamContext context = {0, NULL, NULL, NULL, NULL};
    context.info = self;
    CFStringRef path = CFStringCreateWithCString(NULL, PiuToString((*self)->path), kCFStringEncodingUTF8);
    CFArrayRef paths = CFArrayCreate(NULL, (const void **)&path, 1, &kCFTypeArrayCallBacks);
    (*self)->stream = FSEventStreamCreate(kCFAllocatorDefault, PiuSystem_DirectoryNotifierCallback, &context, paths, 
    		kFSEventStreamEventIdSinceNow, 1.0, kFSEventStreamCreateFlagFileEvents);
	CFRelease(paths);
	CFRelease(path);
	if ((*self)->stream) {
		FSEventStreamScheduleWithRunLoop((*self)->stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		FSEventStreamStart((*self)->stream);
    }
    else {
		xsSetHostData(xsThis, NULL);
		xsUnknownError("operation failed");
    }
}

void PiuSystem_DirectoryNotifierDelete(void* it)
{
	if (it) {
		PiuDirectoryNotifier self = it;
		if (self->stream) {
			FSEventStreamStop(self->stream);
			FSEventStreamInvalidate(self->stream);
// 			FSEventStreamUnscheduleFromRunLoop(self->stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
			FSEventStreamRelease(self->stream);
		}
	}
}

void PiuSystem_DirectoryNotifierMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuDirectoryNotifier self = it;
	PiuMarkString(the, self->path);
	PiuMarkReference(the, self->callback);
}

void PiuSystem_DirectoryNotifier_close(xsMachine* the)
{
	PiuDirectoryNotifier* self = PIU(DirectoryNotifier, xsThis);
	PiuSystem_DirectoryNotifierDelete(*self);
	xsSetHostData(xsThis, NULL);
}


