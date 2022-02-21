import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		const path = new Outline.FreeTypePath();
		path.beginSubpath(80, 110, false);
		path.lineTo(160, 30);
		path.lineTo(240, 110);
		path.lineTo(160, 190);
		path.lineTo(80, 110);
		path.endSubpath();
		this.outlines = [
			Outline.stroke(path, 20, Outline.LINECAP_BUTT, Outline.LINEJOIN_ROUND),
			Outline.stroke(path, 20, Outline.LINECAP_BUTT, Outline.LINEJOIN_BEVEL),
			Outline.stroke(path, 20, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER),
		];
		this.labels = [
			"LINEJOIN_ROUND",
			"LINEJOIN_BEVEL",
			"LINEJOIN_MITER",
		];
		this.index = -1;
		shape.duration = 3000;
	}
	onFinished(shape) {
		shape.time = 0;
		shape.start();
	}
	onTimeChanged(shape) {
		const c = this.outlines.length;
		let i = Math.floor(c * shape.fraction);
		if (i == c) i = c - 1;
		if (this.index != i) {
			shape.strokeOutline = this.outlines[i];
			shape.bubble("onLabel", this.labels[i]);
		}
	}
}
