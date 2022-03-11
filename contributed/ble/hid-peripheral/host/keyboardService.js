/*
 * Copyright (c) 2022 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import BLEServer from "bleserver";
import { uuid } from "btutils";
import {HIDKeyboard, HID_MODIFIERS} from "hidkeyboard";
import {HIDMedia, KEYINFO} from "hidmedia";

const domain = "BLEKB";

class KeyboardService extends BLEServer {
    constructor(options) {
        super(options);

        this.keyboard = new HIDKeyboard();
        this.media = new HIDMedia();
        this.bound = false;

        this.boundCallback = options.onKeyboardBound;
        this.unboundCallback = options.onKeyboardUnbound;
    }
    onReady() {
        this.deviceName = "Moddable";
        this.securityParameters = { encryption: true, bonding: true };

        this.keyboardReportCharacteristic = undefined;
        this.mediaReportCharacteristic = undefined;
        this.onDisconnected();
    }
    onConnected() {
        this.stopAdvertising();
    }
    onDisconnected() {
        this.unboundCallback?.();
        this.mediaReportCharacteristic = undefined;
        this.keyboardReportCharacteristic = undefined;
        this.startAdvertising({
            advertisingData: { flags: 6, completeName: this.deviceName, incompleteUUID16List:[uuid`1812`, uuid`180f`], appearance: 961 }
        });
    }
    onCharacteristicNotifyEnabled(characteristic) {
        if ("media_input_report" == characteristic.name) {
            this.mediaReportCharacteristic = characteristic;
            trace(`media report bound by request\n`);
            if (!this.bound) {
                this.bound = true;
                this.boundCallback?.();
            }
        } else if ("keyboard_input_report" == characteristic.name) {
            this.keyboardReportCharacteristic = characteristic;
            trace(`keyboard report bound by request\n`);
            if (!this.bound) {
                this.bound = true;
                this.boundCallback?.();
            }
        } else if ("battery" == characteristic.name) {
            this.batteryCharacteristic = characteristic;
            this.notifyValue(this.batteryCharacteristic, 85);
        } else {
            trace(`request to bind characteristic: ${JSON.stringify(characteristic)}\n`);
        }
    }
    onCharacteristicRead(characteristic){
        switch (characteristic.name){
            case "media_input_report":
                return this.media.report;
            case "keyboard_input_report":
                return this.keyboard.report;
            case "control_point":
                return [0,0];
            default:
                trace(`unhandled read of characteristic: ${characteristic.name}\n`);
                break;
        }
    }
    notifyKeyboard() {
        if (this.keyboardReportCharacteristic)
            this.notifyValue(this.keyboardReportCharacteristic, this.keyboard.report);
        else
            trace(`not connected ${this.keyboard.report}`);
    }
    notifyMedia() {
        if (this.mediaReportCharacteristic)
            this.notifyValue(this.mediaReportCharacteristic, this.media.report);
        else
            trace(`not connected: ${this.media.report}\n`);
    }
    onKeyUp(options) {
        if (this.media.canHandle(options)) {
            this.media.onKeyUp(options);
            this.notifyMedia();
        }
        
        if (this.keyboard.canHandle(options)) {
            this.keyboard.onKeyUp(options);
            this.notifyKeyboard();
        }
    }
    onKeyDown(options) {
        if (this.media.canHandle(options)) {
            this.media.onKeyDown(options);
            this.notifyMedia();
        }
        
        if (this.keyboard.canHandle(options)) {
            this.keyboard.onKeyDown(options);
            this.notifyKeyboard();
        }
    }
    onKeyTap(options) {
        this.onKeyDown(options);
        this.onKeyUp(options);
    }
}

export { KeyboardService as default, KEYINFO, HID_MODIFIERS };