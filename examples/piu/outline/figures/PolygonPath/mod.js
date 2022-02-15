import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		this.count = 2;
		shape.duration = 1000;
		shape.bubble("onLabel", `PolygonPath`);
	}
	onFinished(shape) {
		shape.time = 0;
		shape.start();
	}
	onTimeChanged(shape) {
		const count = 3 + Math.floor(shape.fraction * 9)
		if (this.count != count) {
			const cx = application.width >> 1;
			const cy = application.height >> 1;
			const r = Math.min(cx, cy) - 40;
			const result = new Array();
			const delta = 2 * Math.PI / count;
			let angle = Math.PI / 2;
			for (let i = 0; i < count; i++) {
				result.push(cx + (r  * Math.cos(angle)));
				result.push(cy + (r  * Math.sin(angle)));
				angle += delta;
			}
			const path = Outline.PolygonPath(...result);
			shape.fillOutline = Outline.fill(path);
			shape.strokeOutline = Outline.stroke(path, 5, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
			this.count = count;
		}
	}
}
