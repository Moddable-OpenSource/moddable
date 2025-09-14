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

typedef struct BLEServerRecord BLEServerRecord;
typedef struct BLEServerRecord *BLEServer;

@interface BLEServerDelegate : NSObject <CBPeripheralManagerDelegate>
@property (assign) BLEServer server;
- (void) peripheralManagerDidUpdateState:(CBPeripheralManager *) peripheral;
- (void) peripheralManager:(CBPeripheralManager *) peripheral didAddService:(CBService *) service error:(NSError *) error;
- (void) peripheralManagerDidStartAdvertising:(CBPeripheralManager *) peripheral error:(NSError *) error;
- (void) peripheralManager:(CBPeripheralManager *) peripheral central:(CBCentral *) central didSubscribeToCharacteristic:(CBCharacteristic *) characteristic;
- (void) peripheralManager:(CBPeripheralManager *) peripheral central:(CBCentral *) central didUnsubscribeFromCharacteristic:(CBCharacteristic *) characteristic;
- (void) peripheralManager:(CBPeripheralManager *) peripheral didReceiveReadRequest:(CBATTRequest *) request;
- (void) peripheralManager:(CBPeripheralManager *) peripheral didReceiveWriteRequests:(NSArray<CBATTRequest *> *) requests;
- (id) notifyFailure: (id)object;
- (id) notifySuccess: (id)object;
@end

@interface BLEServerCharacteristic : CBMutableCharacteristic
@property (assign) xsSlot* onSubscribe;
@property (assign) xsSlot* onUnsubscribe;
@property (assign) xsSlot* onRead;
@property (assign) xsSlot* onWrite;
@property (assign) xsSlot* server;
@property (assign) xsSlot* target;
@property (assign) xsSlot* notifyCallback;
@end

struct BLEServerRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onConnect;
	xsSlot *onDisconnect;
	xsSlot *onError;
	xsSlot *onReady;
	xsSlot *onWarning;
	xsSlot *characteristicPrototype;
	xsSlot *connectionPrototype;
	xsSlot *connections;
	CBPeripheralManager *peripheral;
   	BLEServerDelegate *delegate;
    NSMutableArray* services;
	int8_t useCount;
};

static void BLEServer_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static const xsHostHooks BLEServerHooks = {
	BLEServer_destructor,
	BLEServer_mark,
	NULL
};

CBPeripheralManager *gPeripheralManager = nil;

static void BLEServerFailed(BLEServer server, NSError* error)
{
	if (server->onError) {
		xsBeginHost(server->the);
		xsThis = xsAccess(server->object);
		xsResult = xsString([[error localizedDescription] UTF8String]);
		xsResult = xsNew1(xsGlobal, xsID_Error, xsResult);
		xsCallFunction1(xsReference(server->onError), xsThis, xsResult);
		xsEndHost(client->the);
	}
}

static void BLEServerAddCentral(xsMachine* the, BLEServer server, CBCentral* central)
{
	xsVar(1) = xsReference(server->connections);
	xsmcGet(xsVar(0), xsVar(1), xsID_length);
	xsIntegerValue length = xsmcToInteger(xsVar(0)), index;
	for (index = 0; index < length; index++) {
		xsmcGetIndex(xsVar(0), xsVar(1), index);
		CBCentral* connection = (CBCentral*)xsmcGetHostDataValidate(xsVar(0), (void *)BLEServerConnection_destructor);
		if (connection == central)
			return;
	}
	xsVar(0) = xsNewHostInstance(xsReference(server->connectionPrototype));
	xsmcSetHostData(xsVar(0), [central retain]);
	xsCall1(xsVar(1), xsID_push, xsVar(0));
	if (server->onConnect) {
		xsThis = xsAccess(server->object);
		xsCallFunction1(xsReference(server->onConnect), xsThis, xsVar(0));
	}
}

static void BLEServerFindConnection(xsMachine* the, BLEServer server, CBCentral* central)
{
	xsVar(1) = xsReference(server->connections);
	xsmcGet(xsVar(0), xsVar(1), xsID_length);
	xsIntegerValue length = xsmcToInteger(xsVar(0)), index;
	for (index = 0; index < length; index++) {
		xsmcGetIndex(xsVar(0), xsVar(1), index);
		CBCentral* connection = (CBCentral*)xsmcGetHostDataValidate(xsVar(0), (void *)BLEServerConnection_destructor);
		if (connection == central)
			return;
	}
	xsmcSetUndefined(xsVar(0));
}


