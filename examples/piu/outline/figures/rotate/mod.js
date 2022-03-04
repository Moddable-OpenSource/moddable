import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		const path = new Outline.CanvasPath();
		const cx = application.width >> 1;
		const cy = application.height >> 1;
		const rx = Math.min(cx, cy) - 40;
		const ry = Math.min(cx, cy) - 80;
		path.ellipse(cx, cy, rx, ry, 0, 0, 2 * Math.PI);
		path.ellipse(cx, cy, rx, ry, Math.PI / 3, 0, 2 * Math.PI);
		path.ellipse(cx, cy, rx, ry, 2 * Math.PI / 3, 0, 2 * Math.PI);
		this.fillOutline = Outline.fill(path, Outline.EVEN_ODD_RULE);
		this.strokeOutline = Outline.stroke(path, 5, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		shape.duration = 1000;
		this.cx = cx;
		this.cy = cy;
	}
	onFinished(shape) {
		shape.time = 0;
		shape.start();
	}
	onTimeChanged(shape) {
		const angle = (360 * shape.fraction * Math.PI) / 180;
		shape.fillOutline = this.fillOutline.clone().rotate(angle, this.cx, this.cy);
		shape.strokeOutline = this.strokeOutline.clone().rotate(angle, this.cx, this.cy);
		shape.bubble("onLabel", `rotate(${angle.toFixed(3)})`);
	}
}
