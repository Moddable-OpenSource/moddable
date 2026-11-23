/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import Poco from "commodetto/Poco";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";
import Timer from "timer";
import AudioIn from "embedded:io/audio/in";
import AudioOut from "embedded:io/audio/out";

const render = new Poco(screen, {displayListLength: 2048});
const font = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));
const blue = render.makeColor(0, 35, 80);
const green = render.makeColor(0, 75, 65);
const white = render.makeColor(255, 255, 255);
const yellow = render.makeColor(255, 200, 0);
const audioFormat = {sampleRate: 16000, channels: 1, bitsPerSample: 16};
const cueSamples = audioFormat.sampleRate / 5;
const recordSamples = audioFormat.sampleRate;
const silenceSamples = audioFormat.sampleRate / 4;
const samples = new Int16Array(cueSamples + recordSamples + silenceSamples);
const pcm = new Uint8Array(samples.buffer);

let background = blue;
let status = "Press A to record";
let audio;

function drawScreen() {
	const {powerSource} = power.status;
	const lines = ["M5StopWatch", status, `PMIC source 0x${powerSource.toString(16)}`];
	render.begin();
	render.fillRectangle(background, 0, 0, render.width, render.height);
	for (let i = 0; i < lines.length; i++) {
		const line = lines[i];
		const x = (render.width - render.getTextWidth(line, font)) >> 1;
		render.drawText(line, font, white, x, (render.height >> 1) - 60 + 60 * i);
	}
	render.end();
}

function record() {
	let recording = false;
	let recorded = 0;
	const input = audio = new AudioIn({
		...audioFormat,
		onReadable(size) {
			if (!recording) {
				this.read(pcm.subarray(0, size));
				return;
			}
			const count = Math.min(size >> 1, recordSamples - recorded);
			if (count) {
				const start = cueSamples + recorded;
				this.read(pcm.subarray(start * 2, (start + count) * 2));
				recorded += count;
			}
		}
	});
	status = "Warming microphone";
	drawScreen();
	input.start();
	Timer.set(() => {
		recording = true;
		status = "Recording";
		drawScreen();
	}, 1200);
	Timer.set(() => {
		input.stop();
		input.close();
		audio = undefined;

		let peak = 0;
		for (let i = 0; i < recorded; i++)
			peak = Math.max(peak, Math.abs(samples[cueSamples + i]));
		trace(`MIC samples=${recorded} peak=${peak}\n`);

		if (recorded) {
			play(recorded, peak);
		}
		else {
			status = "No mic samples. A to retry";
			drawScreen();
		}
	}, 2500);
}

function play(recorded, peak) {
	const recordingEnd = cueSamples + recorded;
	const end = recordingEnd + silenceSamples;
	const gain = peak ? Math.min(256, 12000 / peak) : 1;
	for (let i = 0; i < cueSamples; i++)
		samples[i] = 12000 * Math.sin(2 * Math.PI * 660 * i / audioFormat.sampleRate);
	for (let i = cueSamples; i < recordingEnd; i++)
		samples[i] *= gain;
	samples.fill(0, recordingEnd, end);
	let offset = 0;
	const output = audio = new AudioOut({
		...audioFormat,
		onWritable(size) {
			const count = Math.min(size >> 1, end - offset);
			if (!count)
				return;
			this.write(pcm.subarray(offset * 2, (offset + count) * 2));
			offset += count;
			if (offset === end) {
				Timer.set(() => {
					output.close();
					audio = undefined;
					status = `Mic peak ${peak}. A to retry`;
					drawScreen();
					trace(`SPEAKER samples=${recorded}\n`);
				}, 1000);
			}
		}
	});
	status = "Playing recording";
	drawScreen();
	output.volume = 1;
	output.start();
}

const buttonA = new device.peripheral.button.A({
	onPush() {
		if (this.pressed && !audio) {
			trace("BUTTON A\n");
			record();
		}
	}
});
const buttonB = new device.peripheral.button.B({
	onPush() {
		if (!this.pressed)
			return;
		background = (background === blue) ? green : blue;
		drawScreen();
		trace(`BUTTON B PMIC ${JSON.stringify(power.status)}\n`);
	}
});

const touch = new device.sensor.Touch();
Timer.repeat(() => {
	const points = touch.sample();
	if (!points?.length)
		return;
	for (const {x, y} of points) {
		const left = Math.max(0, Math.min(render.width - 12, x - 6)) & ~1;
		const top = Math.max(0, Math.min(render.height - 12, y - 6)) & ~1;
		render.begin(left, top, 12, 12);
		render.fillRectangle(yellow, left, top, 12, 12);
		render.end();
		trace(`TOUCH ${x},${y}\n`);
	}
}, 50);

drawScreen();
trace(`DISPLAY ${render.width}x${render.height}\n`);
trace(`TOUCH controller=${touch.identification.model}\n`);
trace(`PMIC ${JSON.stringify(power.status)}\n`);
