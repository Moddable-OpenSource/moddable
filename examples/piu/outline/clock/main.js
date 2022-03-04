import {} from "piu/MC";
import {} from "piu/shape";
import {Outline} from "commodetto/outline";

class FrameBehavior extends Behavior {
	onCreate(shape) {
		const path = new Outline.CanvasPath();
		path.arc(120, 120, 120, 0, 2 * Math.PI);	
		shape.fillOutline = Outline.fill(path);
	}
}

class DialBehavior extends Behavior {
	onCreate(shape) {
		const fillPath = new Outline.CanvasPath();
		const strokePath = new Outline.CanvasPath();
		for (let i = 0; i < 60; i++) {
			const a = 6 * i  * Math.PI / 180;
			const dx = Math.cos(a);
			const dy = Math.sin(a);
			if (i % 5) {
				fillPath.moveTo(120 * dx, 120 * dy);
				fillPath.lineTo(114 * dx, 114 * dy);
			}
			else {
				strokePath.moveTo(120 * dx, 120 * dy);
				strokePath.lineTo(95 * dx, 95 * dy);
			}
		}
		shape.fillOutline = Outline.stroke(fillPath, 3, Outline.LINECAP_BUTT).translate(120, 120);
		shape.strokeOutline = Outline.stroke(strokePath, 7, Outline.LINECAP_BUTT).translate(120, 120);
	}
}

class HandBehavior extends Behavior {
	onClockChanged(shape, t) {
		const a = ((180 - (t * 6)) % 360) * Math.PI / 180;
		const cx = shape.width >> 1;
		const cy = shape.height >> 1;
		shape.fillOutline = this.outline.clone().rotate(a).translate(cx, cy);
	}
}

class HourBehavior extends HandBehavior {
	onCreate(shape) {
		const path = new Outline.CanvasPath();
		path.moveTo(-7, -22);
		path.lineTo(7, -22);
		path.lineTo(6, 65);
		path.lineTo(-6, 65);
		path.lineTo(-7, -22);
		path.closePath();
		this.outline = Outline.fill(path);
	}
}

class MinuteBehavior extends HandBehavior {
	onCreate(shape) {
		const path = new Outline.CanvasPath();
		path.moveTo(-7, -22);
		path.lineTo(7, -22);
		path.lineTo(4, 98);
		path.lineTo(-4, 98);
		path.lineTo(-7, -22);
		path.closePath();
		this.outline = Outline.fill(path);
	}
}

class SecondBehavior extends HandBehavior {
	onCreate(shape) {
		const width = 2;
		const path = new Outline.CanvasPath();
		path.rect(-(width / 2), -30, width, 96);
		path.arc(0, 0, 4, 0, 2 * Math.PI);
		path.closePath();
		path.arc(0, 67, 6, 0, 2 * Math.PI);
		path.closePath();
		this.outline = Outline.fill(path);
	}
}

class ClockApplicationBehavior extends Behavior {
	onCreate(application, $) {
		this.$ = $;
		application.interval = 33;
		application.start();
	}
	onTimeChanged(application) {
		const date = new Date();
		const hours = date.getHours() % 12;
		const minutes = date.getMinutes();
		const seconds = date.getSeconds() + (date.getMilliseconds() / 1000);
		const $ = this.$;
		$.HOURS.delegate("onClockChanged", (hours * 5) + (minutes / 12));
		$.MINUTES.delegate("onClockChanged", minutes);
		$.SECONDS.delegate("onClockChanged", seconds);
	}
}

let ClockApplication = Application.template($ => ({
	Behavior: ClockApplicationBehavior, skin:{ fill:"black" },
	contents: [
		Shape($, { width:240, height:240, Behavior:FrameBehavior, skin:{ fill:"white", stroke:"black" } } ),
		Shape($, { width:240, height:240, Behavior:DialBehavior, skin:{ fill:"black", stroke:"black" } } ),
		Shape($, { anchor:"HOURS", width:240, height:240, Behavior:HourBehavior, skin:{ fill:"black" } } ),
		Shape($, { anchor:"MINUTES", width:240, height:240, Behavior:MinuteBehavior, skin:{ fill:"black" } } ),
		Shape($, { anchor:"SECONDS", width:240, height:240, Behavior:SecondBehavior, skin:{ fill:"red" } } ),
	]
}));
export default new ClockApplication({}, { pixels: 240 * 32 });