@implementation BLEServerDelegate
- (void) peripheralManagerDidUpdateState:(CBPeripheralManager *) peripheral
{
	BLEServer server = self.server;
	if (!server)
		return;
	if (peripheral.state == CBManagerStatePoweredOn) {
		for (CBMutableService* service in server->services) {
			[peripheral addService:service];
		}
		xsBeginHost(server->the);
		xsThis = xsAccess(server->object);
		xsCallFunction0(xsReference(server->onReady), xsThis);
		xsEndHost(the);
	}
}

- (void) peripheralManager:(CBPeripheralManager *) peripheral didAddService:(CBService *) service error:(NSError *) error
{
	BLEServer server = self.server;
	if (!server)
		return;
	if (error)
		BLEServerFailed(server, error);
}

- (void) peripheralManagerDidStartAdvertising:(CBPeripheralManager *) peripheral error:(NSError *) error
{
	BLEServer server = self.server;
	if (!server)
		return;
	if (error)
		BLEServerFailed(server, error);
}

- (void) peripheralManager:(CBPeripheralManager *) peripheral central:(CBCentral *) central didSubscribeToCharacteristic:(CBCharacteristic *) _characteristic
{
	BLEServer server = self.server;
	BLEServerCharacteristic* characteristic = (BLEServerCharacteristic*)_characteristic;
	if (!server || !characteristic.target || !characteristic.onSubscribe)
		return;
	xsBeginHost(server->the);
	xsmcVars(2);
	BLEServerAddCentral(the, server, central);
	xsThis = xsReference(characteristic.target);
	xsCallFunction1(xsReference(characteristic.onSubscribe), xsThis, xsVar(0));
 	xsEndHost(the);
}

- (void) peripheralManager:(CBPeripheralManager *) peripheral central:(CBCentral *) central didUnsubscribeFromCharacteristic:(CBCharacteristic *) _characteristic
{
	BLEServer server = self.server;
	BLEServerCharacteristic* characteristic = (BLEServerCharacteristic*)_characteristic;
	if (!server || !characteristic.target || !characteristic.onUnsubscribe)
		return;
	xsBeginHost(server->the);
	xsmcVars(2);
	BLEServerAddCentral(the, server, central);
	xsThis = xsReference(characteristic.target);
	xsCallFunction1(xsReference(characteristic.onUnsubscribe), xsThis, xsVar(0));
 	xsEndHost(the);
}

- (void) peripheralManager:(CBPeripheralManager *) peripheral didReceiveReadRequest:(CBATTRequest *) request
{
	BLEServer server = self.server;
	BLEServerCharacteristic* characteristic = (BLEServerCharacteristic*)request.characteristic;
	if (!server || !characteristic.target || !characteristic.onRead) {
		[peripheral respondToRequest:request withResult:CBATTErrorUnlikelyError];
		return;
	}
	xsBeginHost(server->the);
	xsmcVars(2);
	BLEServerAddCentral(the, server, request.central);
	xsThis = xsReference(characteristic.target);
	xsResult = xsCallFunction1(xsReference(characteristic.onRead), xsThis, xsVar(0));
	void* buffer;
	xsUnsignedValue length;
	xsmcGetBufferReadable(xsResult, &buffer, &length);
	request.value = [NSData dataWithBytes:buffer length:length];
	[peripheral respondToRequest:request withResult:CBATTErrorSuccess];
 	xsEndHost(the);
}

