import {} from "piu/MC";
import {} from "piu/shape";
import {Outline} from "commodetto/outline";

class BallBehavior extends Behavior {
	onCreate(ball, delta) {
		this.dx = delta;
		this.dy = delta;
	}
	onDisplaying(ball) {
		this.x = ball.x;
		this.y = ball.y;
		this.width = ball.container.width - ball.width;
		this.height = ball.container.height - ball.height;
		ball.start();
	}
	onTimeChanged(ball) {
		var dx = this.dx;
		var dy = this.dy;
		ball.moveBy(dx, dy);
		var x = this.x + dx;
		var y = this.y + dy;
		if ((x < 0) || (x > this.width)) dx = -dx;
		if ((y < 0) || (y > this.height)) dy = -dy;
		this.dx = dx;
		this.dy = dy;
		this.x = x;
		this.y = y;
	}
};

class Shape1Behavior extends BallBehavior {
	onCreate(shape, delta) {
		super.onCreate(shape, delta);
		const path = new Outline.FreeTypePath;
		path.beginSubpath(50, 90);
		path.lineTo(18, 42);
		path.cubicTo(10, 30, 30, 10, 50, 40);
		path.cubicTo(70, 10, 90, 30, 82, 42);
		path.endSubpath();
		shape.fillOutline = Outline.fill(path);
		shape.strokeOutline = Outline.stroke(path, 5, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
	}
}

class Shape2Behavior extends BallBehavior {
	onCreate(shape, delta) {
		super.onCreate(shape, delta);
		const path = Outline.PolygonPath(50,0,21,90,98,35,2,35,79,90);
		this.fillOutline = Outline.fill(path, Outline.EVEN_ODD_RULE);
		this.strokeOutline = Outline.stroke(path, 5, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		this.angle = 0;
		this.when = Date.now();
	}
	onTimeChanged(shape) {
		super.onTimeChanged(shape);
		const when = Date.now();
		if (this.when < when) {
			const angle = (this.angle * Math.PI) / 180;
			shape.fillOutline = this.fillOutline.clone().rotate(angle, 50, 50);
			shape.strokeOutline = this.strokeOutline.clone().rotate(angle, 50, 50);
			this.angle += 5;
			if (this.angle > 360)
				this.angle = 0;
			this.when += 50;
		}
	}
}

class Shape3Behavior extends BallBehavior {
	onCreate(shape, delta) {
		super.onCreate(shape, delta);
		const path = Outline.SVGPath(`M 50,50 
			c  5,0  5,10 0,10 c -10,0 -10,-20 0,-20 
			c 20,0 20,30 0,30 c -25,0 -25,-40 0,-40 
			c 35,0 35,50 0,50 c -40,0 -40,-60 0,-60 
			c 50,0 50,70 0,70 c -55,0 -55,-80 0,-80
		`);
		this.strokeOutline = Outline.stroke(path, 5, Outline.LINECAP_SQUARE);
		this.angle = 360;
		this.when = Date.now();
	}
	onTimeChanged(shape) {
		super.onTimeChanged(shape);
		const when = Date.now();
		if (this.when < when) {
			const angle = (this.angle * Math.PI) / 180;
			shape.strokeOutline = this.strokeOutline.clone().rotate(angle, 50, 50);
			this.angle -= 5;
			if (this.angle < 0)
				this.angle = 360;
			this.when += 50;
		}
	}
}

class Shape4Behavior extends BallBehavior {
	onCreate(shape, delta) {
		super.onCreate(shape, delta);
		const path = new Outline.CanvasPath;
		path.moveTo(50, 10);
		path.bezierCurveTo(75, 10, 90, 25, 90, 50);
		path.bezierCurveTo(90, 75, 75, 90, 50, 90);
		path.bezierCurveTo(25, 90, 10, 75, 10, 50);
		path.bezierCurveTo(10, 25, 25, 10, 50, 10);
		path.closePath();
		path.moveTo(70, 60);
		path.bezierCurveTo(65, 75, 35, 75, 30, 60);
		path.rect(35,35,5,5);
		path.rect(60,35,5,5);
		shape.fillOutline = Outline.fill(path, Outline.NON_ZERO_RULE);
		shape.strokeOutline = Outline.stroke(path, 5);
	}
}

let ShapeApplication = Application.template($ => ({
	skin:new Skin({ fill:"black" }),
	contents: [
		Shape(1, { right:0, top:0, width:100, height:100, Behavior: Shape4Behavior, skin:new Skin({ fill:rgba(255,255,0,0.75), stroke:rgb(255,255,0,) }) } ),
		Shape(4, { left:0, top:0, width:100, height:100, Behavior: Shape1Behavior, skin:new Skin({ fill:rgba(255,0,0,0.75), stroke:rgb(255,0,0) }) } ),
		Shape(2, { right:0, bottom:0, width:100, height:100, Behavior: Shape3Behavior, skin:new Skin({ fill:rgba(0,255,0,0.75), stroke:rgb(0,255,0) }) } ),
		Shape(3, { left:0, bottom:0, width:100, height:100, Behavior: Shape2Behavior, skin:new Skin({ fill:rgba(0,0,255,0.75), stroke:rgb(0,0,255) }) } ),
	]
}));

export default new ShapeApplication(null, { displayListLength:4096, touchCount:1, pixels: 240 * 64 });

