/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#ifndef __mod_ble__
#define __mod_ble__

#include "inttypes.h"
#include "xsmc.h"

typedef enum {
	NoInputNoOutput = 0,
	DisplayOnly,
	KeyboardOnly,
	KeyboardDisplay,
	DisplayYesNo
} IOCapability;

typedef enum {
	LE_LIMITED_DISCOVERABLE_MODE = (1L << 0),
	LE_GENERAL_DISCOVERABLE_MODE = (1L << 1),
	NO_BR_EDR = (1L << 2),
	LE_BR_EDR_CONTROLLER = (1L << 3),
	LE_BR_EDR_HOST = (1L << 4)
} AdvertisingFlags;

typedef enum {
	kBLEAddressTypePublic = 0,
	kBLEAddressTypeRandom,
	kBLEAddressTypeRPAPublic,
	kBLEAddressTypeRPARandom
} BLEAddressType;

typedef enum {
	kBLEScanFilterPolicyNone = 0,
	kBLEScanFilterPolicyWhitelist,
	kBLEScanFilterNotResolvedDirected,
	kBLEScanFilterWhitelistNotResolvedDirected
} BLEScanFilterPolicy;

typedef enum {
	kBLEAdvFilterPolicyNone = 0,
	kBLEAdvFilterPolicyWhitelistScans,
	kBLEAdvFilterPolicyWhitelistConnections,
	kBLEAdvFilterPolicyWhitelistScansConnections
} BLEAdvFilterPolicy;

/* simulator build */

#if (mxMacOSX || mxWindows || mxLinux)
	#define modCriticalSectionDeclare
	#define modCriticalSectionBegin()
	#define modCriticalSectionEnd()
	#define modMessagePostToMachine(...)
	typedef void (*modMessageDeliver)(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
#endif

/* whitelist */

typedef struct modBLEWhitelistAddressRecord modBLEWhitelistAddressRecord;
typedef modBLEWhitelistAddressRecord *modBLEWhitelistAddress;

struct modBLEWhitelistAddressRecord {
	struct modBLEWhitelistAddressRecord *next;

	uint8_t address[6];
	BLEAddressType addressType;
};

int modBLEWhitelistContains(uint8_t addressType, uint8_t *address);
void modBLEWhitelistClear(void);

/* security */

uint16_t modBLESetSecurityParameters(uint8_t encryption, uint8_t bonding, uint8_t mitm, uint16_t ioCapability);

/* connections */

#define kInvalidConnectionID 0xFFFF

typedef enum {
	kBLEConnectionTypeClient = 0,
	kBLEConnectionTypeServer
} BLEConnectionType;

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

#define modBLEConnectionPart \
	struct modBLEConnectionRecord *next; \
	xsMachine	*the; \
	xsSlot		objConnection; \
	uint16_t	id; \
	uint8_t		type; \
	uint8_t		addressType; \
	uint8_t		address[6]; \
	uint8_t		mtu_exchange_pending;

struct modBLEConnectionRecord {
	modBLEConnectionPart
};

void modBLEConnectionAdd(modBLEConnection connection);
void modBLEConnectionRemove(modBLEConnection connection);
modBLEConnection modBLEConnectionFindByConnectionID(int16_t conn_id);
modBLEConnection modBLEConnectionFindByAddress(uint8_t *address);
modBLEConnection modBLEConnectionFindByAddressAndType(uint8_t *address, uint8_t addressType);
modBLEConnection modBLEConnectionGetFirst(void);
modBLEConnection modBLEConnectionGetNext(modBLEConnection connection);

/* message queue */

typedef struct modBLEMessageQueueRecord modBLEMessageQueueRecord;
typedef modBLEMessageQueueRecord *modBLEMessageQueue;

typedef struct modBLEMessageQueueEntryRecord modBLEMessageQueueEntryRecord;
typedef modBLEMessageQueueEntryRecord *modBLEMessageQueueEntry;

struct modBLEMessageQueueRecord {
	xsMachine *the;
	modMessageDeliver callback;
	void *refcon;
	struct modBLEMessageQueueEntryRecord *entries;
};

#define modBLEMessageQueueEntryPart \
	struct modBLEMessageQueueEntryRecord *next; \
	uint16_t conn_id; \

struct modBLEMessageQueueEntryRecord {
	modBLEMessageQueueEntryPart
};

void modBLEMessageQueueEnqueue(modBLEMessageQueue queue, modBLEMessageQueueEntry entry);
modBLEMessageQueueEntry modBLEMessageQueueDequeue(modBLEMessageQueue queue);
void modBLEMessageQueueConfigure(modBLEMessageQueue queue, xsMachine *the, modMessageDeliver callback, void *refcon);
void modBLEMessageQueueEmpty(modBLEMessageQueue queue);

#endif
