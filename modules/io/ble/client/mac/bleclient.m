/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

#include "xsmc.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "builtinCommon.h"

#import <CoreBluetooth/CoreBluetooth.h>

typedef struct BLEScannerRecord BLEScannerRecord;
typedef struct BLEScannerRecord *BLEScanner;

typedef struct BLEClientRecord BLEClientRecord;
typedef struct BLEClientRecord *BLEClient;

@interface BLEScannerDelegate : NSObject <CBCentralManagerDelegate>
@property (assign) BLEScanner scanner;
- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral;
- (void)centralManager:(CBCentralManager *) central didDisconnectPeripheral:(CBPeripheral *) peripheral error:(NSError *) error;
- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary<NSString *,id> *)advertisementData RSSI:(NSNumber *)RSSI;
- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error;
- (void)centralManagerDidUpdateState:(CBCentralManager *)central;
@end

@interface BLEClientDelegate : NSObject <CBPeripheralDelegate>
@property (assign) BLEClient client;
- (void) peripheral:(CBPeripheral *) peripheral didDiscoverServices:(NSError *) error;
- (void) peripheral:(CBPeripheral *) peripheral didDiscoverCharacteristicsForService:(CBService *) service error:(NSError *) error;
- (void) peripheral:(CBPeripheral *) peripheral didDiscoverDescriptorsForCharacteristic:(CBCharacteristic *) characteristic error:(NSError *) error;
- (void) peripheral:(CBPeripheral *) peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *) characteristic error:(NSError *) error;
- (void) peripheral:(CBPeripheral *) peripheral didUpdateValueForCharacteristic:(CBCharacteristic *) characteristic  error:(NSError *) error;
- (void) peripheral:(CBPeripheral *) peripheral didUpdateValueForDescriptor:(CBDescriptor *) descriptor error:(NSError *) error;
- (void) peripheral:(CBPeripheral *) peripheral didWriteValueForCharacteristic:(CBCharacteristic *) characteristic error:(NSError *) error;
@end

struct BLEScannerRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onReadable;
	CBCentralManager *central;
   	BLEScannerDelegate *delegate;
    NSMutableDictionary* already;
    NSMutableArray* services;
	xsBooleanValue scanning;
	xsSlot *advertisementConstructor;
	xsSlot *advertisement;
	int8_t useCount;
};

struct BLEClientRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onError;
	xsSlot *onReadable;
	CBCentralManager *central;
	CBPeripheral* peripheral;
	BLEClientDelegate *delegate;
	xsSlot *request;
	xsSlot *param;
	CBCharacteristic *completion;
	CBCharacteristic *notification;
	NSMutableArray* uuids;
};

static void BLEScanner_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static const xsHostHooks BLEScannerHooks = {
	BLEScanner_destructor,
	BLEScanner_mark,
	NULL
};

static void BLEClient_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static const xsHostHooks BLEClientHooks = {
	BLEClient_destructor,
	BLEClient_mark,
	NULL
};

static void BLEClientCharacteristicDestructor(void *it)
{
	if (it) {
		CBCharacteristic *characteristic = it;
		[characteristic release];
	}
}

static void BLEClientDescriptorDestructor(void *it)
{
	if (it) {
		CBDescriptor *descriptor = it;
		[descriptor release];
	}
}

static void BLEClientServiceDestructor(void *it)
{
	if (it) {
		CBService *service = it;
		[service release];
	}
}

static void BLEClientRequestFailed(BLEClient client, NSError* error)
{
	xsBeginHost(client->the);
	xsmcVars(1);
	xsVar(0) = xsReference(client->request);
	client->request = NULL;
	client->param = NULL;
	client->completion = NULL;
	xsResult = xsString([[error localizedDescription] UTF8String]);
	xsResult = xsNew1(xsGlobal, xsID_Error, xsResult);
	xsCall2(xsVar(0), xsID_executed, xsResult, xsNull);
	xsEndHost(client->the);
}

