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

const applicationSkin = new Skin({ fill:"gray" });
const applicationStyle = new Style({ font:"600 28px Open Sans", color:"white", });

const format = "buffer/disposable";	// or "buffer"

class CameraModel {
	constructor() {
		let width = 176;
		let height = 144;
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

		width = camera.width;
		height = camera.height;

		this.width = width;
		this.height = height;

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

let CameraApplication = Application.template($ => ({
	skin:applicationSkin, style:applicationStyle, Behavior:ApplicationBehavior,
	contents: [
		Container($, {
			width:$.width, height:$.height, clip:true,
			contents: [
				ImageBuffer($, { anchor:"PREVIEW", imageWidth:$.camera.width, imageHeight:$.camera.height }),
			]
		}),
		Label($, { top:0, string:`${$.width} x ${$.height}` }),
	]
}));

export default new CameraApplication(new CameraModel(), { pixels: screen.width * 16, commandListLength:4096, displayListLength:4096, touchCount:1 });
