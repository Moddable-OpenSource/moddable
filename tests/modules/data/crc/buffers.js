/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/956
flags: [module]
---*/

import { CRC8, CRC16 } from "crc";

let buffer1 = Uint8Array.of(0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef);
let buffer2 = Uint8Array.of(0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10);
let buffer3 = new Uint8Array([...buffer1, ...buffer2]);

let crc;
let crc8 = new CRC8(0x9B, 0x00, true, true, 0x00);	// CRC-8/WCDMA

crc8.checksum(buffer1);
crc = crc8.checksum(buffer2);
assert.sameValue(0x88, crc, "CRC8");

crc8.reset();
assert.sameValue(crc, crc8.checksum(buffer3), "CRC8 multiple buffers");

let crc16 = new CRC16(0x1021, 0xFFFF, true, true, 0xFFFF); // CRC-16/X-25

crc16.checksum(buffer1);
crc = crc16.checksum(buffer2);
assert.sameValue(0x3F82, crc, "CRC16");

crc16.reset();
assert.sameValue(crc, crc16.checksum(buffer3), "CRC16 multiple buffers");