- (void) peripheralManager:(CBPeripheralManager *) peripheral didReceiveWriteRequests:(NSArray<CBATTRequest *> *) requests
{
	BLEServer server = self.server;
	CBATTRequest* request = requests[0];
	BLEServerCharacteristic* characteristic = (BLEServerCharacteristic*)request.characteristic;
	if (!server || !characteristic.target || !characteristic.onWrite) {
		[peripheral respondToRequest:request withResult:CBATTErrorUnlikelyError];
		return;
	}
	xsBeginHost(server->the);
	xsmcVars(2);
	BLEServerAddCentral(the, server, request.central);
	xsThis = xsReference(characteristic.target);
	NSData* data = request.value;
	xsmcSetArrayBuffer(xsResult, (xsStringValue)[data bytes], (xsIntegerValue)[data length]);
	xsCallFunction2(xsReference(characteristic.onWrite), xsThis, xsResult, xsVar(0));
	[peripheral respondToRequest:request withResult:CBATTErrorSuccess];
 	xsEndHost(the);
}
- (id) notifyFailure: (id)object
{
	CBPeripheralManager* peripheral = gPeripheralManager;
	if (!peripheral)
 		return nil;
    BLEServerDelegate* delegate = peripheral.delegate;
	if (!delegate)
 		return nil;
	BLEServer server = delegate.server;
	if (!server)
 		return nil;
 	BLEServerCharacteristic* characteristic = object;
	if (!characteristic.notifyCallback)
 		return nil;
	xsBeginHost(server->the);
	xsResult = xsString("transmit queue is full");
	xsResult = xsNew1(xsGlobal, xsID_Error, xsResult);
	xsCallFunction2(xsReference(characteristic.notifyCallback), xsGlobal, xsResult, xsNull);
 	xsEndHost(the);
 	return nil;
}
- (id) notifySuccess: (id)object
{
	CBPeripheralManager* peripheral = gPeripheralManager;
	if (!peripheral)
 		return nil;
    BLEServerDelegate* delegate = peripheral.delegate;
	if (!delegate)
 		return nil;
	BLEServer server = delegate.server;
	if (!server)
 		return nil;
 	BLEServerCharacteristic* characteristic = object;
	if (!characteristic.notifyCallback)
 		return nil;
	xsBeginHost(server->the);
	xsCallFunction2(xsReference(characteristic.notifyCallback), xsGlobal, xsNull, xsNull);
 	xsEndHost(the);
 	return nil;
}
@end

@implementation BLEServerCharacteristic
@synthesize onSubscribe;
@synthesize onUnsubscribe;
@synthesize onRead;
@synthesize onWrite;
@synthesize server;
@synthesize target;
@end

void BLEServer_build(xsMachine* the)
{
	xsmcVars(2);
    BLEServerDelegate *delegate;
	CBPeripheralManager *peripheral;
	BLEServer server = NULL;
	
	if (!gPeripheralManager) {
		delegate = [[BLEServerDelegate alloc] init];
		if (!delegate)
			xsUnknownError("cannot create delegate");
		peripheral = [[CBPeripheralManager alloc] initWithDelegate:delegate queue:nil options:[NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithBool:YES], CBPeripheralManagerOptionShowPowerAlertKey,
			nil]
		];
		if (!peripheral)
			xsUnknownError("cannot create peripheral");
		gPeripheralManager = [peripheral retain];
	}
	else {
		peripheral = gPeripheralManager;
		delegate = gPeripheralManager.delegate;
		if (delegate.server)
			xsUnknownError("already serving");
	}

	server = (BLEServer)c_calloc(1, sizeof(BLEServerRecord));
	if (!server)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, server);
	xsSetHostHooks(xsThis, (xsHostHooks *)&BLEServerHooks);
	server->the = the;
	server->object = xsThis;
	xsRemember(server->object);
	server->onConnect = builtinGetCallback(the, xsID_onConnect);
	server->onDisconnect = builtinGetCallback(the, xsID_onDisconnect);
	server->onError = builtinGetCallback(the, xsID_onError);
	server->onReady = builtinGetCallback(the, xsID_onReady);
	server->onWarning = builtinGetCallback(the, xsID_onWarning);
	server->characteristicPrototype = xsmcToReference(xsArg(2));
	server->connectionPrototype = xsmcToReference(xsArg(1));
	xsVar(0) = xsmcNewArray(0);
	server->connections = xsmcToReference(xsVar(0));
	
	server->useCount = 1;
	server->peripheral = [peripheral retain];
	server->delegate = [delegate retain];
	server->services = [[NSMutableArray arrayWithCapacity:16] retain];
	delegate.server = server;
}