@implementation BLEScannerDelegate
- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
	BLEClientDelegate* delegate = peripheral.delegate;
	if (!delegate)
		return;
	BLEClient client = delegate.client;
	xsBeginHost(client->the);
	xsmcVars(1);
	xsVar(0) = xsReference(client->request);
	client->request = NULL;
	client->param = NULL;
	xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
	xsEndHost(client->the);
}
- (void) centralManager:(CBCentralManager *) central didDisconnectPeripheral:(CBPeripheral *) peripheral error:(NSError *) error
{
	BLEClientDelegate* delegate = peripheral.delegate;
	if (!delegate)
		return;
	BLEClient client = delegate.client;
	if (error) {
		xsBeginHost(client->the);
		xsmcVars(1);
		xsResult = xsAccess(client->object);
		xsVar(0) = xsString([[error localizedDescription] UTF8String]);
		xsVar(0) = xsNew1(xsGlobal, xsID_Error, xsVar(0));
		xsCallFunction1(xsReference(client->onError), xsResult, xsVar(0));
		xsEndHost(client->the);
	}
}
- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary<NSString *,id> *)advertisementData RSSI:(NSNumber *)RSSI
{
	BLEScanner scanner = self.scanner;
	if (!scanner)
		return;
	NSString* name = peripheral.name;
	if (!name)
		return;
	NSUUID* uuid = peripheral.identifier;
	if (!uuid)
		return;
	if ([scanner->already objectForKey:uuid])
		return;
	[scanner->already setObject:peripheral forKey:uuid];
	scanner->useCount += 1;
	xsBeginHost(scanner->the);
	xsmcVars(1);
	xsThis = xsAccess(scanner->object);

	xsResult = xsNewFunction0(xsReference(scanner->advertisementConstructor));
	xsmcSetHostData(xsResult, [advertisementData retain]);
	xsmcSetInteger(xsVar(0), [RSSI integerValue]);
	xsmcDefine(xsResult, xsID_rssi, xsVar(0), xsDontDelete | xsDontSet);
	xsVar(0) = xsString([uuid.UUIDString UTF8String]);
	xsmcDefine(xsResult, xsID_address, xsVar(0), xsDontDelete | xsDontSet);
	scanner->advertisement = xsmcToReference(xsResult);
	xsCallFunction1(xsReference(scanner->onReadable), xsThis, xsInteger(1));
	scanner->advertisement = NULL;
	xsEndHost(scanner->the);
	
	BLEScanner_destructor(scanner);		// down use count
}
- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
	BLEClientDelegate* delegate = peripheral.delegate;
	if (!delegate)
		return;
	BLEClient client = delegate.client;
	BLEClientRequestFailed(client, error);
}
- (void)centralManagerDidUpdateState:(CBCentralManager *)central
{
	BLEScanner scanner = self.scanner;
	if (!scanner)
		return;
	if (central.state == CBManagerStatePoweredOn) {
		[central scanForPeripheralsWithServices:scanner->services options:nil];
	}
}
@end

