import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		const path = new Outline.CanvasPath();
		path.moveTo(80, 60);
		path.lineTo(240, 60);
		
		path.moveTo(80, 120);
		path.quadraticCurveTo(160, 200, 240, 120);
		
		this.outlines = [
			Outline.stroke(path, 20, Outline.LINECAP_BUTT),
			Outline.stroke(path, 20, Outline.LINECAP_ROUND),
			Outline.stroke(path, 20, Outline.LINECAP_SQUARE),
		];
		this.labels = [
			"LINECAP_BUTT",
			"LINECAP_ROUND",
			"LINECAP_SQUARE",
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
