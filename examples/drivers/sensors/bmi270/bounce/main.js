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

import config from "mc/config";
import Timer from "timer";
import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";

const render = new Poco(screen, {rotation: config.rotation ?? screen.rotation});
const width = render.width;
const height = render.height;
const font = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));

const ball = parseBMP(new Resource("ball-color.bmp"));
ball.alpha = parseBMP(new Resource("ball-alpha.bmp"));
ball.x = (width - ball.width) >> 1;
ball.vx = 0;
ball.vy = 0;
ball.yMin = font.height * 3;
ball.y = ball.yMin + ((height - ball.yMin - ball.height) >> 1);
ball.backgroundColor = render.makeColor(0, 0, 0);

const textColor = render.makeColor(255, 255, 255);
const backgroundColor = render.makeColor(64, 64, 64);
const barColor = render.makeColor(128, 128, 128);

render.begin();
	render.fillRectangle(backgroundColor, 0, 0, width, ball.yMin);
	render.fillRectangle(ball.backgroundColor, 0, ball.yMin, width, height - ball.yMin);
render.end();

const sensor = new device.sensor.IMU();

Timer.repeat(() => {
	const {accelerometer} = sensor.sample();
	if (!accelerometer)
		return;

	// BMI270 reports acceleration in m/s²; use g for the bars and ball motion.
	const x = accelerometer.x / 9.80665;
	const y = accelerometer.y / 9.80665;
	const z = accelerometer.z / 9.80665;

	render.begin(0, 0, width, ball.yMin);
		render.fillRectangle(backgroundColor, 0, 0, width, ball.yMin);
		drawBar("X", x, 0, 0, width, font.height);
		drawBar("Y", y, 0, font.height, width, font.height);
		drawBar("Z", z, 0, font.height * 2, width, font.height);
	render.end();

	ball.vx = (ball.vx + x) * 0.98;
	ball.vy = (ball.vy - y) * 0.98;
	let nx = ball.x + ball.vx;
	let ny = ball.y + ball.vy;
	if (nx < 0) {
		nx = 0;
		ball.vx = -ball.vx;
	}
	else if (nx > (width - ball.width)) {
		nx = width - ball.width;
		ball.vx = -ball.vx;
	}
	if (ny < ball.yMin) {
		ny = ball.yMin;
		ball.vy = -ball.vy;
	}
	else if (ny > (height - ball.height)) {
		ny = height - ball.height;
		ball.vy = -ball.vy;
	}
	moveBallTo(Math.round(nx), Math.round(ny));
}, 17);

function formatValue(value) {
	if (!value)
		return value;
	if (value < 0)
		return value.toFixed(3);
	return "+" + value.toFixed(3);
}

function drawBar(label, value, x, y, width, height) {
	const halfWidth = width >> 1;
	const barWidth = (Math.max(-1, Math.min(1, value)) * halfWidth) | 0;

	if (value > 0)
		render.fillRectangle(barColor, x + halfWidth, y, barWidth, height);
	else
		render.fillRectangle(barColor, x + halfWidth + barWidth, y, -barWidth, height);

	render.drawText(`${label} ${formatValue(value)} g`, font, textColor, x + 4, y, width - 8);
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
