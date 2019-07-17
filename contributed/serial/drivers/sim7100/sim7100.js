/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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


import Timer from "timer";
import Time from "time";
import Serial from "serial";

const STATE_START = 0;
const STATE_IDLE = 1;
const STATE_STOP_STATE_ENGINE = 2;
const STATE_ENGINE_STOPPED = 3;
const STATE_WAIT_READY = 4;
const STATE_READY = 5;
const STATE_WAIT_RESPONSE = 6;

const STATE_GPS_DISABLED = 10;
const STATE_GPS_ENABLE_COMPLETE = 12;
const STATE_GPS_RECEIVE = 13;
const STATE_GPS_NO_DATA = 14;
const STATE_GPS_TRY_AGAIN = 15;

const STATE_SMS_SEND_MSG_DEST = 20;
const STATE_SMS_SEND_MSG = 21;
const STATE_SMS_SEND_COMPLETE = 22;

const SMS_TERMINATOR = "";

const WAIT_FOR_LOCK_MS = 30000;		// if gps returns ,,,,, (no fix) try again

function calcDecDeg(str) {
    let parts = str.split('.');
    let deg = parseInt(parts[0].slice(0,parts[0].length-2));
    let min = parseFloat(parts[0].slice(-2) + "." + parts[1]);
    return (deg + (min / 60)).toFixed(8);
}


export default class SIM7100 {

	constructor(dictionary) {
		if (undefined !== dictionary) {
			this.timeoutMS = dictionary.timeout;
		}
		else {
			this.timeoutMS = 1000;
		}
		this.gps = { seq: 0, interval: 0 };
		this.lastGPS = this.gps.seq;
		this.gpsON = 0;
		this.lastState = STATE_IDLE;
		this.state = STATE_START;
		this.nextState = STATE_START;
		this.serial = new Serial();
		this.serial.sim7100 = this;
		this.stateCycleMS = 100;
		this.timedOutMS = 0;
		this.respHandler = [
			{ str: "+CGPSINFO: ", fn: this.handleCGPSINFO, data: this },
			{ str: "+CGPS: ", fn: this.handleCGPS, data: this },
			{ str: "+CSPN: ", fn: this.handleCSPN, data: this },
		];

		this.serial.onDataReceived = function(data, len) {
			let s = data.toString();
			if (s.slice(0,5) == "ERROR") {
				if (undefined !== this.sim7100.onError)
					this.sim7100.onError(s);
				else
					this.sim7100.state = this.sim7100.nextState;
				return;
			}
			if (s.slice(0,2) == "OK") {
				if (undefined !== this.sim7100.onOK)
					this.sim7100.onOK();
				else
					this.sim7100.state = this.sim7100.nextState;
				return;
			}
			this.sim7100.respHandler.forEach(function(handler) {
					if (s.startsWith(handler.str)) {
						let parts = s.slice(handler.str.length).split(',');
						handler.fn(s, parts, handler.data);
					}
				});
			this.sim7100.onError = function(r) {
				trace("ERROR\n");
			}

		}
	}		// end constructor

	start(cycle = 100) {
		this.stateCycleMS = cycle;
		this.startStateEngine();
	}

	stop() {
		this.stopStateEngine();
	}

	reset() {
		this.serial.write("AT+CRESET\r");
	}

	sendSMS(dest, msg) {
		this.lastState = this.state;
		this.state = STATE_WAIT_RESPONSE;
		this.nextState = STATE_SMS_SEND_MSG_DEST;
		this.smsDestination = dest;
		this.outgoingMsg = msg;
		this.serial.write("AT+CMGF=1\r");
	}

	disableGPS() {
		this.state = STATE_WAIT_RESPONSE;
		this.nextState = STATE_GPS_DISABLED;
		if (undefined !== this.gps.interval)
			this.serial.write(`AT+CGPSINFO=0\r`);
		this.serial.write("AT+CGPS=0\r");
	}

	enableGPS() {
		this.state = STATE_WAIT_RESPONSE;
 		this.nextState = STATE_GPS_ENABLE_COMPLETE;
		this.serial.write("AT+CGPS?\r");
	}

	getGPS(interval) {
		if (undefined === interval) {
			// one shot
			this.serial.write(`AT+CGPSINFO\r`);
			return;
		}

		this.gps.interval = interval;
		this.serial.write(`AT+CGPSINFO=${interval}\r`);
	}

	handleCGPSINFO(s, parts, data) {
		if (parts[0].length == 0 || parts[1].length == 0) {
			if (0 == data.gps.interval) {
				trace("failed to get GPS data\n");
				data.onOK = function() {
					data.onOK = undefined;
					data.state = STATE_GPS_NO_DATA;
				}
			}
			return;
		}
		data.gps.seq++;
		data.gps.lat = calcDecDeg(parts[0]);
		if (parts[1].toLowerCase() == "s")
			data.gps.lat = -data.gps.lat;
		data.gps.lon = calcDecDeg(parts[2]);
		if (parts[3].toLowerCase() == "w")
			data.gps.lon = -data.gps.lon;
		data.gps.alt = parseFloat(parts[6]);
		data.gps.speed = parseFloat(parts[7]);
		data.gps.course = parseFloat(parts[8]);
		data.gps.date = {
			day: 	parseInt(parts[4].slice(0,2), 10),
			month:	parseInt(parts[4].slice(2,4), 10),
			year:	parseInt(parts[4].slice(-2), 10) 	};
		data.gps.time = {
			hour:	parseInt(parts[5].slice(0,2), 10),
			min:	parseInt(parts[5].slice(2,4), 10),
			sec:	parseFloat(parts[5].slice(-4))		};
	}

