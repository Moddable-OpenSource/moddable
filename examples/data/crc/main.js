/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
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

// crccalc.com

import { CRC8, CRC16, CRC32 } from "crc";

const crc8_tests = [
	{ poly: 0x07, init: 0x00, res: 0xF4, refIn: false, refOut: false, xorOut: 0x00, name: "CRC-8" },
	{ poly: 0x9B, init: 0xFF, res: 0xDA, refIn: false, refOut: false, xorOut: 0x00, name: "CRC-8/CDMA2000" },
	{ poly: 0x39, init: 0x00, res: 0x15, refIn: true,  refOut: true,  xorOut: 0x00, name: "CRC-8/DARC" },
	{ poly: 0xD5, init: 0x00, res: 0xBC, refIn: false, refOut: false, xorOut: 0x00, name: "CRC-8/DVB-S2" },
	{ poly: 0x1D, init: 0xFF, res: 0x97, refIn: true,  refOut: true,  xorOut: 0x00, name: "CRC-8/EBU" },
	{ poly: 0x1D, init: 0xFD, res: 0x7E, refIn: false, refOut: false, xorOut: 0x00, name: "CRC-8/I-CODE" },
	{ poly: 0x07, init: 0x00, res: 0xA1, refIn: false, refOut: false, xorOut: 0x55, name: "CRC-8/ITU" },
	{ poly: 0x31, init: 0x00, res: 0xA1, refIn: true,  refOut: true,  xorOut: 0x00, name: "CRC-8/MAXIM" },
	{ poly: 0x07, init: 0xFF, res: 0xD0, refIn: true,  refOut: true,  xorOut: 0x00, name: "CRC-8/ROHC" },
	{ poly: 0x9B, init: 0x00, res: 0x25, refIn: true,  refOut: true,  xorOut: 0x00, name: "CRC-8/WCDMA" }
];

