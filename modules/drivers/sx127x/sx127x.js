/*
 * This is a Moddable JS implementation of a LoRa driver for the ST127x chipset,
 * such as found on the Heltec Wifi LoRa 32 V2 board.  It is a port of the C++
 * ESP-IDF driver (https://github.com/Inteform/esp32-lora-library, has no copyright
 * or license), which is a port of an Arduino C++ LoRa driver (which does have a
 * copyright and license, included herein, https://github.com/sandeepmistry/arduino-LoRa)
 *
 * MIT License
 *
 * Copyright (c) 2021 Christopher W. Midgley
 * Copyright (c) 2016 Sandeep Mistry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * This is a driver that implements support for LoRa using an ST127x chip (over SPI).  This provides for broadcast (no
 * error recovery, no sequencing) packet delivery (similar to UDP) without any enforcement of compliance to regulatory
 * requirements.   The reference implementation was on a Heltec ESP32 Lora V2 dev board.
 *
 * LoRa has strict regional requirements on frequency, per-packet on-air-time as well as total airtime (duty cycle or
 * percent of air time).  You are responsible to ensure you comply with the regulations (serious fines can be levied,
 * region specific, for violations).
 *
 * You are usually limited to a maximum of 400ms of on-air time, so make sure to use a calculator (such as
 * https://avbentem.github.io/airtime-calculator/ttn/au915/222) to determine your per-packet air time. The key settings
 * for controlling per-packet air time are:
 *
 * - Packet size (max 222 bytes)
 * - Bandwidth (higher is less air time, but also lower distance coverage)
 * - Spreading factor (chirp rate, lower is faster, each level is 2x slower)
 * - Coding rate (error correct, small is less correction but smaller packets)
 *
 * Both the sender and the receiver must have the same settings, or the packets will not be received.
 *
 * The other critical factor you must manage is your duty cycle, which is also region specific and very restricted to
 * ensure the airspace can be shared with others.  For example, in the US there is no duty cycle but there is a limit of
 * 400ms of transmission per 20 second period of time.  Therefore, if a packet consumes 200ms of airtime, it can be sent
 * twice each 20 seconds.  In the EU, depending on the channel, there is a 0.1% or 1% duty cycle.  A packet consuming
 * 200ms of airtime on a 1% channel would be limited to one packet each 20 seconds (or one packet every 3.3 minutes on
 * the 0.1% channel).  You are allowed to burst, as long as your overall duty cycle (over an unspecified time period)
 * remains within guidlines. It's your responsibility to research your region specific regulations to ensure compliance.
 *
 * Note that LoRa is not the same as LoRaWAN, which is a network that operates across multiple channels / frequencies
 * and gateways the messages using a standard protocol onto the Internet.  See https://www.thethingsnetwork.org/ for a
 * very popular free network, widely deployed.  This driver could be used as a basis to implement a LoRaWAN client (but
 * not a server, as the ST127x chip can not operate across multiple frequencies) in another project.  Note that these
 * networks often enforce even stricter regulations on airtime usage.
 *
 * A datasheet on the Semtech SX127x chips can be found here: https://cdn-shop.adafruit.com/product-files/3179/sx1276_77_78_79.pdf
 */

import Timer from "timer";
import Registers from "sx127x_registers";

// max retries during init to look for the chip; takes 2ms per lookup so 100 = 200ms delay before failing
const MAX_INIT_RETRIES = 100;

// ST127x transceiver modes
const MODE_SLEEP = 0x00;
const MODE_STDBY = 0x01;
const MODE_TX = 0x03;
const MODE_RX_CONTINUOUS = 0x05;
const MODE_LONG_RANGE_MODE = 0x80;

// ST127x PA configuration
const PA_BOOST = 0x80;

// ST127x IRQ masks
const IRQ_TX_DONE_MASK = 0x08;
const IRQ_PAYLOAD_CRC_ERROR_MASK = 0x20;
const IRQ_RX_DONE_MASK = 0x40;

// RSI values
const RF_MID_BAND_THRESHOLD = 525;
const RSSI_OFFSET_HF_PORT = 157;
const RSSI_OFFSET_LF_PORT = 164;

