declare module "embedded:io/bluetoothle/peripheral" {
    import type { Buffer } from "embedded:io/_common";
    import type { GATTSecurityOptions, GATTSecurityState } from "embedded:io/bluetoothle/_common";

    interface GATTServerOptions {
        mtu?: number;
        services: GATTServerService[];
        security?: GATTSecurityOptions;
        onReady?(this: GATTServer): void;
        onConnect?(this: GATTServer, connection: GATTServerConnection): void;
        onDisconnect?(this: GATTServer, connection: GATTServerConnection): void;
        onPasskey?(this: GATTServer, connection: GATTServerConnection, action: "input" | "display" | "compareNumber" | "outOfBand", data?: number | ArrayBuffer): void;
        onSecured?(this: GATTServer, connection: GATTServerConnection, state: GATTSecurityState): void;
        onWarning?(this: GATTServer, message: string): void;
    }

    interface GATTServerConnection {
        close(): void;
        notify(characteristic: GATTServerCharacteristic, value: ArrayBuffer, callback?: (error?: Error) => void): void;
        replyToPasskey(action: "input" | "compareNumber" | "outOfBand", value: number | boolean | ArrayBuffer): void;
        get maxinumWrite(): number;
    }

    interface GATTServerService {
        uuid: string;
        characteristics?: GATTServerCharacteristic[];
    }

    interface GATTServerCharacteristic {
        uuid: string;
        properties?: number;
        value?: Buffer;
        descriptors?: GATTServerDescriptor[];

        onRead?(this: GATTServerCharacteristic, connection: GATTServerConnection): Buffer;
        onWrite?(this: GATTServerCharacteristic, value: ArrayBuffer, connection: GATTServerConnection): void;
        onSubscribe?(this: GATTServerCharacteristic, connection: GATTServerConnection): void;
        onUnsubscribe?(cthis: GATTServerCharacteristic, onnection: GATTServerConnection): void;
    }

    interface GATTServerDescriptor {
        uuid: string;
        value?: Buffer;

        onRead?(connection: GATTServerConnection): Buffer;
        onWrite?(value: ArrayBuffer, connection: GATTServerConnection): void;
    }

    interface GATTServerAdvertisingRecords {
        name?: string;
        services?: string[];
        manufacturerData?: { manufacturer: number, data: Buffer };
        flags?: number;
        [ADType: number]: Buffer;
    }

    class GATTServer {
        constructor(options: GATTServerOptions);

        close?(): void;
        addService(service: GATTServerService): void;
        deleteService(service: GATTServerService): void;
        startAdvertising(scan: GATTServerAdvertisingRecords, response?: GATTServerAdvertisingRecords): void;
        stopAdvertising(): void;

        static readonly properties: {
            authenticatedSignedWrites: 64;
            broadcast: 1;
            indicate: 32;
            notify: 16;
            read: 2;
            readAuthenticated: 1026;
            readEncrypted: 514;
            reliableWrite: 128;
            subscribeAuthenticated: 65552;
            subscribeEncrypted: 32784;
            write: 8;
            writeAuthenticated: 8200;
            writeEncrypted: 4104;
            writeWithOutResponse: 4;
        };
        static readonly advertise: {
            limitedDiscoverable: 1;
            generalDiscoverable: 2;
            bleOnly: 4;
            dualModeController: 8;
            dualModeHost: 16;
        }
    }

    export { GATTServer };
}
