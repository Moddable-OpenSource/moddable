import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		this.radius = 0;
		shape.duration = 1000;
		shape.bubble("onLabel", `RoundRectPath`);
	}
	onFinished(shape) {
		shape.time = 0;
		shape.start();
	}
	onTimeChanged(shape) {
		const radius = Math.floor(shape.fraction * 80)
		if (this.radius != radius) {
			const path = Outline.RoundRectPath(40, 40, application.width - 80, application.height - 80, radius);
			shape.fillOutline = Outline.fill(path);
			shape.strokeOutline = Outline.stroke(path, 5, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
			this.radius = radius;
		}
	}
}
