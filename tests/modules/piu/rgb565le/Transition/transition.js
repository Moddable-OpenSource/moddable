/*---
description: 
flags: [onlyStrict, async]
---*/

let events = [];

class SampleTransition extends Transition {
	constructor() {
		super(50);
		try {
			screen.checkImage("73d09edd8afd47d9038203b6c975f44c");
		} catch(e) {
			$DONE(e.message);
		}
	}
	onBegin(container, current, next) {
		current.state = 1;
		try {
			screen.checkImage("ebc4432964a5c602347717a831ce4163");
		} catch(e) {
			$DONE(e.message);
		}
	}
	onStep(fraction) {
		if (!this.tested) {
			container1.state = 2;
			try {
				screen.checkImage("e0f2c943e0a7398b6bda3b02ec6e9ff3");
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
			screen.checkImage("73abe5bc0e6f03166f8810a15b95416f");
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