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

import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";

const ROTATE_BUFFER_LINES = 16;
const SEND_LINES = 16;
const UPDATE_HEIGHT = 160;
const COLORS = [0xf800, 0x07e0, 0x001f, 0xffe0, 0x07ff, 0xf81f, 0xffff, 0x39e7];

const font = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));
const tests = [
	{ name: "rotation 0, full width", rotation: 0, width: "full", frames: 24 },
	{ name: "rotation 90, full width", rotation: 90, width: "full", frames: 24 },
	{ name: "rotation 90, narrow", rotation: 90, width: 96, frames: 36 },
	{ name: "rotation 270, full width", rotation: 270, width: "full", frames: 24 },
	{ name: "rotation 270, narrow", rotation: 270, width: 96, frames: 36 },
	{ name: "rotation 180, full width", rotation: 180, width: "full", frames: 24 }
];

let testIndex = 0;
const bufferCache = new Map;

trace("CO5300 rotation benchmark\n");
trace("Build once before and after the driver change, then compare avg frame ms.\n");

Timer.set(runNextTest, 250);

function runNextTest() {
	const test = tests[testIndex++ % tests.length];
	const result = benchmark(test);

	trace(`${result.name}: avg=${result.average} ms/frame, total=${result.elapsed} ms, frames=${result.frames}, update=${result.updateWidth}x${result.updateHeight}, rotation=${result.rotation}`);
	if (usesSoftwareRotation(result.rotation))
		trace(`, estimated chunks old=${result.oldChunks}, current=${result.currentChunks}`);
	trace("\n");

	drawSummary(result);
	Timer.set(runNextTest, 2500);
}

function benchmark(test) {
	screen.rotation = test.rotation;

	const screenWidth = screen.width;
	const screenHeight = screen.height;
	const updateWidth = ("full" === test.width) ? screenWidth : Math.min(test.width, screenWidth);
	const updateHeight = Math.min(UPDATE_HEIGHT, screenHeight);
	const sendLines = Math.min(SEND_LINES, updateHeight);
	const xRange = screenWidth - updateWidth;
	const y = (screenHeight - updateHeight) >> 1;
	const buffers = makePatternBuffers(updateWidth, sendLines);

	let start = Date.now();
	for (let frame = 0; frame < test.frames; frame++) {
		const x = xRange ? ((frame * 17) % (xRange + 1)) : 0;
		let lines = updateHeight;
		let phase = frame & 3;

		screen.begin(x, y, updateWidth, updateHeight);
		while (lines > 0) {
			const rows = (lines > sendLines) ? sendLines : lines;
			screen.send(buffers[phase].buffer, 0, updateWidth * rows * 2);
			lines -= rows;
			phase = (phase + 1) & 3;
		}
		screen.end();
	}
	const elapsed = Date.now() - start;

	return {
		name: test.name,
		rotation: test.rotation,
		frames: test.frames,
		elapsed,
		average: Math.round(elapsed / test.frames),
		fps: Math.round((test.frames * 1000) / elapsed),
		updateWidth,
		updateHeight,
		sendLines,
		oldChunks: estimateChunks(updateWidth, updateHeight, sendLines, 2),
		currentChunks: estimateChunks(updateWidth, updateHeight, sendLines, Math.max(1, Math.idiv((Math.max(screenWidth, screenHeight) * ROTATE_BUFFER_LINES), updateWidth)))
	};
}

function estimateChunks(updateWidth, updateHeight, sendLines, rowsPerChunk) {
	let chunks = 0;
	for (let lines = updateHeight; lines > 0;) {
		const rows = (lines > sendLines) ? sendLines : lines;
		chunks += Math.idiv(rows + rowsPerChunk - 1, rowsPerChunk);
		lines -= rows;
	}
	return chunks;
}

function makePatternBuffers(width, lines) {
	const key = `${width}:${lines}`;
	let buffers = bufferCache.get(key);
	if (buffers)
		return buffers;

	buffers = [];

	for (let phase = 0; phase < 4; phase++) {
		const pixels = new Uint16Array(new SharedArrayBuffer(width * lines * 2));
		for (let y = 0; y < lines; y++) {
			for (let x = 0; x < width; x++)
				pixels[(y * width) + x] = COLORS[((x >> 4) + (y >> 3) + phase) & 7];
		}
		buffers.push(pixels);
	}

	bufferCache.set(key, buffers);
	return buffers;
}

function drawSummary(result) {
	screen.rotation = result.rotation;

	const render = new Poco(screen, { displayListLength: 4096 });
	try {
		const black = render.makeColor(0, 0, 0);
		const white = render.makeColor(255, 255, 255);
		const green = render.makeColor(0, 220, 120);
		const yellow = render.makeColor(255, 220, 0);
		const cyan = render.makeColor(0, 180, 255);
		const gray = render.makeColor(45, 45, 45);

		render.begin();
		render.fillRectangle(black, 0, 0, render.width, render.height);
		render.fillRectangle(gray, 0, 0, render.width, 34);
		drawText(render, "CO5300 rotation benchmark", white, 8, 8);
		drawText(render, result.name, cyan, 8, 48);
		drawText(render, `update ${result.updateWidth}x${result.updateHeight}, send ${result.sendLines} lines`, white, 8, 76);
		drawText(render, `avg ${result.average} ms/frame, ${result.fps} fps`, green, 8, 104);
		drawText(render, `total ${result.elapsed} ms / ${result.frames} frames`, white, 8, 132);

		if (usesSoftwareRotation(result.rotation)) {
			const maxChunks = Math.max(result.oldChunks, result.currentChunks);
			const oldWidth = Math.max(1, Math.idiv((render.width - 24) * result.oldChunks, maxChunks));
			const currentWidth = Math.max(1, Math.idiv((render.width - 24) * result.currentChunks, maxChunks));

			drawText(render, `estimated chunks/frame old ${result.oldChunks}`, yellow, 8, 172);
			render.fillRectangle(yellow, 8, 198, oldWidth, 16);
			drawText(render, `estimated chunks/frame current ${result.currentChunks}`, green, 8, 224);
			render.fillRectangle(green, 8, 250, currentWidth, 16);
		}
		else
			drawText(render, "90/270 software rotation not used", yellow, 8, 172);

		render.end();
	}
	finally {
		render.close();
	}
}

function drawText(render, text, color, x, y) {
	render.drawText(text, font, color, x, y);
}

function usesSoftwareRotation(rotation) {
	return (90 === rotation) || (270 === rotation);
}