@implementation BLEClientDelegate
- (void) peripheral:(CBPeripheral *) peripheral didDiscoverServices:(NSError *) error
{
	BLEClient client = self.client;
	if (error)
		BLEClientRequestFailed(client, error);
	else {
		xsBeginHost(client->the);
		xsmcVars(3);
		xsVar(0) = xsReference(client->request);
		client->request = NULL;
		NSArray* array = peripheral.services;
		NSUInteger count = array.count, index;
		xsResult = xsmcNewArray(0);
		for (index = 0; index < count; index++) {
			CBService *service = [array objectAtIndex:index];
			if ((client->uuids == nil) || [client->uuids containsObject: service.UUID]) {
				xsVar(1) = xsNewHostObject(BLEClientServiceDestructor);
				xsmcSetHostData(xsVar(1), [service retain]);
				xsVar(2) = xsString([[service.UUID.UUIDString lowercaseString] UTF8String]);
				xsmcDefine(xsVar(1), xsID_uuid, xsVar(2), xsDontDelete | xsDontSet);
				xsCall1(xsResult, xsID_push, xsVar(1));
			}
		}
		client->param = NULL;
		xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
		xsEndHost(client->the);
	}
	if (client->uuids != nil) {
		[client->uuids release];
		client->uuids = nil;
	}
}
- (void) peripheral:(CBPeripheral *) peripheral didDiscoverCharacteristicsForService:(CBService *) service error:(NSError *) error
{
	BLEClient client = self.client;
	if (error)
		BLEClientRequestFailed(client, error);
	else {
		xsBeginHost(client->the);
		xsmcVars(3);
		xsVar(0) = xsReference(client->request);
		client->request = NULL;
		NSArray* array = service.characteristics;
		NSUInteger count = array.count, index;
		xsResult = xsmcNewArray(0);
		for (index = 0; index < count; index++) {
			CBCharacteristic *characteristic = [array objectAtIndex:index];
			if ((client->uuids == nil) || [client->uuids containsObject: characteristic.UUID]) {
				xsVar(1) = xsNewHostObject(BLEClientCharacteristicDestructor);
				xsmcSetHostData(xsVar(1), [characteristic retain]);
				xsVar(2) = xsReference(client->param);
				xsmcDefine(xsVar(1), xsID_service, xsVar(2), xsDontDelete | xsDontSet);
				xsVar(2) = xsString([[characteristic.UUID.UUIDString lowercaseString] UTF8String]);
				xsmcDefine(xsVar(1), xsID_uuid, xsVar(2), xsDontDelete | xsDontSet);
				xsVar(2) = xsInteger(characteristic.properties);
				xsmcDefine(xsVar(1), xsID_properties, xsVar(2), xsDontDelete | xsDontSet);
				xsmcSetInteger(xsVar(2), [[characteristic valueForKey:@"_valueHandle"] integerValue]);
				xsmcDefine(xsVar(1), xsID_handle, xsVar(2), xsDontDelete | xsDontSet);
				xsCall1(xsResult, xsID_push, xsVar(1));
			}
		}
		client->param = NULL;
		xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
		xsEndHost(client->the);
	}
	if (client->uuids != nil) {
		[client->uuids release];
		client->uuids = nil;
	}
}
- (void) peripheral:(CBPeripheral *) peripheral didDiscoverDescriptorsForCharacteristic:(CBCharacteristic *) characteristic error:(NSError *) error
{
	BLEClient client = self.client;
	if (error)
		BLEClientRequestFailed(client, error);
	else {
		xsBeginHost(client->the);
		xsmcVars(3);
		xsVar(0) = xsReference(client->request);
		client->request = NULL;
		NSArray* array = characteristic.descriptors;
		NSUInteger count = array.count, index;
		xsResult = xsmcNewArray(0);
		for (index = 0; index < count; index++) {
			CBDescriptor *descriptor = [array objectAtIndex:index];
			if ((client->uuids == nil) || [client->uuids containsObject: descriptor.UUID]) {
				xsVar(1) = xsNewHostObject(BLEClientDescriptorDestructor);
				xsmcSetHostData(xsVar(1), [descriptor retain]);
				xsVar(2) = xsReference(client->param);
				xsmcDefine(xsVar(1), xsID_characteristic, xsVar(2), xsDontDelete | xsDontSet);
				xsVar(2) = xsString([[descriptor.UUID.UUIDString lowercaseString] UTF8String]);
				xsmcDefine(xsVar(1), xsID_uuid, xsVar(2), xsDontDelete | xsDontSet);
				xsmcSetIndex(xsResult, index, xsVar(1));
			}
		}
		client->param = NULL;
		xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
		xsEndHost(client->the);
	}
	if (client->uuids != nil) {
		[client->uuids release];
		client->uuids = nil;
	}
}
- (void) peripheral:(CBPeripheral *) peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *) characteristic error:(NSError *) error
{
	BLEClient client = self.client;
	if (error)
		BLEClientRequestFailed(client, error);
	else {
		xsBeginHost(client->the);
		xsmcVars(1);
		xsVar(0) = xsReference(client->request);
		client->request = NULL;
		client->param = NULL;
		xsmcSetBoolean(xsResult, characteristic.isNotifying);
		xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
		xsEndHost(client->the);
	}
}
- (void) peripheral:(CBPeripheral *) peripheral didUpdateValueForCharacteristic:(CBCharacteristic *) characteristic  error:(NSError *) error
{
	BLEClient client = self.client;
	if (error)
		BLEClientRequestFailed(client, error);
	else {
		xsBeginHost(client->the);
		xsmcVars(1);
		if (client->completion == characteristic) {
			xsVar(0) = xsReference(client->request);
			client->request = NULL;
			client->param = NULL;
			client->completion = NULL;
			NSData* data = characteristic.value;
			xsmcSetArrayBuffer(xsResult, (xsStringValue)[data bytes], (xsIntegerValue)[data length]);
			xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
			goto bail;
		}
		if (characteristic.isNotifying) {
			xsResult = xsAccess(client->object);
			client->notification = characteristic;
			xsCallFunction1(xsReference(client->onReadable), xsResult, xsInteger(1));
			client->notification = nil;
		}
	bail:
		xsEndHost(client->the);
	}
}
- (void) peripheral:(CBPeripheral *) peripheral didUpdateValueForDescriptor:(CBDescriptor *) descriptor error:(NSError *) error
{
	BLEClient client = self.client;
	if (error)
		BLEClientRequestFailed(client, error);
	else {
		xsBeginHost(client->the);
		xsmcVars(1);
		xsVar(0) = xsReference(client->request);
		client->request = NULL;
		client->param = NULL;
		NSValue* value = descriptor.value;
		NSUInteger size;
		const char* encoding = [value objCType];
		NSGetSizeAndAlignment(encoding, &size, NULL);
		void* ptr = malloc(size);
		if (!ptr)
			fxAbort(client->the, xsNotEnoughMemoryExit);
		[value getValue:ptr];
		xsmcSetArrayBuffer(xsResult, (xsStringValue)ptr, (xsIntegerValue)size);
		free(ptr);
		xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
		xsEndHost(client->the);
	}
}
- (void) peripheral:(CBPeripheral *) peripheral didWriteValueForCharacteristic:(CBCharacteristic *) characteristic error:(NSError *) error
{
	BLEClient client = self.client;
	if (error)
		BLEClientRequestFailed(client, error);
	else {
		xsBeginHost(client->the);
		xsmcVars(1);
		xsVar(0) = xsReference(client->request);
		client->request = NULL;
		client->param = NULL;
		xsCall2(xsVar(0), xsID_executed, xsNull, xsResult);
		xsEndHost(client->the);
	}
}
@end