class LoRa_SX127x {
	#options; // copy of the dictionary passed to the constructor
	#runtimeOptions = {}; // runtime options, from constructor(options) and set(options)
	#spi; // embedded:io/spi instance
	#interruptPin; // embedded:io/digital instance for the interrupt pin (if defined)
	#selectPin; // embedded:io/digital instance for the chip select pin
	#resetPin; // embedded:io/digital instance for the chip reset pin
	#radioTransmitting = false; // true when busy transmitting a packet
	#packetAvailable = false; // true when a read packet is waiting
	#register;

	/**
	 * set up driver for the ST127x LoRa chip (SPI)
	 *
	 * The parameter options is a dictionary and has the following members:
	 * - spi: Device settings dictionary for the LoRa chip on SPI (see below) [REQUIRED]
	 * - reset: Device settings dictionary for the LoRa RESET pin [REQUIRED]
	 * - interrupt: Device settings dictionary for the LoRa INTERRUPT pin [REQUIRED]
	 * - frequency: Frequency for transmission (see #setFrequency, defaults to 915Mhz for US) [OPTIONAL]
	 * - codingRate: Coding rate (default to 5; see #setCodingRate) [OPTIONAL]
	 * - transmitPower: Transmit power (see set_tx_power, defaults to full power or 17) [OPTIONAL]
	 * - crc: true if use CRC, false if not (default enabled or true) [OPTIONAL]
	 * - spreadingFactor: Spreading factor to use (default to 7; see #setSpreadingFactor) [OPTIONAL]
	 * - bandwidth: Bandwidth rate (default to 500; see #setBandwidth) [OPTIONAL]
	 * - gain: Gain (0=automatic gain control, 1-6=specific gain levels) [OPTIONAL]
	 * - implicitHeader: true if implicit, false if explicit packet size (see #setExplicitHeader and #setImplicitHeader) [OPTIONAL]
	 * - implicitHeaderSize: size of header, must be set if implicitHeader is true [OPTIONAL]
	 * - preambleLength: Preamble bytes prior to packet (default to 8) [OPTIONAL]
	 * - syncWord: Sync word to use in preamble (default to 0x12, private networks) [OPTIONAL]
	 * - enableReceiver: The receiver is normally enabled, but can be disabled to save power (default true) [OPTIONAL]
	 * - onReadable: Callback when data arrives for async operations (requires int to be defined) [OPTIONAL]
	 * - onWritable: Callback when data arrives for async operations (requires int to be defined) [OPTIONAL]
	 *
	 * Typical members of spi:
	 * - io: device.io.* of the device (always device.io.SPI)
	 * - clock: pin number of the clock (SCK)
	 * - in: pin number of SPI in (MISO)
	 * - out: pin number of SPI out (MOSI)
	 * - select: pin number for chip select (SS or CS) [REQUIRED: does not use SPI driver for chip select]
	 * - active: state of select pin when active; optional: defaults to 0)
	 * - hz: frequency of SPI in hz
	 * - port: SPI port
	 * - mode: SPI mode
	 *
	 * Typical members of reset (defines the RESET pin):
	 * - io: Usually device.io.DEVICE
	 * - pin: Pin number to reset the ST127x chip
	 * - mode: Usually device.io.Digital.Output
	 *
	 * Typical members of interrupt (defines the INTERRUPT pin):
	 * - io: Usually device.io.DEVICE
	 * - pin: Pin number to receive interrupts from the ST127x chip
	 * - mode: Usually device.io.Digital.Input
	 */
	constructor(options) {
		this.#options = options;

		// attach to SPI
		this.#spi = new options.spi.io(options.spi);

		// define the interrupt pin
		this.#interruptPin = new options.interrupt.io({
			...options.interrupt,
			edge: options.interrupt.io.Rising,
			onReadable: this.#interruptCompletion.bind(this),
		});

		// set up pin for chip select, and set to not selected
		this.#selectPin = new options.select.io(options.select);
		this.#selectPin.write(1);

		// set up pin to reset the LoRa chip
		this.#resetPin = new options.reset.io(options.reset);
		
		this.#register = new Registers(this.#spi, this.#selectPin);

		// reset hardware (takes about 12ms)
		this.#reset();

		// check version of chip, retrying for a while; this basically ensures we have access to our ST127x chip
		let i = 0;
		while (i++ < MAX_INIT_RETRIES) {
			const version = this.#register.VERSION;
			if ((version === 0x12) || (version === 0x13)) break;
			Timer.delay(2);
		}
		if (i >= MAX_INIT_RETRIES + 1) throw Error("Not Found");

		// set up the chip
		this.#sleep();
		this.#register.FIFO_RX_BASE_ADDR = 0;
		this.#register.FIFO_TX_BASE_ADDR = 0;

		// set operational values based on options settings and defaults
		this.configure({
			frequency: options.frequency ?? 915,
			transmitPower: options.transmitPower ?? 17,
			crc: options.crc ?? true,
			implicitHeader: options.implicitHeader ?? false,
			spreadingFactor: options.spreadingFactor ?? 7,
			bandwidth: options.bandwidth ?? 500,
			codingRate: options.codingRate ?? 5,
			preambleLength: options.preambleLength ?? 8,
			syncWord: options.syncWord ?? 0x12,
			enableReceiver: options.enableReceiver ?? true,
			gain: options.gain ?? 0
		});

		// set the radio into receive or idle based on the options.enableReceiver mode
		this.#updateRadioMode();
	}

	/**
	 * Change the operational parameters of the LoRa chip.  These options can all be originally set on the constructor options
	 * dictionary, but can be adjusted here at runtime.
	 *
	 * Members include (see constructor for documentation)
	 * - frequency
	 * - transmitPower
	 * - crc
	 * - implicitHeader
	 * - implicitHeaderSize (must be set if implicitHeader is true)
	 * - spreadingFactor
	 * - bandwidth
	 * - codingRate
	 * - preambleLength
	 * - syncWord
	 * - enableReceiver
	 * - gain
	 *
	 * @param options Dictionary of options to set
	 */
	configure(options) {
		const runtimeOptions = this.#runtimeOptions;

		if (undefined !== options.frequency && options.frequency !== runtimeOptions.frequency)
			this.#setFrequency(options.frequency);

		if (undefined !== options.transmitPower && options.transmitPower !== runtimeOptions.transmitPower)
			this.#setTransmitPower(options.transmitPower);

		if (undefined !== options.crc && options.crc !== runtimeOptions.crc) this.#setCRC(options.crc);

		if (undefined !== options.implicitHeader && options.implicitHeader !== runtimeOptions.implicitHeader)
			if (options.implicitHeader) this.#setImplicitHeader(runtimeOptions.implicitHeaderSize);
			else this.#setExplicitHeader(runtimeOptions.implicitHeaderSize);

		if (undefined !== options.spreadingFactor && options.spreadingFactor !== runtimeOptions.spreadingFactor)
			this.#setSpreadingFactor(options.spreadingFactor);

		if (undefined !== options.bandwidth && options.bandwidth !== runtimeOptions.bandwidth)
			this.#setBandwidth(options.bandwidth);

		if (undefined !== options.codingRate && options.codingRate !== runtimeOptions.codingRate)
			this.#setCodingRate(options.codingRate);

		if (undefined !== options.preambleLength && options.preambleLength !== runtimeOptions.preambleLength)
			this.#setPreambleLength(options.preambleLength);

		if (undefined !== options.syncWord && options.syncWord !== runtimeOptions.syncWord)
			this.#setSyncWord(options.syncWord);

		if (undefined !== options.enableReceiver) this.#setEnableReceiver(options.enableReceiver);

		if (undefined !== options.gain) this.#setGain(options.gain);
	}

	/**
	 * Get the current operational values in use for the LoRa chip (from constructor and set operation).  Responses match
	 * dictionary requirements for the configure() call.
	 *
	 * @returns Dictionary of all values
	 */
	get() {
		return { ...this.#runtimeOptions };
	}

	/**
	 * Shutdown hardware and release associated resources
	 */
	close() {
		if (this.#spi) this.#sleep();

		this.#spi?.close();
		this.#selectPin?.close();
		this.#resetPin?.close();
		this.#interruptPin?.close();

		this.#spi = undefined;
		this.#selectPin = undefined;
		this.#resetPin = undefined;
		this.#interruptPin = undefined;

		this.#radioTransmitting = false;
	}

	/**
	 * Set data format for write and read.  Just verifies the value is "buffer" as that is the only format supported.
	 */
	set format(value) {
		if ("buffer" !== value) throw new RangeError;
	}

	/**
	 * Return the format used for read/write (always "buffer")
	 */
	get format() {
		return "buffer";
	}

	/**
	 * Read a received packet. Normally called after received() indicates a packet is available.
	 *
	 * @param arg Buffer to fill with data, or Number to allocate a buffer.  Data must fit or packet is dropped.
	 * @return If Buffer passed, returns bytes received.  If Number, returns ArrayBuffer with data.  undefined when no packet.
	 */
	read(arg) {
		// is a packet waiting?
		if (!this.#packetAvailable) return undefined;

		// determine packet size (based on implicit vs. explict headers)
		const len = this.#runtimeOptions.implicitHeader ? this.#register.PAYLOAD_LENGTH : this.#register.RX_NB_BYTES;

		// set up our buffer (if arg is number, allocate it; otherwise assume it's a buffer)
		let buf, result;
		if (undefined === arg) {
			buf = new Uint8Array(len);
			result = buf.buffer;
		}
		else {
			result = len;
			if (arg instanceof ArrayBuffer)
				buf = new Uint8Array(arg);
			else if (arg instanceof Uint8Array)
				buf = arg; 
			else
				throw new Error("unsupported");				// to do: dataview, Int8Array, SharedArrayBuffer

			//  ensure sufficient room in our buffer
			if (len > buf.byteLength) {
				trace("Pkt too big");
				return undefined;
			}
		}

		// pause the radio
		this.#idle();

		// transfer the packet from the radio
		this.#register.FIFO_ADDR_PTR = this.#register.FIFO_RX_CURRENT_ADDR;
		for (let i = 0; i < len; i++) buf[i] = this.#register.FIFO;

		// enable the radio if wanted based on options
		this.#packetAvailable = false;
		this.#updateRadioMode();

		// return buffer (if dynamically allocated) or length received (if buffer provided)
		return result;
	}

	/**
	 * Send a packet.  If onWritable is set, will callback when the write is complete and the driver is ready to
	 * accept another packet.
	 *
	 * @param buf Data to be sent (buffer, such as ArrayBuffer)
	 * @param size Size of data (optional, if not supplied uses buf.byteLength)
	 */
	write(buf, size = buf.byteLength) {
		if (this.#radioTransmitting) throw Error("Xmit Overlap");

		// prepare radio for transmission
		this.#idle();
		this.#register.FIFO_ADDR_PTR = 0;

		// track that we are transmitting
		this.#radioTransmitting = true;

		// send data to radio
		if (!ArrayBuffer.isView(buf))
			buf = new Uint8Array(buf);
		for (let i = 0; i < size; ++i) this.#register.FIFO = buf[i];
		this.#register.PAYLOAD_LENGTH = size;

		// tell the chip to interrupt on transmit complete
		this.#register.DIO_MAPPING_1 = 0x40;

		// tell radio to start tranmission, and let completion be handled by the ISR
		this.#register.OP_MODE = MODE_LONG_RANGE_MODE | MODE_TX;
	}

	#writeCompletion() {
		// reset chip back to our standard radio mode (idle or receive)
		this.#register.IRQ_FLAGS = IRQ_TX_DONE_MASK;
		this.#updateRadioMode();

		// track that we are done transmitting, and handle the completion
		this.#radioTransmitting = false;
		this.#options.onWritable?.();
	}

	/**
	 * Returns statistics information about the radio communications
	 *
	 * Returns an object containing:
	 *
	 * rssi: The Received Signal Strength Indicator of the radio.  RSSI range is -120dBm to 0dBm, with 0db being perfect
	 * (and never occurs).  RSSI of -30dBm is very good, whereas -100dBm is quite poor.
	 *
	 * packetRssi: Same as Rssi, but for the last received packet
	 *
	 * snr: SNR (Signal to Noise Ratio), or the ratio of the signal and the radio noise floor.  An SNR greater than 0
	 * means the received signal operates above the noise floor, and less than means the signal is below the noise
	 * floor.  Typical values are -20dB and +10dB, though around -20dB LoRa is unable to decode the signal.
	 *
	 * packetFrequencyError: The frequency error of the received packet in Hz. The frequency error is the
	 * frequency offset between the receiver centre frequency and that of an incoming LoRa signal
	 *
	 * @returns Object with statistics (see above)
	 */
	statistics() {
		return {
			rssi: this.#rssi(),
			packetRssi: this.#packetRssi(),
			snr: this.#snr(),
			packetFrequencyError: this.#packetFrequencyError(),
		};
	}

	/***
	 *** PRIVATE METHODS
	 ***/

	/**
	 * SNR (Signal to Noise Ratio), or the ratio of the signal and the radio noise floor.  An SNR greater than 0 means
	 * the received signal operates above the noise floor, and less than means the signal is below the noise floor.
	 * Typical values are -20dB and +10dB, though around -20dB LoRa is unable to decode the signal.
	 *
	 * @returns Signal to Noise Ratio
	 */
	#snr() {
		return this.#register.PKT_SNR_VALUE * 0.25;
	}

	/**
	 * The Received Signal Strength Indicator of the last received packet.  RSSI range is -120dBm to 0dBm, with 0db
	 * being perfect (and never occurs).  RSSI of -30dBm is very good, whereas -100dBm is quite poor.
	 *
	 * @returns RSSI for last received packet
	 */
	#packetRssi() {
		return (
			this.#register.PKT_RSSI_VALUE -
			(this.#runtimeOptions.frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT)
		);
	}

	/**
	 * The Received Signal Strength Indicator of the radio.  RSSI range is -120dBm to 0dBm, with 0db being perfect
	 * (and never occurs).  RSSI of -30dBm is very good, whereas -100dBm is quite poor.
	 *
	 * @returns RSSI
	 */
	#rssi() {
		return (
			this.#register.RSSI_VALUE -
			(this.#runtimeOptions.frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT)
		);
	}

	/**
	 * Returns the frequency error of the received packet in Hz. The frequency error is the
	 * frequency offset between the receiver centre frequency and that of an incoming LoRa signal
	 *
	 * @returns Frequency error
	 */
	#packetFrequencyError() {
		let freqError = 0;
		freqError = this.#register.FREQ_ERROR_MSB & 0x7;
		freqError <<= 8;
		freqError += this.#register.FREQ_ERROR_MID;
		freqError <<= 8;
		freqError += this.#register.FREQ_ERROR_LSB;

		if (this.#register.FREQ_ERROR_MSB & 0x8) {
			// Sign bit is on
			freqError -= 524288; // B1000'0000'0000'0000'0000
		}

		let fXtal = 32e6; // FXOSC: crystal oscillator (XTAL) frequency (2.5. Chip Specification, p. 14)
		let fError = ((freqError * (1 << 24)) / fXtal) * (this.#runtimeOptions.bandwidth / 500000.0); // p. 37

		return fError;
	}
	/**
	 * Handle the interrupt completion for both transmit and receive
	 */
	#interruptCompletion() {
		if (this.#radioTransmitting) this.#writeCompletion();
		else {
			// reset IRQ
			let irq = this.#register.IRQ_FLAGS;
			this.#register.IRQ_FLAGS = irq;

			// is a packet waiting and without CRC error?
			if ((irq & IRQ_RX_DONE_MASK) != 0 && !(irq & IRQ_PAYLOAD_CRC_ERROR_MASK)) {
				this.#packetAvailable = true;

				const count = this.#runtimeOptions.implicitHeader ? this.#register.PAYLOAD_LENGTH : this.#register.RX_NB_BYTES; 
				this.#options.onReadable?.(count);
			}
		}
	}

	/**
	 * Configure power level for transmission
	 *
	 * @param level 2-17, from least to most power
	 */
//	#OLDsetTransmitPower(level) {
//		// RF9x module uses PA_BOOST pin
//		if (level < 2) level = 2;
//		else if (level > 17) level = 17;
//
//		this.#runtimeOptions.transmitPower = level;
//
//		this.#writeRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
//	}

	#setTransmitPower(level) {
		if (level > 17) {
			if (level > 20) level = 20;

			// subtract 3 from level, so 18 - 20 maps to 15 - 17
			level -= 3;

			// High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
			this.#register.PA_DAC = 0x87;
			this.#setOCP(140);
		} else {
			if (level < 2) {
				level = 2;
			}
			//Default value PA_HF/LF or +17dBm
			this.#register.PA_DAC = 0x84;
			this.#setOCP(100);
		}

		this.#register.PA_CONFIG = PA_BOOST | (level - 2);
	}

	/**
	 * Set over current protection (OCP) on radio
	 *
	 * @param mA milliamps current
	 */
	#setOCP(mA) {
		let ocpTrim = 27;

		if (mA <= 120) {
			ocpTrim = (mA - 45) / 5;
		} else if (mA <= 240) {
			ocpTrim = (mA + 30) / 10;
		}

		this.#register.OCP = 0x20 | (0x1f & ocpTrim);
	}

	/**
	 * Set gain (0=auto-gain control (AGC), 1-6 for specific gain control)
	 *
	 * @param gain Gain (0-6) with 0=AGC
	 */
	#setGain(gain) {
		// check allowed range and stash the value
		if (gain > 6) gain = 6;
		this.#runtimeOptions.gain = gain;

		// set to standby
		this.#idle();

		// set gain
		if (gain == 0) {
			// if gain = 0, enable AGC
			this.#register.MODEM_CONFIG_3 = 0x04;
		} else {
			// disable AGC
			this.#register.MODEM_CONFIG_3 = 0x00;

			// clear Gain and set LNA boost
			this.#register.LNA = 0x03;

			// set gain
			this.#register.LNA = this.#register.LNA | (gain << 5);
		}
	}

	/**
	 * Set the LDO (Low Datarate Optimizing) flag when less than 16ms
	 */
	#setLdoFlag() {
		// Section 4.1.1.5
		const symbolDuration = 1000 / (this.#runtimeOptions.bandwidth / (1 << this.#runtimeOptions.spreadingFactor));

		// Section 4.1.1.6
		const ldoOn = (symbolDuration > 16) ? 1 : 0;

		// set bit 3 of REG_MODEM_CONFIG_3 to ldoOn
		this.#register.MODEM_CONFIG_3 = (this.#register.MODEM_CONFIG_3 & 0xf7) | (ldoOn << 3);
	}
 
	/**
	 * Set carrier frequency (mandiated by regional regulatory requiremenst)
	 *
	 * @param frequency Frequency in MHz (such as 915 for us, or 868 for EU)
	 */
	#setFrequency(frequency) {
		this.#runtimeOptions.frequency = frequency;

		let frf = (BigInt(frequency * 1e6) << 19n) / 32000000n;
		this.#register.FRF_MSB = Number((frf >> 16n) & 0xffn);
		this.#register.FRF_MID = Number((frf >> 8n) & 0xffn);
		this.#register.FRF_LSB = Number(frf & 0xffn);
	}

	/**
	 * Set spreading factor
	 *
	 * @param sf Spreading factor (6-12, lower=faster, higher=more reliable)
	 */
	#setSpreadingFactor(sf) {
		this.#runtimeOptions.spreadingFactor = this.#clamp(sf, 6, 12);

		if (this.#runtimeOptions.spreadingFactor == 6) {
			this.#register.DETECTION_OPTIMIZE = 0xc5;
			this.#register.DETECTION_THRESHOLD = 0x0c;
		} else {
			this.#register.DETECTION_OPTIMIZE = 0xc3;
			this.#register.DETECTION_THRESHOLD = 0x0a;
		}

		this.#register.MODEM_CONFIG_2 =
			this.#register.MODEM_CONFIG_2 & 0x0f | ((this.#runtimeOptions.spreadingFactor << 4) & 0xf0);

		this.#setLdoFlag();
	}

	/**
	 * Set bandwidth (bit rate); regulated by region (for example, US allows 500 and 125, EU allows 250 and 125)
	 *
	 * @param bandwidth Bandwidth (in kHz (up to 500))
	 */
	#setBandwidth(sbw) {
		this.#runtimeOptions.bandwidth = sbw;

		let bw;

		if (sbw <= 7.8) bw = 0;
		else if (sbw <= 10.4) bw = 1;
		else if (sbw <= 15.6) bw = 2;
		else if (sbw <= 20.8) bw = 3;
		else if (sbw <= 31.25) bw = 4;
		else if (sbw <= 41.7) bw = 5;
		else if (sbw <= 62.5) bw = 6;
		else if (sbw <= 125) bw = 7;
		else if (sbw <= 250) bw = 8;
		else bw = 9;
		this.#register.MODEM_CONFIG_1 = (this.#register.MODEM_CONFIG_1 & 0x0f) | (bw << 4);

		this.#setLdoFlag();
	}

	/**
	 * Set coding rate
	 *
	 * @param denominator 5-8 (denominator for the coding rate 4/x, lower=less air time, higher=more reliable)
	 */
	#setCodingRate(denominator) {
		this.#runtimeOptions.codingRate = denominator;

		const cr = this.#clamp(denominator, 5, 8) - 4;
		this.#register.MODEM_CONFIG_1 = (this.#register.MODEM_CONFIG_1 & 0xf1) | (cr << 1);
	}

	/**
	 * Set the size of preamble, used in front of packet for packet detection.  Rarely needs to change, especially in
	 * peer-to-peer configurations.
	 *
	 * @param length Preamble length in symbols
	 */
	#setPreambleLength(length) {
		this.#runtimeOptions.preambleLength = length;
		this.#register.PREAMBLE_MSB = (length >> 8) & 0xff;
		this.#register.PREAMBLE_LSB = length & 0xff;
	}

	/**
	 * Change radio sync word (used in preamble to identify).  Defaults to 0x12 for private networks, can be changed
	 * such as to 0x34 for public networks (such as LoRaWAN)
	 * )
	 * @param sw New sync word to use
	 */
	#setSyncWord(sw) {
		this.#runtimeOptions.syncWord = sw;
		this.#register.SYNC_WORD = sw;
	}

	/**
	 * Change radio usage of CRC (append/verify CRC if true)
	 *
	 * @param enable True to enable CRC, false to disable
	 */
	#setCRC(enable) {
		this.#runtimeOptions.crc = enable;

		if (enable) this.#register.MODEM_CONFIG_2 |= 0x04;
		else this.#register.MODEM_CONFIG_2 &= 0xfb;
	}

	/**
	 * Configure explicit header mode, which allows for variable packet sizes where the size is included in the packet header
	 * (takes additional air time for size, but allows for variable airtime packets)
	 */
	#setExplicitHeader() {
		this.#runtimeOptions.implicitHeader = false;
		this.#register.MODEM_CONFIG_1 &= 0xfe;
	}

	/**
	 * Configure implicit header mode, which sets an implicit packet size and does not include packet size in the header
	 * (reducing air time slightly).  All packets must be of the exact same size.
	 *
	 * @param size Size of the packets.
	 */
	#setImplicitHeader(size) {
		this.#runtimeOptions.implicitHeader = true;
		this.#register.MODEM_CONFIG_1 |= 0x01;
		this.#register.PAYLOAD_LENGTH = size;
	}

	/**
	 * The radio can be disabled to save power.  When a new packet is transmitted it is automatically turned on and off
	 * needed.  If you disable the radio, receive callbacks will not operate until it is enabled again.
	 *
	 * @param enable true to enable the radio, false to turn it off
	 */
	#setEnableReceiver(enable) {
		this.#runtimeOptions.enableReceiver = enable;

		if (!this.#radioTransmitting) this.#updateRadioMode();
	}

	/**
	 * Adjust the radio to turn on/off the receiver
	 */
	#updateRadioMode() {
		if (this.#runtimeOptions.enableReceiver) {
			// enable receive
			this.#register.OP_MODE = MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS;
			// tell the chip to interrupt on receive complete
			this.#register.DIO_MAPPING_1 = 0x00;
		} else {
			// idle the radio
			this.#idle();
		}
	}

	/**
	 * Idle the radio
	 */
	#idle() {
		this.#register.OP_MODE = MODE_LONG_RANGE_MODE | MODE_STDBY;
	}

	/**
	 * Perform physical reset on the Lora chip
	 */
	#reset() {
		this.#resetPin.write(0);
		Timer.delay(10);
		this.#resetPin.write(1);
		Timer.delay(10);
	}

	/**
	 * Sets the radio transceiver in sleep mode (low power consumption)
	 */
	#sleep() {
		this.#register.OP_MODE = MODE_LONG_RANGE_MODE | MODE_SLEEP;
	}

	/**
	 * Helper method to clamp a value between min and max
	 *
	 * @param num Number of clamp
	 * @param min Min value of num
	 * @param max Max value of num
	 * @return Number guaranteed to be min <= num <= max
	 */
	#clamp(num, min, max) {
		return Math.min(Math.max(num, min), max);
	}

	/**
	 * Internal debugging method that shows all st127x registers to the debugger
	 *
	 * @params msg Optional message to include with the dump
	 */
	// #lora_dump_registers(msg = "") {
	// 	trace(`STM127x register dump: ${msg}`);
	// 	trace("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	// 	for (let i = 0; i < 0x80; i++) {
	// 		trace(`${this.#read_reg(i).toString(16).padStart(2, "0")} `);
	// 		if ((i & 0x0f) == 0x0f) trace("\n");
	// 	}
	// }
}

export default LoRa_SX127x;
