/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import Timer from "timer";
import GAPWhitelist from "gapwhitelist";
import GAP from "gap";

$TESTMC.timeout($TESTMC.config.ble.client.timeout * 3, "target BLE peripheral not found");

const LOOKING = 0;
const BLOCKED = 1;
const WHITELISTED = 2;

class Scanner extends BLEClient {
    onReady() {
        GAPWhitelist.clear();
        GAPWhitelist.add("00:00:00:00:00");
        this.state = LOOKING;
        this.start();
    }
    start(whitelist=false) {
        trace(`start scanning: ${whitelist}\n`);
        if (whitelist) {
            this.startScanning({ filterPolicy: GAP.ScanFilterPolicy.WHITELIST, duplicates: true });
        } else {
            this.startScanning({duplicates: true});
        }
    }
    stop() {
        trace("stop scanning\n");
        this.stopScanning();
    }
    onDiscovered(device) {
        trace(`found: ${device?.address.toString()} ${this.state}\n`);
        if (device?.scanResponse?.completeName === $TESTMC.config.ble.client.targetDevice) {
            this.stop();
            switch (this.state) {
                case LOOKING:
                    this.goodAddress = device.address;
                    this.state = BLOCKED;
                    this.start(true);
                    this.timer = Timer.set(() => {
                        this.stop();
                        this.state = WHITELISTED;
                        
                        GAPWhitelist.add(this.goodAddress);
                        this.start(true);
                    }, 5_000);
                    break;
                case BLOCKED:
                    Timer.clear(this.timer);
                    this.close();
                    $DONE("found blocked device in scan");
                    break;
                case WHITELISTED:
                    trace(`3\n`);
                    this.close();
                    $DONE();
            }
        }
    }
}

let scanner = new Scanner();