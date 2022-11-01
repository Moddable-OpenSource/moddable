/*---
description: 
flags: [async, module]
---*/

import BLEServer from "bleserver";
import Timer from "timer";
import GAP from "gap";
import GAPWhitelist from "gapwhitelist";

const DEBUG = false;
function debug(string) {
    if (DEBUG) trace(string);
}

let goodAddress = undefined;
let goodAddressType = undefined;

class WhitelistTest extends BLEServer {

    constructor(options) {
        super();

        this.index = 8;
        this.done = false;

        
        this.onTimeout = options.onTimeout;
        this.onComplete = options.onComplete;
    }

    onReady(options={}) {
        this.deviceName = $TESTMC.config.ble.server.deviceName;

        this.whitelist = options.whitelist;
        this.whitelistentry = options.whitelistentry;
        this.whitelisttype = options.whitelisttype;

        GAPWhitelist.clear();

        const advertisingData = {
            flags: 6,
            completeName: this.deviceName,
            manufacturerSpecific: {
                identifier: 0x0e,
                data: [this.index]
            }
        }

        if (this.whitelistentry !== undefined && this.whitelisttype !== undefined) {
            debug(`Adding whitelist entry: ${this.whitelistentry.toString()} ${this.whitelisttype}\n`)
            GAPWhitelist.add(this.whitelistentry, this.whitelisttype);
        }

        if (this.whitelist) {
            debug(`Starting advertising with filtering.\n`);
            this.startAdvertising({
                filterPolicy: GAP.AdvFilterPolicy.WHITELIST_SCANS_CONNECTIONS,
                advertisingData
            });
        } else {
            debug(`Starting advertising without filtering.\n`);
            this.startAdvertising({ advertisingData });
        }

        this.timeout = Timer.set(id => {
            this.onTimeout();
        }, 8_000);
    }

    onConnected(device) {
        this.stopAdvertising();
        goodAddress = device.address;
        goodAddressType = device.addressType;

    }

    onCharacteristicWritten(characteristic, value) {
        if ('location' === characteristic.name) {
            debug(`Got write: ${value}\n`);
            Timer.clear(this.timeout);
            this.disconnect();
            this.onComplete();
        }
    }
}

class TestManager {
    constructor() {
        this.state = 0;
        this.connection = new WhitelistTest({
            onTimeout: this.timeout.bind(this),
            onComplete: this.complete.bind(this)
        })
    }

    complete() {
        if (this.state === 0) {
            debug(`Found good client. Starting whitelist test without entry.\n`);
            this.connection.onReady({
                whitelist: true,
                onTimeout: this.timeout.bind(this),
                onComplete: this.complete.bind(this)
            });
        } else if (this.state === 1) {
            this.done("Got connection with empty whitelist!")
        } else if (this.state === 2) {
            debug(`Success!\n`);
            this.done();
        }
        this.state++;
    }

    timeout() {
        if (this.state === 0) {
            this.done("could not find test device");
        } else if (this.state === 1) {
            debug("Successful timeout on empty whitelist. Starting test with good whitelist.\n");
            this.connection.onReady({
                whitelist: true,
                whitelistentry: goodAddress,
                whitelisttype: goodAddressType
            });
        } else if (this.state === 2) {
            debug("timeout on last step\n");
            this.done("did not find whitelisted connection");
        }
        this.state++;
    }
    done(message){
        this.connection.close();
        $DONE(message);
    }
}

const test = new TestManager();