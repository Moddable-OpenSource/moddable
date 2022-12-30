/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

export default class {
   buffer16 = new Uint8Array(2);

   constructor(spi, select) {
      this.spi = spi;
      this.select = select;
   }
   getUint8(register) {
      const buffer = this.buffer16;

      this.select.write(0);
      buffer[0] = register;
      buffer[1] = 0xff;
      this.spi.transfer(buffer);
      this.select.write(1);
      return buffer[1];
   }
   setUint8(register, value) {
      const buffer = this.buffer16;

      this.select.write(0);
      buffer[0] = 0x80 | register;
      buffer[1] = value;
      this.spi.write(buffer);
      this.select.write(1);
   }
   get FIFO() {
      return this.getUint8(0);
   }
   set FIFO(value) {
      this.setUint8(0, value);
   }
   get OP_MODE() {
      return this.getUint8(1);
   }
   set OP_MODE(value) {
      this.setUint8(1, value);
   }
   get pad0() {
      return new Uint8Array(this.buffer, this.byteOffset + 2, 4);
   }
   set pad0(value) {
      for (let i = 0, j = 2; i < 4; i++, j += 1)
         this.setUint8(j, value[i]);
   }
   get FRF_MSB() {
      return this.getUint8(6);
   }
   set FRF_MSB(value) {
      this.setUint8(6, value);
   }
   get FRF_MID() {
      return this.getUint8(7);
   }
   set FRF_MID(value) {
      this.setUint8(7, value);
   }
   get FRF_LSB() {
      return this.getUint8(8);
   }
   set FRF_LSB(value) {
      this.setUint8(8, value);
   }
   get PA_CONFIG() {
      return this.getUint8(9);
   }
   set PA_CONFIG(value) {
      this.setUint8(9, value);
   }
   get pad1() {
      return this.getUint8(10);
   }
   set pad1(value) {
      this.setUint8(10, value);
   }
   get OCP() {
      return this.getUint8(11);
   }
   set OCP(value) {
      this.setUint8(11, value);
   }
   get LNA() {
      return this.getUint8(12);
   }
   set LNA(value) {
      this.setUint8(12, value);
   }
   get FIFO_ADDR_PTR() {
      return this.getUint8(13);
   }
   set FIFO_ADDR_PTR(value) {
      this.setUint8(13, value);
   }
   get FIFO_TX_BASE_ADDR() {
      return this.getUint8(14);
   }
   set FIFO_TX_BASE_ADDR(value) {
      this.setUint8(14, value);
   }
   get FIFO_RX_BASE_ADDR() {
      return this.getUint8(15);
   }
   set FIFO_RX_BASE_ADDR(value) {
      this.setUint8(15, value);
   }
   get FIFO_RX_CURRENT_ADDR() {
      return this.getUint8(16);
   }
   set FIFO_RX_CURRENT_ADDR(value) {
      this.setUint8(16, value);
   }
   get pad2() {
      return this.getUint8(17);
   }
   set pad2(value) {
      this.setUint8(17, value);
   }
   get IRQ_FLAGS() {
      return this.getUint8(18);
   }
   set IRQ_FLAGS(value) {
      this.setUint8(18, value);
   }
   get RX_NB_BYTES() {
      return this.getUint8(19);
   }
   set RX_NB_BYTES(value) {
      this.setUint8(19, value);
   }
   get pad3() {
      return new Uint8Array(this.buffer, this.byteOffset + 20, 5);
   }
   set pad3(value) {
      for (let i = 0, j = 20; i < 5; i++, j += 1)
         this.setUint8(j, value[i]);
   }
   get PKT_SNR_VALUE() {
      return this.getUint8(25);
   }
   set PKT_SNR_VALUE(value) {
      this.setUint8(25, value);
   }
   get PKT_RSSI_VALUE() {
      return this.getUint8(26);
   }
   set PKT_RSSI_VALUE(value) {
      this.setUint8(26, value);
   }
   get RSSI_VALUE() {
      return this.getUint8(27);
   }
   set RSSI_VALUE(value) {
      this.setUint8(27, value);
   }
   get pad4() {
      return this.getUint8(28);
   }
   set pad4(value) {
      this.setUint8(28, value);
   }
   get MODEM_CONFIG_1() {
      return this.getUint8(29);
   }
   set MODEM_CONFIG_1(value) {
      this.setUint8(29, value);
   }
   get MODEM_CONFIG_2() {
      return this.getUint8(30);
   }
   set MODEM_CONFIG_2(value) {
      this.setUint8(30, value);
   }
   get pad5() {
      return this.getUint8(31);
   }
   set pad5(value) {
      this.setUint8(31, value);
   }
   get PREAMBLE_MSB() {
      return this.getUint8(32);
   }
   set PREAMBLE_MSB(value) {
      this.setUint8(32, value);
   }
   get PREAMBLE_LSB() {
      return this.getUint8(33);
   }
   set PREAMBLE_LSB(value) {
      this.setUint8(33, value);
   }
   get PAYLOAD_LENGTH() {
      return this.getUint8(34);
   }
   set PAYLOAD_LENGTH(value) {
      this.setUint8(34, value);
   }
   get pad6() {
      return new Uint8Array(this.buffer, this.byteOffset + 35, 3);
   }
   set pad6(value) {
      for (let i = 0, j = 35; i < 3; i++, j += 1)
         this.setUint8(j, value[i]);
   }
   get MODEM_CONFIG_3() {
      return this.getUint8(38);
   }
   set MODEM_CONFIG_3(value) {
      this.setUint8(38, value);
   }
   get pad7() {
      return this.getUint8(39);
   }
   set pad7(value) {
      this.setUint8(39, value);
   }
   get FREQ_ERROR_MSB() {
      return this.getUint8(40);
   }
   set FREQ_ERROR_MSB(value) {
      this.setUint8(40, value);
   }
   get FREQ_ERROR_MID() {
      return this.getUint8(41);
   }
   set FREQ_ERROR_MID(value) {
      this.setUint8(41, value);
   }
   get FREQ_ERROR_LSB() {
      return this.getUint8(42);
   }
   set FREQ_ERROR_LSB(value) {
      this.setUint8(42, value);
   }
   get pad8() {
      return new Uint8Array(this.buffer, this.byteOffset + 43, 6);
   }
   set pad8(value) {
      for (let i = 0, j = 43; i < 6; i++, j += 1)
         this.setUint8(j, value[i]);
   }
   get DETECTION_OPTIMIZE() {
      return this.getUint8(49);
   }
   set DETECTION_OPTIMIZE(value) {
      this.setUint8(49, value);
   }
   get pad9() {
      return new Uint8Array(this.buffer, this.byteOffset + 50, 5);
   }
   set pad9(value) {
      for (let i = 0, j = 50; i < 5; i++, j += 1)
         this.setUint8(j, value[i]);
   }
   get DETECTION_THRESHOLD() {
      return this.getUint8(55);
   }
   set DETECTION_THRESHOLD(value) {
      this.setUint8(55, value);
   }
   get pad10() {
      return this.getUint8(56);
   }
   set pad10(value) {
      this.setUint8(56, value);
   }
   get SYNC_WORD() {
      return this.getUint8(57);
   }
   set SYNC_WORD(value) {
      this.setUint8(57, value);
   }
   get pad11() {
      return new Uint8Array(this.buffer, this.byteOffset + 58, 6);
   }
   set pad11(value) {
      for (let i = 0, j = 58; i < 6; i++, j += 1)
         this.setUint8(j, value[i]);
   }
   get DIO_MAPPING_1() {
      return this.getUint8(64);
   }
   set DIO_MAPPING_1(value) {
      this.setUint8(64, value);
   }
   get pad12() {
      return this.getUint8(65);
   }
   set pad12(value) {
      this.setUint8(65, value);
   }
   get VERSION() {
      return this.getUint8(66);
   }
   set VERSION(value) {
      this.setUint8(66, value);
   }
   get pad13() {
      return new Uint8Array(this.buffer, this.byteOffset + 67, 10);
   }
   set pad13(value) {
      for (let i = 0, j = 67; i < 10; i++, j += 1)
         this.setUint8(j, value[i]);
   }
   get PA_DAC() {
      return this.getUint8(77);
   }
   set PA_DAC(value) {
      this.setUint8(77, value);
   }
}

