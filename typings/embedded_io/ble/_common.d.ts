declare module "embedded:io/bluetoothle/_common" {
    export interface GATTSecurityOptions {
        bond?: boolean,
        authenticate?: boolean,
        ioCapabilities?: "none" | "display" | "numbers" | "display+numbers" | "display+confirm",
        immediate?: boolean
    }
    export interface GATTSecurityState {
        encrypted: boolean,
        authenticated: boolean,
        bonded: boolean,
        keySize: number
    }
}