void BLEServer_addService(xsMachine *the)
{
	xsmcVars(5);
	BLEServer server = (BLEServer)xsmcGetHostDataValidate(xsThis, (void *)&BLEServerHooks);
	CBUUID* uuid;
	xsmcGet(xsVar(0), xsArg(0), xsID_characteristics);
	xsmcGet(xsVar(1), xsVar(0), xsID_length);
	int characteristicsLength = xsmcToInteger(xsVar(1)), characteristicIndex;
	if (characteristicsLength < 0)
		xsRangeError("invalid characteristics");
	NSMutableArray* characteristics = [NSMutableArray arrayWithCapacity:characteristicsLength];
	for (characteristicIndex = 0; characteristicIndex < characteristicsLength; characteristicIndex++) {
		xsmcGetIndex(xsVar(1), xsVar(0), characteristicIndex);
		xsmcGet(xsVar(2), xsVar(1), xsID_uuid);
		CBUUID* uuid = [CBUUID UUIDWithString:[NSString stringWithUTF8String:xsmcToString(xsVar(2))]];
		NSData* value = nil;
		xsmcGet(xsVar(2), xsVar(1), xsID_properties);
		if (!xsmcTest(xsVar(2)))
			xsUnknownError("no properties");
		CBCharacteristicProperties properties = xsmcToInteger(xsVar(2));
		if (properties < 0)
			xsRangeError("invalid properties");
		CBAttributePermissions permissions = 0;
		if (properties & CBCharacteristicPropertyRead) {
			permissions |= CBAttributePermissionsReadable;
			if (properties > 0xFF) {
				permissions |= CBAttributePermissionsReadEncryptionRequired;
				properties &= 0xFF;
			}
		}
		if (properties & (CBCharacteristicPropertyWriteWithoutResponse | CBCharacteristicPropertyWrite | CBCharacteristicPropertyAuthenticatedSignedWrites)) {
			permissions |= CBAttributePermissionsWriteable;
			if (properties > 0xFF) {
				permissions |= CBAttributePermissionsWriteEncryptionRequired;
				properties &= 0xFF;
			}
		}
		if (properties & CBCharacteristicPropertyNotify) {
			if (properties > 0xFF) {
				properties = CBCharacteristicPropertyNotifyEncryptionRequired;
			}
		}
		if (properties & CBCharacteristicPropertyIndicate) {
			if (properties > 0xFF) {
				properties = CBCharacteristicPropertyIndicateEncryptionRequired;
			}
		}
		if (xsmcHas(xsVar(1), xsID_value)) {
			void* buffer;
			xsUnsignedValue length;
			xsmcGet(xsVar(2), xsVar(1), xsID_value);
			xsmcGetBufferReadable(xsVar(2), &buffer, &length);
			value = [NSData dataWithBytes:buffer length:length];
		}
		BLEServerCharacteristic* characteristic = [[BLEServerCharacteristic alloc] initWithType:uuid properties:properties value:value permissions:permissions];
		if (value == nil) {	
			xsmcGet(xsVar(2), xsVar(1), xsID_onRead);
			xsSlot* onRead = xsmcToReference(xsVar(2));
			xsmcGet(xsVar(2), xsVar(1), xsID_onWrite);
			xsSlot* onWrite = xsmcToReference(xsVar(2));
			xsmcGet(xsVar(2), xsVar(1), xsID_onSubscribe);
			xsSlot* onSubscribe = xsmcToReference(xsVar(2));
			xsmcGet(xsVar(2), xsVar(1), xsID_onUnsubscribe);
			xsSlot* onUnsubscribe = xsmcToReference(xsVar(2));
			if ((C_NULL == onRead) != !(properties & CBCharacteristicPropertyRead))
				xsUnknownError("inconsistent onRead");
			if ((C_NULL == onWrite) != !(properties & (CBCharacteristicPropertyWriteWithoutResponse | CBCharacteristicPropertyWrite | CBCharacteristicPropertyAuthenticatedSignedWrites)))
				xsUnknownError("inconsistent onWrite");
			if (!!onSubscribe != !!(properties & (CBCharacteristicPropertyNotify | CBCharacteristicPropertyIndicate | CBCharacteristicPropertyNotifyEncryptionRequired | CBCharacteristicPropertyIndicateEncryptionRequired )))
				xsUnknownError("inconsistent onSubscribe");
			xsVar(2) = xsNewHostInstance(xsReference(server->characteristicPrototype));
			xsmcSetHostData(xsVar(2), [characteristic retain]);
			characteristic.onSubscribe = onSubscribe;
			characteristic.onUnsubscribe = onUnsubscribe;
			characteristic.onRead = onRead;
			characteristic.onWrite = onWrite;
			characteristic.server = xsmcToReference(xsThis);
			characteristic.target = xsmcToReference(xsVar(2));
		}
		else
			characteristic.target = NULL;
		[characteristics addObject:characteristic];
		
		xsmcGet(xsVar(2), xsVar(1), xsID_descriptors);
		if (!xsmcTest(xsVar(2)))
			continue;
		xsmcGet(xsVar(3), xsVar(2), xsID_length);
		int descriptorsLength = xsmcToInteger(xsVar(3)), descriptorIndex;
		if (descriptorsLength <= 0)
			xsRangeError("invalid descriptors");
		NSMutableArray* descriptors = [NSMutableArray arrayWithCapacity:descriptorsLength];
		for (descriptorIndex = 0; descriptorIndex < descriptorsLength; descriptorIndex++) {
			xsmcGetIndex(xsVar(3), xsVar(2), descriptorIndex);
			xsmcGet(xsVar(4), xsVar(3), xsID_uuid);
			NSString* string = [NSString stringWithUTF8String:xsmcToString(xsVar(4))];
			CBUUID* uuid = [CBUUID UUIDWithString:string];
			id value = nil;
			if (xsmcHas(xsVar(3), xsID_value)) {
				xsmcGet(xsVar(4), xsVar(3), xsID_value);
				void* buffer;
				xsUnsignedValue length;
				xsmcGetBufferReadable(xsVar(4), &buffer, &length);
				if ([string isEqualToString:CBUUIDCharacteristicUserDescriptionString]) {
					value = [NSString stringWithUTF8String:buffer];
				}
				else if ([string isEqualToString:CBUUIDCharacteristicFormatString]) {
					value = [NSData dataWithBytes:buffer length:length];
				}
				else {
					if (server->onWarning) {
						xsCallFunction1(xsReference(server->onWarning), xsThis, xsString("only user description and format descriptors on macOS")); 
					}
				}
			}
			else {
				if (server->onWarning) {
					xsCallFunction1(xsReference(server->onWarning), xsThis, xsString("no descriptors without value on macOS")); 
				}
			}
			if (value) {
				CBMutableDescriptor* descriptor = [[CBMutableDescriptor alloc] initWithType:uuid value:value];
				[descriptors addObject:descriptor];
			}
		}
		characteristic.descriptors = descriptors;
	}
	xsmcGet(xsVar(0), xsArg(0), xsID_uuid);
	uuid = [CBUUID UUIDWithString:[NSString stringWithUTF8String:xsmcToString(xsVar(0))]];
	CBMutableService* service = [[CBMutableService alloc] initWithType:uuid primary:YES];
	service.characteristics = characteristics;
	[server->services addObject:service];
	if (server->peripheral.state == CBManagerStatePoweredOn)
		[server->peripheral addService:service];
}