void BLEAdvertisement_constructor(xsMachine* the)
{
}

void BLEAdvertisement_destructor(void* it)
{
	if (it) {
		NSDictionary *advertisementData = it;
		[advertisementData release];
	}
}

void BLEAdvertisement_get(xsMachine *the)
{
	int type = xsmcToInteger(xsArg(0));
	NSDictionary<NSString *, id> *advertisementData = (NSDictionary<NSString *, id> *)xsmcGetHostDataValidate(xsThis, BLEAdvertisement_destructor);
	if ((3 == type) || (5 == type) || (7 == type)) {
		NSArray<CBUUID *> *mainUUIDs = advertisementData[CBAdvertisementDataServiceUUIDsKey];
		NSArray<CBUUID *> *overflowUUIDs = advertisementData[CBAdvertisementDataOverflowServiceUUIDsKey];
		if (!mainUUIDs && !overflowUUIDs)
			return;

		if (!mainUUIDs) mainUUIDs = @[];
		if (!overflowUUIDs) overflowUUIDs = @[];
		NSArray<CBUUID *> *allUUIDs = [mainUUIDs arrayByAddingObjectsFromArray:overflowUUIDs];

		NSMutableData *uuids = [NSMutableData data];
		for (CBUUID *uuid in allUUIDs) {
			NSUInteger len = uuid.data.length;
			if (((3 == type) && (2 == len)) || ((5 == type) && (4 == len)) || ((7 == type) && (16 == len)))
				[uuids appendData:uuid.data];
		}

		if ([uuids length])
			xsmcSetArrayBuffer(xsResult, (void *)[uuids bytes], [uuids length]);
	}
	else if (255 == type) {
		NSData *manufacturerData = advertisementData[CBAdvertisementDataManufacturerDataKey];
		if (manufacturerData)
			xsmcSetArrayBuffer(xsResult, (void *)[manufacturerData bytes], [manufacturerData length]);
	}
	else if (10 == type) {
		NSNumber *txPower = advertisementData[CBAdvertisementDataTxPowerLevelKey];
		if (txPower) {
			uint8_t powerValue = (uint8_t)[txPower integerValue];
			xsmcSetArrayBuffer(xsResult, &powerValue, sizeof(powerValue));
		}
	}
	else if (9 == type) {
		NSString *localName = advertisementData[CBAdvertisementDataLocalNameKey];
		if (localName) {
			NSData *name = [localName dataUsingEncoding:NSUTF8StringEncoding];
			xsmcSetArrayBuffer(xsResult, (void *)[name bytes], [name length]);
		}
	}
//@@ CBAdvertisementDataServiceDataKey]
//@@ CBAdvertisementDataSolicitedServiceUUIDsKey
}

