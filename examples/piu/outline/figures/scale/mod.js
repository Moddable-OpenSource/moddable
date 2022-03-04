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
		const f = 0.5 + (Math.quadEaseOut(shape.fraction) * 4);
		const cx = this.cx;
		const cy = this.cy;
		shape.fillOutline = this.fillOutline.clone().translate(-cx, -cy).scale(f).translate(cx, cy);
		shape.strokeOutline = this.strokeOutline.clone().translate(-cx, -cy).scale(f).translate(cx, cy);
		shape.bubble("onLabel", `scale(${f.toFixed(3)})`);
	}
}
