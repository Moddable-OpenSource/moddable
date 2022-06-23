/*---
description: 
flags: [onlyStrict, async]
---*/

let events = [];

class SampleTransition extends Transition {
	constructor() {
		super(50);
		try {
			screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");
		} catch(e) {
			$DONE(e.message);
		}
	}
	onBegin(container, current, next) {
		current.state = 1;
		try {
			screen.checkImage("64668108040303d923bbf1399e359203");
		} catch(e) {
			$DONE(e.message);
		}
	}
	onStep(fraction) {
		if (!this.tested) {
			container1.state = 2;
			try {
				screen.checkImage("5bf3d0246377fe601c1bcab629307974");
			} catch(e) {
				$DONE(e.message);
			}
			this.tested = true;
		}
	}
	onEnd(container, current, next) {
		container.remove(current);
		container.add(next);
		try {
			screen.checkImage("817a0ecd3bdedd0c64a77176d88dd8db");
		} catch(e) {
			$DONE(e.message);
		}
		$DONE();
	}
}

let container1 = new Container(null, { top: 0, bottom: 0, left: 0, right: 0, skin: new Skin({ fill: ["red", "yellow", "green"] }) });
let container2 = new Container(null, { top: 0, bottom: 0, left: 0, right: 0, skin: new Skin({ fill: "blue" }) });

new Application({}, {
	contents: [container1]
});

let transition = new SampleTransition();


application.run(transition, container1, container2);

$TESTMC.timeout(200, "`onEnd` should have been triggered");