void BLEAdvertisement_get_name(xsMachine *the)
{
	NSDictionary<NSString *, id> *advertisementData = (NSDictionary<NSString *, id> *)xsmcGetHostDataValidate(xsThis, BLEAdvertisement_destructor);
	NSString *localName = advertisementData[CBAdvertisementDataLocalNameKey];
	if (!localName)
		return;
	xsResult = xsString([localName UTF8String]);
}

void BLEAdvertisement_get_services(xsMachine *the)
{
	xsmcVars(1);
	NSDictionary<NSString *, id> *advertisementData = (NSDictionary<NSString *, id> *)xsmcGetHostDataValidate(xsThis, BLEAdvertisement_destructor);
	NSArray<CBUUID *> *mainUUIDs = advertisementData[CBAdvertisementDataServiceUUIDsKey];
	NSArray<CBUUID *> *overflowUUIDs = advertisementData[CBAdvertisementDataOverflowServiceUUIDsKey];
	if (!mainUUIDs && !overflowUUIDs)
		return;
	xsResult = xsmcNewArray(0);
	for (CBUUID *uuid in mainUUIDs) {
		xsVar(0) = xsString([[uuid.UUIDString lowercaseString] UTF8String]);
		xsCall1(xsResult, xsID_push, xsVar(0));
	}
	for (CBUUID *uuid in overflowUUIDs) {
		xsVar(0) = xsString([[uuid.UUIDString lowercaseString] UTF8String]);
		xsCall1(xsResult, xsID_push, xsVar(0));
	}
}

void BLEAdvertisement_get_manufacturerData(xsMachine *the)
{
	xsmcVars(1);
	NSDictionary<NSString *, id> *advertisementData = (NSDictionary<NSString *, id> *)xsmcGetHostDataValidate(xsThis, BLEAdvertisement_destructor);
	NSData *manufacturerData = advertisementData[CBAdvertisementDataManufacturerDataKey];
	if (!manufacturerData)
		return;
	NSUInteger length = manufacturerData.length;
	if (length < 2)
		return;
	xsmcSetNewObject(xsResult);
	uint8_t* bytes = (uint8_t*)manufacturerData.bytes;
	xsVar(0) = xsInteger(bytes[0] | (bytes[1] << 8));
	xsmcDefine(xsResult, xsID_manufacturer, xsVar(0), xsDontDelete | xsDontSet);
	xsmcSetArrayBuffer(xsVar(0), (void *)(bytes + 2), length - 2);
	xsmcDefine(xsResult, xsID_data, xsVar(0), xsDontDelete | xsDontSet);
}