const crc16_tests = [
	{ poly: 0x1021, init: 0xFFFF, res: 0x29B1, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/CCITT-FALSE" },
	{ poly: 0x8005, init: 0x0000, res: 0xBB3D, refIn: true,  refOut: true,  xorOut: 0x0000, name: "CRC-16/ARC" },
	{ poly: 0x1021, init: 0x1D0F, res: 0xE5CC, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/ARG-CCITT" },
	{ poly: 0x8005, init: 0x0000, res: 0xFEE8, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/BUYPASS" },
	{ poly: 0xC867, init: 0xFFFF, res: 0X4C06, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/CDMA2000" },
	{ poly: 0x8005, init: 0x800D, res: 0x9ECF, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/DDS-110" },
	{ poly: 0x0589, init: 0x0000, res: 0x007E, refIn: false, refOut: false, xorOut: 0x0001, name: "CRC-16/DECT-R" },
	{ poly: 0x0589, init: 0x0000, res: 0x007F, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/DECT-X" },
	{ poly: 0x3D65, init: 0x0000, res: 0xEA82, refIn: true,  refOut: true,  xorOut: 0xFFFF, name: "CRC-16/DNP" },
	{ poly: 0x3D65, init: 0x0000, res: 0xC2B7, refIn: false, refOut: false, xorOut: 0xFFFF, name: "CRC-16/EN-13757" },
	{ poly: 0x1021, init: 0xFFFF, res: 0xD64E, refIn: false, refOut: false, xorOut: 0xFFFF, name: "CRC-16/GENIBUS" },
	{ poly: 0x8005, init: 0x0000, res: 0x44C2, refIn: true,  refOut: true,  xorOut: 0xFFFF, name: "CRC-16/MAXIM" },
	{ poly: 0x1021, init: 0xFFFF, res: 0x6F91, refIn: true,  refOut: true,  xorOut: 0x0000, name: "CRC-16/MCRF4XX" },
	{ poly: 0x1021, init: 0xB2AA, res: 0x63D0, refIn: true,  refOut: true,  xorOut: 0x0000, name: "CRC-16/RIELLO" },
	{ poly: 0x8BB7, init: 0x0000, res: 0xD0DB, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/T10-DIF" },
	{ poly: 0xA097, init: 0x0000, res: 0x0FB3, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/TELEDISK" },
	{ poly: 0x1021, init: 0x89EC, res: 0x26B1, refIn: true,  refOut: true,  xorOut: 0x0000, name: "CRC-16/TMS37157" },
	{ poly: 0x8005, init: 0xFFFF, res: 0xB4C8, refIn: true,  refOut: true,  xorOut: 0xFFFF, name: "CRC-16/USB" },
	{ poly: 0x1021, init: 0xC6C6, res: 0xBF05, refIn: true,  refOut: true,  xorOut: 0x0000, name: "CRC-A" },
	{ poly: 0x1021, init: 0x0000, res: 0x2189, refIn: true,  refOut: true,  xorOut: 0x0000, name: "CRC-16/KERMIT" },
	{ poly: 0x8005, init: 0xFFFF, res: 0x4B37, refIn: true,  refOut: true,  xorOut: 0x0000, name: "CRC-16/MODBUS" },
	{ poly: 0x1021, init: 0xFFFF, res: 0x906E, refIn: true,  refOut: true,  xorOut: 0xFFFF, name: "CRC-16/X-25" },
	{ poly: 0x1021, init: 0x0000, res: 0x31C3, refIn: false, refOut: false, xorOut: 0x0000, name: "CRC-16/XMODEM" }
];

const crc32_tests = [
	{ poly: 0x04C11DB7, init: 0xFFFFFFFF, res: 0xCBF43926, refIn: true,  refOut: true,  xorOut: 0xFFFFFFFF, name: "CRC-32" },
	{ poly: 0x04C11DB7, init: 0xFFFFFFFF, res: 0xFC891918, refIn: false, refOut: false, xorOut: 0xFFFFFFFF, name: "CRC-32/BZIP2" },
	{ poly: 0x1EDC6F41, init: 0xFFFFFFFF, res: 0xE3069283, refIn: true,  refOut: true,  xorOut: 0xFFFFFFFF, name: "CRC-32C" },
	{ poly: 0xA833982B, init: 0xFFFFFFFF, res: 0x87315576, refIn: true,  refOut: true,  xorOut: 0xFFFFFFFF, name: "CRC-32D" },
	{ poly: 0x04C11DB7, init: 0xFFFFFFFF, res: 0x0376E6E7, refIn: false, refOut: false, xorOut: 0x00000000, name: "CRC-32/MPEG-2" },
	{ poly: 0x04C11DB7, init: 0x00000000, res: 0x765E7680, refIn: false, refOut: false, xorOut: 0xFFFFFFFF, name: "CRC-32/POSIX" },
	{ poly: 0x814141AB, init: 0x00000000, res: 0x3010BF7F, refIn: false, refOut: false, xorOut: 0x00000000, name: "CRC-32Q" },
	{ poly: 0x04C11DB7, init: 0xFFFFFFFF, res: 0x340BC6D9, refIn: true,  refOut: true,  xorOut: 0x00000000, name: "CRC-32/JAMCRC" },
	{ poly: 0x000000AF, init: 0x00000000, res: 0xBD0BE338, refIn: false, refOut: false, xorOut: 0x00000000, name: "CRC-32/XFER" }
];

const testData = Uint8Array.of(49, 50, 51, 52, 53, 54, 55, 56, 57);

crc8_tests.forEach(data => {
	const test = new CRC8(
				data.poly,
				data.init,
				data.refIn,
				data.refOut,
				data.xorOut);
	const res = test.checksum(testData);
	if (res === data.res)
		trace(`Success: ${data.name}\n`);
	else
		trace(`Failure: ${data.name} expected ${data.res} vs. ${res}\n`);
});

crc16_tests.forEach(data => {
	const test = new CRC16(
				data.poly,
				data.init,
				data.refIn,
				data.refOut,
				data.xorOut);
	const res = test.checksum(testData);
	if (res === data.res)
		trace(`Success: ${data.name}\n`);
	else
		trace(`Failure: ${data.name} expected ${data.res} vs. ${res}\n`);
});

crc32_tests.forEach(data => {
	const test = new CRC32(
				data.poly,
				data.init,
				data.refIn,
				data.refOut,
				data.xorOut);
	const res = test.checksum(testData);
	if (res === data.res)
		trace(`Success: ${data.name}\n`);
	else
		trace(`Failure: ${data.name} expected ${data.res} vs. ${res}\n`);
});
