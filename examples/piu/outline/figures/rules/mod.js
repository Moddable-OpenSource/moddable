
import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		const path = Outline.PolygonPath(
			160,70,
			131,160,
			208,105,
			112,105,
			189,160);
		this.outlines = [
			Outline.fill(path, Outline.NON_ZERO_RULE),
			Outline.fill(path, Outline.EVEN_ODD_RULE),
		];
		this.labels = [
			"NON_ZERO_RULE",
			"EVEN_ODD_RULE",
		];
		this.index = -1;
		shape.duration = 2000;
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