static NSMutableArray* xsToCBUUIDArray(xsMachine* the)
{
	xsmcGet(xsVar(1), xsVar(0), xsID_length);
	xsIntegerValue length = xsmcToInteger(xsVar(1)), index;
	NSMutableArray* array = [NSMutableArray arrayWithCapacity:length];
	for (index = 0; index < length; index++) {
		xsmcGetIndex(xsVar(1), xsVar(0), index);
		NSString* string = [NSString stringWithUTF8String:xsmcToString(xsVar(1))];
		CBUUID* uuid = [CBUUID UUIDWithString:string];
		[array addObject:uuid];
	}
	return array;
}

CBCentralManager *gCentralManager = nil;

void BLEScanner_constructor(xsMachine* the)
{
	xsmcVars(2);
    BLEScannerDelegate *delegate;
	CBCentralManager *central;
	BLEScanner scanner = NULL;
	
	if (!gCentralManager) {
		delegate = [[BLEScannerDelegate alloc] init];
		if (!delegate)
			xsUnknownError("cannot create delegate");
		central = [[CBCentralManager alloc] initWithDelegate:delegate queue:nil];
		if (!central)
			xsUnknownError("cannot create central");
		gCentralManager = [central retain];
	}
	else {
		central = gCentralManager;
		delegate = gCentralManager.delegate;
		if (delegate.scanner)
			xsUnknownError("already scanning");
	}

	builtinInitializeTarget(the);

	scanner = (BLEScanner)c_calloc(1, sizeof(BLEScannerRecord));
	if (!scanner)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, scanner);
	xsSetHostHooks(xsThis, (xsHostHooks *)&BLEScannerHooks);
	scanner->the = the;
	scanner->object = xsThis;
	xsRemember(scanner->object);
	scanner->onReadable = builtinGetCallback(the, xsID_onReadable);
	scanner->advertisementConstructor = xsmcToReference(xsArg(1));
	
	scanner->useCount = 1;
	scanner->central = [central retain];
	scanner->delegate = [delegate retain];
	delegate.scanner = scanner;
	
	if (xsmcHas(xsArg(0), xsID_services)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_services);
		scanner->services = [xsToCBUUIDArray(the) retain];
	}
	scanner->already = [[NSMutableDictionary dictionaryWithCapacity:16] retain];
	if (scanner->central.state == CBManagerStatePoweredOn)
		[scanner->central scanForPeripheralsWithServices:scanner->services options:nil];
}

void BLEScanner_destructor(void *it)
{
	if (it) {
		BLEScanner scanner = it;
		scanner->useCount -= 1;
		if (scanner->useCount > 0)
			return;

    	if (scanner->delegate) {
    		scanner->delegate.scanner = nil;
			[scanner->delegate release];
		}
    	if (scanner->central)
			[scanner->central release];
		c_free(scanner);
	}
}

void BLEScanner_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	BLEScanner scanner = it;
	if (scanner->onReadable)
		(*markRoot)(the, scanner->onReadable);
	if (scanner->advertisementConstructor)
		(*markRoot)(the, scanner->advertisementConstructor);
	if (scanner->advertisement)
		(*markRoot)(the, scanner->advertisement);
}

void BLEScanner_close(xsMachine *the)
{
	BLEScanner scanner = xsmcGetHostData(xsThis);
	if (scanner && xsmcGetHostDataValidate(xsThis, (void *)&BLEScannerHooks)) {
		[scanner->central stopScan];
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(scanner->object);
		BLEScanner_destructor(scanner);
	}
}

