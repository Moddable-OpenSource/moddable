/*
 * Copyright (c) 2022-2026  Moddable Tech, Inc., Satoshi Tanaka
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
import parseBMP from "commodetto/parseBMP";
import Resource from "Resource";
import loadJPEG from "commodetto/loadJPEG";
import Timer from "timer";
import ILI9341 from "ili9341";
import config from "mc/config";

const STARTING_STEPS = 123;
const GOAL = 200;
const SHAKEVALUE = 3;

const render = new Poco(screen, {rotation: config.rotation});

const black = render.makeColor(0, 0, 0);
const white = render.makeColor(255, 255, 255);
const lightGray = render.makeColor(200, 200, 200);
const darkGray = render.makeColor(55, 55, 55);
const barColor = render.makeColor(52, 207, 235);

const steps = parseBMP(new Resource("steps-alpha.bmp"));
const progressOverlay = parseBMP(new Resource("progress-overlay-alpha.bmp"));

const smallFont = parseBMF(new Resource("OpenSans-Semibold-16.bf4"));
const bigFont = parseBMF(new Resource("OpenSans-Semibold-28.bf4"));

const goalStr = `Goal: ${GOAL}`;
const goalStrOffset = (render.width-render.getTextWidth(goalStr, smallFont))/2;

let backgroundColor, foregroundColor, stepColor, stepCount = STARTING_STEPS;
let lightMode = true;

function redraw() {
	if (lightMode) {
		backgroundColor = white;
		foregroundColor = black;
		stepColor = lightGray;
	} else {
		backgroundColor = black;
		foregroundColor = white;
		stepColor = darkGray;
	}
	render.begin();
		render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
		let offset = (render.width-32)/2;
		render.drawGray(steps, stepColor, 20, 7);
		render.fillRectangle(stepColor, 10, render.height-17, render.width-20, render.height-17);
		const fraction = stepCount / GOAL;
		render.fillRectangle(barColor, 10, render.height-17, (render.width-20)*fraction, render.height-17);
		render.drawGray(progressOverlay, backgroundColor, 0, render.height-17);
		offset = (render.width-(render.getTextWidth(stepCount, bigFont)))/2;
		render.drawText(stepCount, bigFont, foregroundColor, offset, 6);
		render.drawText(goalStr, smallFont, foregroundColor, goalStrOffset, 41);
	render.end();	

}

const imu = new device.sensor.IMU();
let oldData = {x: 0, y: 0, z: 0, init: false};
function onReading(values) {
	const {x,y,z} = values;
	if (oldData.init){
		const delta = Math.abs(x - oldData.x) + Math.abs(y - oldData.y) + Math.abs(z - oldData.z);
		if (delta > SHAKEVALUE) {
			stepCount++;
			redraw();
		}
	}
	oldData.x = x;
	oldData.y = y;
	oldData.z = z;
	oldData.init = true;
}

globalThis.button.a.onChanged = function() {
	if (this.read()) {
		lightMode = !lightMode;
		redraw();
	}
}

globalThis.button.b.onChanged = function() {
	if (this.read()) {
		stepCount = 0;
		redraw();
	}
}

redraw();
Timer.repeat(() => {
	const sample = imu.sample();
	onReading(sample.accelerometer)
}, 17)