void BLEServer_close(xsMachine *the)
{
	xsmcVars(2);
	BLEServer server = xsmcGetHostData(xsThis);
	if (server && xsmcGetHostDataValidate(xsThis, (void *)&BLEServerHooks)) {
		for (CBMutableService* service in server->services) {
			for (BLEServerCharacteristic* characteristic in service.characteristics) {
				if (characteristic.target && characteristic.onUnsubscribe) {
					for (CBCentral* subscribedCentral in characteristic.subscribedCentrals) {
						BLEServerFindConnection(the, server, subscribedCentral);
						xsVar(1) = xsReference(characteristic.target);
						xsCallFunction1(xsReference(characteristic.onUnsubscribe), xsVar(1),  xsVar(0));
					}
				}
			}
		}
		[server->peripheral removeAllServices];
		[server->peripheral stopAdvertising];
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(server->object);
		BLEServer_destructor(server);
	}
}

void BLEServer_deleteService(xsMachine *the)
{
}

void BLEServer_destructor(void *it)
{
	if (it) {
		BLEServer server = it;
		server->useCount -= 1;
		if (server->useCount > 0)
			return;

    	if (server->delegate) {
    		server->delegate.server = nil;
			[server->delegate release];
		}
    	if (server->peripheral) {
			[server->peripheral release];
		}
		c_free(server);
	}
}

void BLEServer_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	BLEServer server = it;
	if (server->onError)
		(*markRoot)(the, server->onError);
	if (server->onReady)
		(*markRoot)(the, server->onReady);
	if (server->onWarning)
		(*markRoot)(the, server->onWarning);
	if (server->connections)
		(*markRoot)(the, server->connections);
	for (CBMutableService* service in server->services) {
		for (BLEServerCharacteristic* characteristic in service.characteristics) {
			if (characteristic.onSubscribe)
				(*markRoot)(the, characteristic.onSubscribe);
			if (characteristic.onUnsubscribe)
				(*markRoot)(the, characteristic.onUnsubscribe);
			if (characteristic.onRead)
				(*markRoot)(the, characteristic.onRead);
			if (characteristic.onWrite)
				(*markRoot)(the, characteristic.onWrite);
			if (characteristic.target)
				(*markRoot)(the, characteristic.target);
			if (characteristic.notifyCallback)
				(*markRoot)(the, characteristic.notifyCallback);
		}
	}
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