void BLEScanner_read(xsMachine *the)
{
	BLEScanner scanner = (BLEScanner)xsmcGetHostDataValidate(xsThis, (void *)&BLEScannerHooks);
	if (scanner->advertisement)
		xsResult = xsReference(scanner->advertisement);
}

void BLEClient_constructor(xsMachine *the)
{
	xsmcVars(2);
	CBCentralManager* central = gCentralManager;
	if (!central)
		xsUnknownError("no central");
		
	CBPeripheral* peripheral = nil;
	if (xsmcHas(xsArg(0), xsID_address)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_address);
		const char *address = xsmcToString(xsVar(0));
		NSString *uuidString = [NSString stringWithUTF8String:address];
		NSUUID *uuid = [[NSUUID alloc] initWithUUIDString:uuidString];
		NSArray<CBPeripheral *> *peripherals = [central retrievePeripheralsWithIdentifiers:@[uuid]];
		peripheral = peripherals.firstObject;
		if (!peripheral)
			xsUnknownError("unknown peripheral");
	}
	else
		xsUnknownError("no peripheral");
	
	BLEClientDelegate* delegate = [[BLEClientDelegate alloc] init];
	if (!delegate)
		xsUnknownError("cannot create delegate");
	
	builtinInitializeTarget(the);

	BLEClient client = (BLEClient)c_calloc(1, sizeof(BLEClientRecord));
	if (!client)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, client);
	xsSetHostHooks(xsThis, (xsHostHooks *)&BLEClientHooks);
	client->the = the;
	client->object = xsThis;
	xsRemember(client->object);
	client->onError = builtinGetCallback(the, xsID_onError);	
	client->onReadable = builtinGetCallback(the, xsID_onReadable);	
	builtinInitializeTarget(the);
	
	client->central = [central retain];
	client->peripheral = [peripheral retain];
	client->delegate = [delegate retain];
	peripheral.delegate = delegate;
	delegate.client = client;
}

void BLEClient_destructor(void *it)
{
	if (it) {
		BLEClient client = it;
    	if (client->central)
			[client->central release];
    	if (client->peripheral) {
    		client->peripheral.delegate = nil;
			[client->peripheral release];
		}
    	if (client->delegate) {
    		client->delegate.client = NULL;
			[client->delegate release];
		}
		c_free(client);
	}
}

void BLEClient_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	BLEClient client = it;
	if (client->onError)
		(*markRoot)(the, client->onError);
	if (client->onReadable)
		(*markRoot)(the, client->onReadable);
}

void BLEClient_close(xsMachine* the)
{
	BLEClient client = xsmcGetHostData(xsThis);
	if (client && xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(client->object);
		BLEClient_destructor(client);
	}
}

void BLEClient_connect(xsMachine *the)
{
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	client->request = xsmcToReference(xsArg(0));
	[client->central connectPeripheral:client->peripheral options:[NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithBool:YES], CBConnectPeripheralOptionNotifyOnDisconnectionKey,
		nil]
	];
}

void BLEClient_disconnect(xsMachine *the)
{
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	[client->central cancelPeripheralConnection:client->peripheral];
}

void BLEClient_getPrimaryServices(xsMachine *the)
{
	xsmcVars(2);
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	NSMutableArray* uuids = nil;
	if (xsmcTest(xsArg(1))) {
		xsVar(0) = xsArg(1);
		uuids = xsToCBUUIDArray(the);
		client->uuids = [uuids retain];
	}
	client->request = xsmcToReference(xsArg(0));
	[client->peripheral discoverServices:uuids];
}

void BLEClient_getCharacteristics(xsMachine *the)
{
	xsmcVars(2);
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	CBService* service = (CBService*)xsmcGetHostDataValidate(xsArg(1), (void *)BLEClientServiceDestructor);
	NSMutableArray* uuids = nil;
	if (xsmcTest(xsArg(2))) {
		xsVar(0) = xsArg(2);
		uuids = xsToCBUUIDArray(the);
		client->uuids = [uuids retain];
	}
	client->request = xsmcToReference(xsArg(0));
	client->param = xsmcToReference(xsArg(1));
	[client->peripheral discoverCharacteristics:uuids forService:service];
}

