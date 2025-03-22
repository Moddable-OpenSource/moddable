/*
 * Copyright (c) 2024 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
import {} from "piu/MC";
import Bitmap from "commodetto/Bitmap";
import Camera from "embedded:x-io/imagein/camera";

const GRAY = "#808080";
const RED = "#FF000080";
const GREEN = "#00FF0080";
const BLUE = "#0000FF80";

const applicationSkin = new Skin({ fill:GRAY });
const applicationStyle = new Style({ font:"600 28px Open Sans", color:"white", });

const diskSkin = new Skin({ texture:new Texture("disk.png"), width:60, height:60, color:[RED,GREEN,BLUE] });
const maskSkin = new Skin({ texture:new Texture("mask.png"), width:120, height:120, color:GRAY });

const format = "buffer/disposable";	// or "buffer"

class CameraModel {
	constructor() {
		let width = 120;
		let height = 120;
		const imageType = Bitmap.RGB565LE;
		
		let frame;
		const camera = new Camera({
			width,
			height,
			imageType,
			format,
			onReadable: () => {
				if ("buffer" === camera.format) {
					camera.read(frame);
				}
				else {
					frame?.close();
					frame = camera.read();
					if (!frame)
						return trace("read failed: no frame\n");
				}
				this.PREVIEW.buffer = frame;
			}
		});
		this.width = width;
		this.height = height;

		width = camera.width;
		height = camera.height;

		if ("buffer" === camera.format)
			frame = new SharedArrayBuffer(width * height * 2);
		trace(`Camera ${width} x ${height}\n`);
		
		camera.start();
		this.camera = camera;
	}
}

class ApplicationBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
	}
	onQuit(application) {
		this.data.camera?.close();
	}
}

class BallBehavior extends Behavior {
	onCreate(ball, delta) {
		this.dx = delta;
		this.dy = delta;
	}
	onDisplaying(ball) {
		this.x = ball.x;
		this.y = ball.y;
		this.width = ball.container.width - ball.width;
		this.height = ball.container.height - ball.height;
		ball.start();
	}
	onTimeChanged(ball) {
		var dx = this.dx;
		var dy = this.dy;
		ball.moveBy(dx, dy);
		var x = this.x + dx;
		var y = this.y + dy;
		if ((x < 0) || (x > this.width)) dx = -dx;
		if ((y < 0) || (y > this.height)) dy = -dy;
		this.dx = dx;
		this.dy = dy;
		this.x = x;
		this.y = y;
	}
};

let CameraApplication = Application.template($ => ({
	skin:applicationSkin, style:applicationStyle, Behavior:ApplicationBehavior,
	contents: [
		Container(2, {
			left:0, width:$.width, top:0, height:$.height, clip:true, Behavior:BallBehavior,
			contents: [
				ImageBuffer($, { anchor:"PREVIEW", imageWidth:$.camera.width, imageHeight:$.camera.height }),
				Content($, { skin:maskSkin }),
			]
		}),
		Content(4, { right:0, top:0, skin:diskSkin, state:0, Behavior:BallBehavior }),
		Content(5, { right:0, bottom:0, skin:diskSkin, state:1, Behavior:BallBehavior }),
		Content(6, { left:0, bottom:0, skin:diskSkin, state:2, Behavior:BallBehavior }),
	]
}));

export default new CameraApplication(new CameraModel(), { pixels: screen.width * 16, commandListLength:8192, displayListLength:8192, touchCount:1 });