/*

View classes generated by https://phoddie.github.io/compileDataView on Fri Nov 26 2021 14:26:25 GMT-0800 (PST) from the following description:

struct Registers {
	uint8_t FIFO; // 0x00;
	uint8_t OP_MODE; // 0x01;
	uint8_t pad0[4];
	uint8_t FRF_MSB; // 0x06;
	uint8_t FRF_MID; // 0x07;
	uint8_t FRF_LSB; // 0x08;
	uint8_t PA_CONFIG; // 0x09;
	uint8_t pad1;
	uint8_t OCP; // 0x0b;
	uint8_t LNA; // 0x0c;
	uint8_t FIFO_ADDR_PTR; // 0x0d;
	uint8_t FIFO_TX_BASE_ADDR; // 0x0e;
	uint8_t FIFO_RX_BASE_ADDR; // 0x0f;
	uint8_t FIFO_RX_CURRENT_ADDR; // 0x10;
	uint8_t pad2;
	uint8_t IRQ_FLAGS; // 0x12;
	uint8_t RX_NB_BYTES; // 0x13;
	uint8_t pad3[5];
	uint8_t PKT_SNR_VALUE; // 0x19;
	uint8_t PKT_RSSI_VALUE; // 0x1a;
	uint8_t RSSI_VALUE; // 0x1b;
	uint8_t pad4;
	uint8_t MODEM_CONFIG_1; // 0x1d;
	uint8_t MODEM_CONFIG_2; // 0x1e;
	uint8_t pad5;
	uint8_t PREAMBLE_MSB; // 0x20;
	uint8_t PREAMBLE_LSB; // 0x21;
	uint8_t PAYLOAD_LENGTH; // 0x22;
	uint8_t pad6[3];
	uint8_t MODEM_CONFIG_3; // 0x26;
	uint8_t pad7;
	uint8_t FREQ_ERROR_MSB; // 0x28;
	uint8_t FREQ_ERROR_MID; // 0x29;
	uint8_t FREQ_ERROR_LSB; // 0x2a;
	uint8_t pad8[6];
	uint8_t DETECTION_OPTIMIZE; // 0x31;
	uint8_t pad9[5];
	uint8_t DETECTION_THRESHOLD; // 0x37;
	uint8_t pad10;
	uint8_t SYNC_WORD; // 0x39;
	uint8_t pad11[6];
	uint8_t DIO_MAPPING_1; // 0x40;
	uint8_t pad12;
	uint8_t VERSION; // 0x42;
	uint8_t pad13[10];
	uint8_t PA_DAC; // 0x4d;
};

*/