void BLEClient_getDescriptors(xsMachine *the)
{
	xsmcVars(2);
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	CBCharacteristic* characteristic = (CBCharacteristic*)xsmcGetHostDataValidate(xsArg(1), (void *)BLEClientCharacteristicDestructor);
	NSMutableArray* uuids = nil;
	if (xsmcTest(xsArg(2))) {
		xsVar(0) = xsArg(2);
		uuids = xsToCBUUIDArray(the);
		client->uuids = [uuids retain];
	}
	client->request = xsmcToReference(xsArg(0));
	client->param = xsmcToReference(xsArg(1));
	[client->peripheral discoverDescriptorsForCharacteristic:characteristic];
}

void BLEClient_read(xsMachine *the)
{
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	client->request = xsmcToReference(xsArg(0));
	xsDestructor destructor = xsGetHostDestructor(xsArg(1));
	if (destructor == BLEClientCharacteristicDestructor) {
		CBCharacteristic* characteristic = (CBCharacteristic*)xsmcGetHostData(xsArg(1));
		client->request = xsmcToReference(xsArg(0));
		client->param = xsmcToReference(xsArg(1));
		client->completion = characteristic;
		[client->peripheral readValueForCharacteristic:characteristic];
	}
	else if (destructor == BLEClientDescriptorDestructor) {
		CBDescriptor* descriptor = (CBDescriptor*)xsmcGetHostData(xsArg(1));
		client->request = xsmcToReference(xsArg(0));
		client->param = xsmcToReference(xsArg(1));
		[client->peripheral readValueForDescriptor:descriptor];
	}
	else
		xsUnknownError("neither characteristic nor descriptor");
}

void BLEClient_readNotification(xsMachine *the)
{
	xsmcVars(1);
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	CBCharacteristic* characteristic = client->notification;
	if (!characteristic)
		return;
	NSData* data = characteristic.value;
	xsmcSetArrayBuffer(xsResult, (xsStringValue)[data bytes], (xsIntegerValue)[data length]);
	xsmcSetInteger(xsVar(0), [[characteristic valueForKey:@"_valueHandle"] integerValue]);
	xsmcDefine(xsResult, xsID_handle, xsVar(0), xsDontDelete | xsDontSet);
}

void BLEClient_write(xsMachine *the)
{
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	CBCharacteristic* characteristic = (CBCharacteristic*)xsmcGetHostDataValidate(xsArg(1), (void *)BLEClientCharacteristicDestructor);
	void* buffer;
	xsUnsignedValue length;
	xsmcGetBufferReadable(xsArg(2), &buffer, &length);
	NSData* data = [NSData dataWithBytes:buffer length:length];
	client->request = xsmcToReference(xsArg(0));
	client->param = xsmcToReference(xsArg(1));
	[client->peripheral writeValue:data forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
}

void BLEClient_enableNotifications(xsMachine *the)
{
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	CBCharacteristic* characteristic = (CBCharacteristic*)xsmcGetHostDataValidate(xsArg(1), (void *)BLEClientCharacteristicDestructor);
	xsBooleanValue enable = xsmcToBoolean(xsArg(2));
	client->request = xsmcToReference(xsArg(0));
	client->param = xsmcToReference(xsArg(1));
	[client->peripheral setNotifyValue:enable forCharacteristic:characteristic];
}

void BLEClient_get_maximumWrite(xsMachine *the)
{
	BLEClient client = (BLEClient)xsmcGetHostDataValidate(xsThis, (void *)&BLEClientHooks);
	NSUInteger length = [client->peripheral maximumWriteValueLengthForType:CBCharacteristicWriteWithResponse];
	xsmcSetInteger(xsResult, length);
}