	handleCGPS(r, parts, data) {
//		let parts = r.slice(7).split(',');
		if (parts[0] == 0) {		// GPS is off
			data.onOK = function() {
				data.onOK = undefined;
				data.serial.write("AT+CGPS=1\r");
			}
		}
	}

	handleCSPN(r, parts, data) {
//		let parts = r.slice(7),split(',');
		data.serviceProvider = parts[0];
		data.state = data.nextState;
	}

	ledBrightness(val) {
    	if (val < 0) val = 0;
	    if (val > 8) val = 8;

		this.serial.write(`AT+CLEDITST=${val}\r`);
	}

	stopStateEngine() {
		this.state = STATE_STOP_STATE_ENGINE;
	}

	startStateEngine() {
		this.state = STATE_START;
		this.serial.poll({terminators: "\r\n", trim: 1});
		this.timer = Timer.repeat(() => {
			switch (this.state) {
				case STATE_START:
					this.state = STATE_WAIT_READY;
					this.nextState = STATE_READY;
					this.serial.write("AT+CSPN?\r");
					this.timedOutMS = this.timeoutMS + Time.ticks;
					break;
				case STATE_WAIT_READY:
					if (Time.ticks > this.timedOutMS)
						throw new Error("no response from sim7100 module");
					break;
				case STATE_READY:
					this.state = STATE_IDLE;
					this.nextState = STATE_IDLE;
					if (undefined !== this.onReady) {
						this.onReady();
					}
				case STATE_IDLE:
					break;
				case STATE_WAIT_RESPONSE:
					break;
				case STATE_GPS_DISABLED:
					this.state = STATE_IDLE;
					this.gpsON = 0;
					if (undefined !== this.onGPSDisabled) {
						this.onGPSDisabled(this);
					}
					break;
				case STATE_GPS_ENABLE_COMPLETE:
					this.gpsON = 1;
					if (undefined !== this.onGPSEnabled)
						this.onGPSEnabled(this);
					this.state = STATE_GPS_RECEIVE;
					this.nextState = STATE_GPS_RECEIVE;
					break;
				case STATE_GPS_NO_DATA:
					this.timedOutMS = WAIT_FOR_LOCK_MS + Time.ticks;
					this.state = STATE_GPS_TRY_AGAIN;
					break;
				case STATE_GPS_TRY_AGAIN:
					if (Time.ticks > this.timedOutMS) {
						this.getGPS();
						this.state = STATE_GPS_RECEIVE;
						this.nextState = STATE_GPS_RECEIVE;
					}
					break;
				case STATE_GPS_RECEIVE:
					if (this.gps.seq != this.lastGPS) {
						if (undefined !== this.onGPSChanged)
							this.onGPSChanged(this.gps);
						else {
							trace(`${this.gps.seq} GPS: lat:${this.gps.lat}, lon:${this.gps.lon}\n`);
							trace(`  ${this.gps.date.month}/${this.gps.date.day}/${this.gps.date.year} @ ${this.gps.time.hour}:${this.gps.time.min}:${this.gps.time.sec}\n`);
							trace(`  Alt: ${this.gps.alt} m  -- Speed: ${this.gps.speed} -- Course: ${this.gps.course}˚\n`);
						}
						this.lastGPS = this.gps.seq;
					}
					break;
				case STATE_SMS_SEND_MSG_DEST:
					this.state = STATE_SMS_SEND_MSG;
					this.serial.write(`AT+CMGS="${this.smsDestination}"\r`);
					break;
				case STATE_SMS_SEND_MSG:
					this.state = STATE_WAIT_RESPONSE;
					this.nextState = STATE_SMS_SEND_COMPLETE;
					this.serial.write(this.outgoingMsg + SMS_TERMINATOR);
					break;
				case STATE_SMS_SEND_COMPLETE:
					if (undefined !== this.onSMSSendComplete)
						this.onSMSSendComplete(this);
					this.state = this.lastState;
					break;
				case STATE_STOP_STATE_ENGINE:
					this.state = STATE_ENGINE_STOPPED;
					this.nextState = STATE_ENGINE_STOPPED;
					if (this.gpsON)
						this.disableGPS();
					break;
				case STATE_ENGINE_STOPPED:
					Timer.clear(this.timer);
					this.state = STATE_IDLE;
					if (undefined !== this.onStopped)
						this.onStopped();
					break;
			}
		}, this.stateCycleMS);
	}

}

