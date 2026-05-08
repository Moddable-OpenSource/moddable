/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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

import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import config from "mc/config";
import Timer from "timer";

const GYRO_SCALER = 0.002;

const render = new Poco(screen, {rotation: config?.rotation ?? screen.rotation});
const width = render.width, height = render.height;

const font = parseBMF(new Resource("OpenSans-Semibold-16.bf4"));

const ball = parseBMP(new Resource("ball-color.bmp"));
ball.alpha = parseBMP(new Resource("ball-alpha.bmp"));
ball.x = width >> 1;
ball.y = height >> 1;
ball.vx = 0;
ball.vy = 0;
ball.yMin = font.height * 3;
ball.backGroundColor = render.makeColor(0, 0, 0);

const textColor = render.makeColor(255, 255, 255);
const backgroundColor = render.makeColor(64, 64, 64);
const barColor = render.makeColor(128, 128, 128);

render.begin();
	render.fillRectangle(backgroundColor, 0, 0, width, ball.yMin);
	render.fillRectangle(ball.backgroundColor, 0, ball.yMin, width, height);
render.end();

const imu = new device.sensor.IMU()
imu.configure({
	"GYRO_SCALER": (2000.0 / 32768.0) * GYRO_SCALER
})
Timer.repeat(() => {
	const sample = imu.sample();
	if(flag) {
		onReading(sample.accelerometer, "a");
	} else {
		onReading(sample.gyroscope, "g");
	}
}, 17)

let flag = true;

globalThis.button.a.onChanged = () =>{
	const value = globalThis.button.a.read();
	if (value) {
		flag = !flag;
	}
}

function onReading(values, labelPrefix){
	const { x, y, z } = values;
	// trace(`${labelPrefix}X: ${formatValue(x)} - ${labelPrefix}Y: ${formatValue(y)} - ${labelPrefix}Z: ${formatValue(z)}\n`);
	render.begin(0, 0, width, ball.yMin);
	render.fillRectangle(backgroundColor, 0, 0, width, height);

	drawBar(`${labelPrefix}X`, x, 0, 0, width, font.height);
	drawBar(`${labelPrefix}Y`, y, 0, font.height, width, font.height);
	drawBar(`${labelPrefix}Z`, z, 0, font.height * 2, width, font.height);
	render.end();

	ball.vx = (ball.vx + x) * 0.98;
	ball.vy = (ball.vy - y) * 0.98;
	let nx = ball.x + ball.vx;
	let ny = ball.y + ball.vy;
	if (nx < 0) {
		nx = -nx;
		ball.vx = -ball.vx * 0.7;
	}
	else if (nx > (width - ball.width)) {
		nx = width - ball.width;
		ball.vx = -ball.vx * 0.7;
	}
	if (ny < ball.yMin) {
		ny = ball.yMin;
		ball.vy = -ball.vy * 0.7;
	}
	else if (ny > (height - ball.height)) {
		ny = height - ball.height;
		ball.vy = -ball.vy * 0.7;
	}
	moveBallTo(nx, ny)
}

function formatValue(value) {
	if (!value)
		return value;
	if (value < 0)
		return value.toFixed(3);
	return `+${value.toFixed(3)}`;
}

function drawBar(label, value, x, y, width, height) {
	const halfWidth = width >> 1;
	const barWidth = (value * halfWidth) | 0;

	if (value > 0)
		render.fillRectangle(barColor, x + halfWidth, y, barWidth, height);
	else
		render.fillRectangle(barColor, x + halfWidth + barWidth, y, -barWidth, height);

	render.drawText(label, font, textColor, x + 50, y);
}

function moveBallTo(x, y) {
	const w = ball.width, h = ball.height;

	if ((Math.abs(ball.x - x) <= w) && (Math.abs(ball.y - y) <= h))
		render.begin(Math.min(ball.x, x), Math.min(ball.y, y), w << 1, h << 1);		// often overdrawing
	else {
		render.begin(ball.x, ball.y, w, h);
		render.fillRectangle(ball.backgroundColor, 0, 0, width, height);
		render.continue(x, y, w, h);
	}

	render.fillRectangle(ball.backgroundColor, 0, 0, width, height);
	render.drawMasked(ball, x, y, 0, 0, w, h, ball.alpha, 0, 0);
	render.end();

	ball.x = x;
	ball.y = y;
}
