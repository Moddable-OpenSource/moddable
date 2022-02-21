import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		shape.duration = 2000;
	}
	onFinished(shape) {
		shape.time = 0;
		shape.start();
	}
	onTimeChanged(shape) {
		let f = shape.fraction * 2;
		let which = Math.floor(f);
		if (which == 2) which = 1;
		f -= which;
		const fillPath = new Outline.FreeTypePath;
		const strokePath = new Outline.FreeTypePath;
		fillPath.beginSubpath(40, 120, true);
		strokePath.beginSubpath(40, 120, true);
		switch (which) {
		case 0:
			const y = 120 + 100 * ((2 * f) - 1);
			fillPath.lineTo(160, y);
			fillPath.lineTo(280, 120);
			strokePath.conicTo(160, y, 280, 120);
			shape.bubble("onLabel", `FreeTypePath conicTo`);
			break;
		case 1:
			const y1 = 120 + 100 * f;
			const y2 = 120 - 100 * f;
			fillPath.lineTo(120, y1);
			fillPath.lineTo(200, y2);
			fillPath.lineTo(280, 120);
			strokePath.cubicTo(120, y1, 200, y2, 280, 120);
			shape.bubble("onLabel", `FreeTypePath cubicTo`);
			break;
		}
		fillPath.endSubpath();
		strokePath.endSubpath();
		shape.fillOutline = Outline.stroke(fillPath, 2);
		shape.strokeOutline = Outline.stroke(strokePath, 4);
	}
}