void BLEServer_startAdvertising(xsMachine *the)
{
	BLEServer server = (BLEServer)xsmcGetHostDataValidate(xsThis, (void *)&BLEServerHooks);
	xsBooleanValue hasName = 0, hasServices = 0;
	NSMutableDictionary *data = nil;
	xsmcVars(3);
	
	xsVar(1) = xsEnumerate(xsArg(0));
	for (;;) {
		xsVar(2) = xsCall0(xsVar(1), xsID_next);
		xsmcGet(xsVar(0), xsVar(2), xsID_done);
		if (xsmcTest(xsVar(0)))
			break;
		xsmcGet(xsVar(0), xsVar(2), xsID_value);
		xsStringValue key = xsmcToString(xsVar(0));
		if (!c_strcmp(key, "name"))
			hasName = 1;
		else if (!c_strcmp(key, "services"))
			hasServices = 1;
		else if (server->onWarning) {
			char buffer[1024];
			snprintf(buffer, sizeof(buffer), "no '%s' advertising on macOS", key);
			xsCallFunction1(xsReference(server->onWarning), xsThis, xsString(buffer)); 
		}
	}
	if (hasName || hasServices) {
		data = [NSMutableDictionary dictionaryWithCapacity:hasName + hasServices];
		if (hasName) {
			xsmcGet(xsVar(0), xsArg(0), xsID_name);
			NSString* value = [NSString stringWithUTF8String:xsmcToString(xsVar(0))];
			[data setValue:value forKey:CBAdvertisementDataLocalNameKey];
		}
		if (hasServices) {
			xsmcGet(xsVar(0), xsArg(0), xsID_services);
			NSMutableArray* value = xsToCBUUIDArray(the);
			[data setValue:value forKey:CBAdvertisementDataServiceUUIDsKey];
		}
	}
	[server->peripheral startAdvertising:data];
}

void BLEServer_stopAdvertising(xsMachine *the)
{
	BLEServer server = (BLEServer)xsmcGetHostDataValidate(xsThis, (void *)&BLEServerHooks);
	[server->peripheral stopAdvertising];
}

void BLEServerCharacteristic_destructor(void *it)
{
	if (it) {
		BLEServerCharacteristic* characteristic = it;
		[characteristic release];
	}
}

void BLEServerConnection_destructor(void *it)
{
	if (it) {
		CBCentral* central = it;
		[central release];
	}
}

void BLEServerConnection_close(xsMachine *the)
{
	xsmcVars(2);
	CBCentral* central = xsmcGetHostData(xsThis);
	if (!central)
		return;
	if (!xsmcGetHostDataValidate(xsThis, (void *)BLEServerConnection_destructor))
		return;
	CBPeripheralManager* peripheral = gPeripheralManager;
	if (!peripheral)
		return;
    BLEServerDelegate* delegate = peripheral.delegate;
	if (!delegate)
		return;
	BLEServer server = delegate.server;
	if (!server)
		return;
	if (server->onWarning) {
		xsThis = xsAccess(server->object);
		xsCallFunction1(xsReference(server->onWarning), xsThis, xsString("connections cannot be closed on macOS")); 
	}
}

void BLEServerConnection_notify(xsMachine *the)
{
	CBCentral* central = (CBCentral*)xsmcGetHostDataValidate(xsThis, (void *)BLEServerConnection_destructor);
	CBPeripheralManager* peripheral = gPeripheralManager;
	if (!peripheral)
		return;
    BLEServerDelegate* delegate = peripheral.delegate;
	if (!delegate)
		return;
	BLEServer server = delegate.server;
	if (!server)
		return;
	BLEServerCharacteristic* characteristic = (BLEServerCharacteristic*)xsmcGetHostDataValidate(xsArg(0), (void *)BLEServerCharacteristic_destructor);
	if (xsmcArgc > 2)
		characteristic.notifyCallback = xsmcToReference(xsArg(2));
	else
		characteristic.notifyCallback = NULL;
	void* buffer;
	xsUnsignedValue length;
	xsmcGetBufferReadable(xsArg(1), &buffer, &length);
	NSData* value = [NSData dataWithBytes:buffer length:length];
	if ([server->peripheral updateValue:value forCharacteristic:characteristic onSubscribedCentrals:[NSArray arrayWithObject:central]]) {
		if (characteristic.notifyCallback)
			[delegate performSelector:@selector(notifySuccess:) withObject:characteristic afterDelay:0];	
	}
	else {
		if (characteristic.notifyCallback)
			[delegate performSelector:@selector(notifyFailure:) withObject:characteristic afterDelay:0];	
	}
}
