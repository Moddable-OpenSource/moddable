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

const KEYINFO = Object.freeze({
    VOLUME_UP:      {HID: {page: 0x0C, id: 0xE9}, report: 0b00000001},
    VOLUME_DOWN:    {HID: {page: 0x0C, id: 0xEA}, report: 0b00000010},
    MUTE:           {HID: {page: 0x0C, id: 0x7F}, report: 0b00000100},
    BACK:           {HID: {page: 0x0C, id: 0xB6}, report: 0b00001000},
    FORWARD:        {HID: {page: 0x0C, id: 0xB5}, report: 0b00010000},
    PLAY:           {HID: {page: 0x0C, id: 0xB0}, report: 0b00100000},
    PLAYPAUSE:      {HID: {page: 0x0C, id: 0xCD}, report: 0b01000000},
    SHUFFLE:        {HID: {page: 0x0C, id: 0xB9}, report: 0b10000000}
}, true);

function getMask(hidValue) {
    for (let k in KEYINFO)
        if (hidValue == KEYINFO[k].HID.id)
            return KEYINFO[k].report;

    return undefined;
}

class HIDMedia {
    constructor() {
        this.report = Uint8Array.from([0]);
    }

    canHandle(options) {
        if (options.hidCode?.page === 0x0C && getMask(options.hidCode.id) != undefined)
            return true;
        else
            return false;
    }

    onKeyUp(options) {
        this.report[0] &= ~getMask(options.hidCode.id);
    }

    onKeyDown(options) {
        this.report[0] |= getMask(options.hidCode.id);
    }
}

export {HIDMedia, KEYINFO};