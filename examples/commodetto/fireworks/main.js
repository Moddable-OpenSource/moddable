/*
 * Copyright (c) 2022 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
 
import Poco from "commodetto/Poco";
import Timer from "timer";
import config from "mc/config";

const background = {red: 0, green: 0, blue: 0};

// fireworks based on https://slicker.me/javascript/fireworks.htm

const render = new Poco(screen, {
	displayListLength: 4 * 1024,
	pixels: screen.width * 60,
	rotation: config.rotation
});

const width = render.width;
const height = render.height;

const max_fireworks = 1,
	max_sparks = 20;

const fireworks = [];

for (let i = 0; i < max_fireworks; i++) {
	const firework = {
		sparks: new Array(max_sparks)
	};
	fireworks.push(firework);
	resetFirework(firework);
}

function resetFirework(firework) {
	firework.x = (width >> 2) + Math.floor(Math.random() * width / 2);
	firework.y = height;
	firework.age = 0;
	firework.phase = 'fly';

	for (let n = 0; n < max_sparks; n++) {
		let spark = {
			vx: Math.random() * 5 + .5,
			vy: Math.random() * 5 + .5,
			weight: Math.random() * .3 + .03,
			red: Math.floor(Math.random() * 2),
			green: Math.floor(Math.random() * 2),
			blue: Math.floor(Math.random() * 2)
		};
		if (Math.random() > .5) spark.vx = -spark.vx;
		if (Math.random() > .5) spark.vy = -spark.vy;
		firework.sparks[n] = spark;
	}
}

render.begin(0, 0, width, height);
render.fillRectangle(render.makeColor(64, 64, 64), 0, 0, width, height);
render.end();

function explode() {
	render.begin(0, 0, width, height);
	render.fillRectangle(render.makeColor(background.red, background.green, background.blue), 0, 0, width, height);
	fireworks.forEach((firework, index) => {
		if (firework.phase == 'explode') {
			firework.sparks.forEach((spark) => {
				for (let i = 0; i < 10; i++) {
					let trailAge = firework.age + i;
					let x = firework.x + spark.vx * trailAge;
					let y = firework.y + spark.vy * trailAge + spark.weight * trailAge * spark.weight * trailAge;
					let fade = i * 20 - firework.age * 2;
					let color = render.makeColor(spark.red * fade, spark.green * fade, spark.blue * fade);
					render.fillRectangle(color, x, y, 4, 4);
				}
			});
			firework.age++;
			if (firework.age > 75 && Math.random() < .05)
				resetFirework(firework);
		} else {
			firework.y = firework.y - 10;
			for (let spark = 0; spark < 15; spark++) {
				let color = render.makeColor(index * 50, spark * 17, 0);
				render.fillRectangle(color, firework.x + Math.random() * spark - spark / 2, firework.y + spark * 4, 4, 4);
			}
			if (Math.random() < .001 || firework.y < (width / 3)) firework.phase = 'explode';
		}
	});
	render.end();
}

Timer.repeat(explode, 17);

// if there's a Host, try to set up LED and butons
if (globalThis.Host) {
	let led;

	// Default LED 
	if (Host.LED?.Default)
		led = new Host.LED.Default;

	// Default button launches fireworks 
	new Host.Button.Default({
		onPush() {
			if (this.pressed)
				fireworks.forEach(firework => resetFirework(firework));
			else
				fireworks.forEach(firework => firework.phase = "explode");
		}
	});

	// Scan for other availble buttons to set background color 
	const colors = ["red", "green", "blue"];
	for (let name in Host.Button) {
		if (name === "Default")
			continue;
		const Button = Host.Button[name];
		if (Button === Host.Button.Default) {
			trace(`Button ${name}: launch firework\n`);
			continue;
		}
		const color = colors.shift();
		trace(`Button ${name}: color ${color}\n`);
		new Button({
			onPush() {
				background[color] = this.pressed ? 255 : 0; 
				led?.write(background);
			}
		});
		if (!colors.length)
			break;
	}
}
