declare module "embedded:io/bluetoothle/central" {
    interface Advertisement {
        address: string;
        rssi?: number;
        get name(): string | undefined;
        get services(): string[] | undefined;
        get manufacturerData(): { manufacturer: number, data: ArrayBuffer } | undefined;
        get(adType: number): ArrayBuffer | undefined;
    }

    interface GAPClientOptions {
        target?: any;
        services?: string[]
        onError?: (this: GAPClient, error: Error) => void;
        onReadable?: (this: GAPClient, count: number) => void;
    }

    class GAPClient {
        constructor(options: GAPClientOptions);
        target: any;
        close(): void;
        read(): Advertisement;
    }

    interface GATTClientNotifiedValue extends ArrayBuffer {
        handle: number
    }

    interface GATTClientWriteOptions {
        response?: boolean
    }

    interface GATTSecurityOptions {
        bond?: boolean,
        authenticate?: boolean,
        ioCapabilities?: "none" | "display" | "numbers" | "display+numbers" | "display+confirm",
        immediate?: boolean
    }

    interface GATTSecurityState {
        encrypted: boolean,
        authenticated: boolean,
        bonded: boolean,
        keySize: number
    }

    interface GATTClientOptions {
        address: string,
        target?: any;
        security?: GATTSecurityOptions;
        onReady?: (this: GATTClient, ) => void;
        onPasskey?: (this: GATTClient, action: string, data: number | ArrayBuffer | undefined) => void;
        onSecured?: (this: GATTClient, state: GATTSecurityState) => void;
        onError?: (this: GATTClient, error: Error) => void;
        onReadable?: (this: GATTClient, count: number) => void;
    }

    interface GATTClientService {
        get uuid(): string;
    }

    interface GATTClientCharacteristic {
        get uuid(): string;
        get handle(): number;
        get properties(): number;
    }

    interface GATTClientDescriptor {
        get uuid(): string;
    }

    class GATTClient {
        constructor(options: GATTClientOptions);
        target: any;

        close(callback?: (error?: Error) => void): void;

        getPrimaryServices(callback: (error: Error | null, services: GATTClientService[]) => void): void;
        getPrimaryServices(options: object, callback: (error: Error | null, services: GATTClientService[]) => void): void;

        getCharacteristics(service: GATTClientService, callback: (error: Error | null, characteristics: GATTClientCharacteristic[]) => void): void;
        getCharacteristics(service: GATTClientService, options: object, callback: (error: Error | null, characteristics: GATTClientCharacteristic[]) => void): void;

        getDescriptors(characteristic: GATTClientCharacteristic, callback: (error: Error | null, descriptors: GATTClientDescriptor[]) => void): void;
        getDescriptors(characteristic: GATTClientCharacteristic, options: object, callback: (error: Error | null, descriptors: GATTClientDescriptor[]) => void): void;

        read(what: GATTClientCharacteristic | GATTClientDescriptor, callback: (error: Error | null, value: ArrayBuffer) => void): void;
        read(what: GATTClientCharacteristic | GATTClientDescriptor, options: object, callback: (error: Error | null, value: ArrayBuffer) => void): void;
        read() : undefined | GATTClientNotifiedValue;

        write(what: GATTClientCharacteristic | GATTClientDescriptor, value: BufferLike, callback?: (error: Error | null) => void): void;
        write(what: GATTClientCharacteristic | GATTClientDescriptor, value: BufferLike, options: GATTClientWriteOptions, callback?: (error: Error | null) => void): void;

        subscribe(characteristic: GATTClientCharacteristic, callback?: (error?: Error) => void) : void;
        unsubscribe(characteristic: GATTClientCharacteristic, callback?: (error?: Error) => void) : void;

        replyToPasskey(action: string, data?: number | ArrayBuffer) : void;

        get maximumWrite(): number;

        store(item: GATTClientService | GATTClientCharacteristic | GATTClientDescriptor): ArrayBuffer;
        restore(item: BufferLike): GATTClientService | GATTClientCharacteristic | GATTClientDescriptor;

        static readonly properties: {
            authenticatedSignedWrites: 64;
            broadcast: 1;
            indicate: 32;
            notify: 16;
            read: 2;
            reliableWrite: 128;
            write: 8;
            writeWithOutResponse: 4;
        };
    }
 
    export { GATTClient, GAPClient };
}
