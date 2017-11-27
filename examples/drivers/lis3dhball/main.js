/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
import ILI9341 from "ili9341";
import LIS3DH from "lis3dh";

let pixelsOut = new ILI9341({});
const width = pixelsOut.width;
const height = pixelsOut.height;

let render = new Poco(pixelsOut);
let font = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));

let ball = parseBMP(new Resource("ball-color.bmp"));
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

let sensor = new LIS3DH({});

Timer.repeat(() => {
	let values = sensor.sample();

	if (180 === parseInt(config.orientation)) {
		values.x = -values.x;
	}

	render.begin(0, 0, width, ball.yMin);
		render.fillRectangle(backgroundColor, 0, 0, width, height);

		drawBar("X", values.x, 0, 0, width, font.height);
		drawBar("Y", values.y, 0, font.height, width, font.height);
		drawBar("Z", values.z, 0, font.height * 2, width, font.height);
	render.end();

	ball.vx = (ball.vx + values.y) * 0.98;
	ball.vy = (ball.vy + values.x) * 0.98;
	let x = ball.x + ball.vx;
	let y = ball.y + ball.vy;
	if (x < 0) {
		x = -x;
		ball.vx = -ball.vx;
	}
	else if (x > (width - ball.width)) {
		x = width - ball.width;
		ball.vx = -ball.vx;
	}
	if (y < ball.yMin) {
		y = ball.yMin;
		ball.vy = -ball.vy;
	}
	else if (y > (height - ball.height)) {
		y = height - ball.height;
		ball.vy = -ball.vy;
	}
	moveBallTo(x, y)
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
	const barWidth = (value * halfWidth) | 0;

	if (value > 0)
		render.fillRectangle(barColor, x + halfWidth, y, barWidth, height);
	else
		render.fillRectangle(barColor, x + halfWidth + barWidth, y, -barWidth, height);

	render.drawText(label + " " + formatValue(value), font, textColor, x + 50, y);
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
