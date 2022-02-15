import {Outline} from "commodetto/outline";

export default class extends Behavior {
	onCreate(shape) {
		shape.duration = 4000;
	}
	onFinished(shape) {
		shape.time = 0;
		shape.start();
	}
	onTimeChanged(shape) {
		let f = shape.fraction * 4;
		let which = Math.floor(f);
		if (which == 4) which = 3;
		f -= which;
		const fillPath = new Outline.CanvasPath;
		const strokePath = new Outline.CanvasPath;
		switch (which) {
		case 0:
			f = f * 2 * Math.PI;
			fillPath.moveTo(240, 120);
			fillPath.lineTo(160, 120);
			fillPath.lineTo(160 + (80 * Math.cos(f)), 120 + (80 * Math.sin(f)));
			strokePath.arc(160, 120, 80, 0, f);
			shape.bubble("onLabel", `CanvasPath arc`);
			break;
		case 1:
			f = f * 2 * Math.PI;
			fillPath.moveTo(240, 120);
			fillPath.lineTo(160, 120);
			fillPath.lineTo(160 + (80 * Math.cos(f)), 120 + (60 * Math.sin(f)));

			strokePath.ellipse(160, 120, 80, 60, 0, 0, f);
			shape.bubble("onLabel", `CanvasPath ellipse`);
			break;
		case 2:
			const y1 = 120 + 100 * f;
			const y2 = 120 - 100 * f;
			fillPath.moveTo(40, 120);
			fillPath.lineTo(120, y1);
			fillPath.lineTo(200, y2);
			fillPath.lineTo(280, 120);
			strokePath.moveTo(40, 120)
			strokePath.bezierCurveTo(120, y1, 200, y2, 280, 120);
			shape.bubble("onLabel", `CanvasPath bezierCurveTo`);
			break;
		case 3:
			const y = 120 + 100 * ((2 * f) - 1);
			fillPath.moveTo(40, 120);
			fillPath.lineTo(160, y);
			fillPath.lineTo(280, 120);
			strokePath.moveTo(40, 120);
			strokePath.quadraticCurveTo(160, y, 280, 120);
			shape.bubble("onLabel", `CanvasPath quadraticCurveTo`);
			break;
		}
		shape.fillOutline = Outline.stroke(fillPath, 2);
		shape.strokeOutline = Outline.stroke(strokePath, 4);
	}
}
