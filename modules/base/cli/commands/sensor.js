/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import CLI from "cli";

CLI.install(async function(command, parts) {
	if ("sensor" !== command)
		return false;

	if (!parts.length)
		throw new Error("missing open/close/configure/sample");

	switch (parts.shift().toLowerCase()) {
		case "open":
			if (this.sensor)
				throw new Error("already open");
			this.suspend();
			try {
				this.sensor = new ((await import(parts[0]))).default({});
			}
			catch(e) {
				this.line(e.toString());
			}
			this.resume();
			break;

		case "close":
			if (!this.sensor)
				throw new Error("none open");
			this.sensor.close();
			delete this.sensor;
			break;

		case "configure":
			if (!this.sensor)
				throw new Error("none open");
			if (parts.length < 1)
				throw new Error("missing property");
			if (parts.length < 2)
				throw new Error("missing value");
			this.sensor.configure({[parts[0]]: parts[1]});
			break;

		case "sample":
			if (!this.sensor)
				throw new Error("none open");
			let sample = this.sensor.sample();
			for (let property in sample)
				this.line(property, ": ", sample[property])
			break;

		case "help":
			this.line("sensor open name - opens sensor module of 'name'");
			this.line("sensor close - closes active sensor");
			this.line("sensor configure property value - sets configuration property to value");
			this.line("sensor sample - reads sample and displays result");
			break;

		default:
			this.line("unknown sensor option");
			break;

	}
	return true;
});
