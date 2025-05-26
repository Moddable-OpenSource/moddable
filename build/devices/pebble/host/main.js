import Timer from "timer"
import {Rocky} from "pebble/graphics"
import ArchiveResource from "pebble/archive-resource";
import ArchiveCompartment from "ArchiveCompartment"
import KV from "embedded:storage/key-value";
import WebStorage from "webstorage";

const clearImmediate = function(id) { return Timer.clear(id) };
const setImmediate = function(callback) { return Timer.set(callback) };
const setInterval = function(callback, delay) { return Timer.repeat(callback, delay) };
const setTimeout = function(callback, delay) { return Timer.set(callback, delay) };

const console = Object.freeze({
	log(...args) {
		trace(...args, "\n");
	}
});

class AppInfo {
	static get uuid() @ "xs_appinfo_get_uuid"
	static get name() @ "xs_appinfo_get_name"
	static get isWatchface() @ "xs_appinfo_get_isWatchface"
}

const state = {};		// for mutable runtime state

const blockedWatchFace = Object.freeze([
	"pebble/button"
]);

export default function() {
	const rocky = new Rocky({});

	try {
		const r = new ArchiveResource(0);		// mod is in resource 0 in example. make this configurable.
		const archive = r.archive;
		trace(`Found mod "${archive.name}"`);

		const globals = {
			console,
			clearImmediate,
			clearTimeout: clearImmediate,
			clearInterval: clearImmediate,
			setImmediate,
			setInterval,
			setTimeout,
			rocky,
			screen,
			Date,
			Math
		};

		Object.defineProperty(globals, "localStorage", {
			enumerable: true,
			configurable: true,
			get() {
				state.localStorage ??= new WebStorage(KV.open({path: `local-${AppInfo.uuid}`}));
				return state.localStorage;
			}
		});

		const mod = new ArchiveCompartment(archive, {
			globals,
			modules: {},
			loadNowHook(specifier) {
				if (AppInfo.isWatchface && blockedWatchFace.includes(specifier))
					throw new Error(blockedWatchFace + " blocked in watchface");

				return {namespace: specifier};		// map through host modules
			}
		});
		mod.importNow("main");
	}
	catch (e) {
		trace(`Error loading mod in app ${AppInfo.name}: ${e}`);
	}
}








































function dontstrip() {
	try {
		// const ar = new ArchiveResource(4);		// mod is in 4 in pebblekit-js-weather
		const ar = new ArchiveResource(0);		// mod is in 0 in mdbl example
		const archive = ar.archive;
		trace(`Found mod "${archive.name}"`);

		const mod = new ArchiveCompartment(archive, { globals: {Timer}, modules: {} });
		mod.importNow("main");
	}
	catch (e) {
		trace("Error loading mod " + e);
	}


	const accel = new Accelerometer({
		onSample() {
			const sample = this.sample();
			trace(`accel ${sample.x}, ${sample.y}, ${sample.z}\n`);
		},
		onTap(direction) {
			trace(`single tap ${direction}\n`);
		},
		onDoubleTap(direction) {
			trace(`double tap ${direction}\n`);
		}
	});
	accel.configure({hz: 10});

	const battery = new Battery({
		onSample() {
			const sample = this.sample();
			trace(`battery ${sample.percent}%, charging ${sample.charging}, plugged-in ${sample.plugged}\n`);
		}
	});

	globalThis.compass = new Compass({
		onSample() {
			const sample = this.sample();
			trace(`compass ${sample.heading}\n`);
		}
	});

	const message = new Message({
		input: 512,
		output: 512,
		keys: new Map([
			["zero", 0],
			["one", 1]
		]),
		onReadable() {
			let msg = this.read();
			if (!msg)
				return trace("undefined msg!?!?");

			trace(`msg.size ${msg.size}\n`);
			trace(`msg.keys ${Array.from(msg.keys())}\n`);
			trace(`msg.values ${Array.from(msg.values())}\n`);
		},
		onWritable() {
			if (this.once)
				return;

			const m = new Map;
			m.set("zero", "zero");
			m.set("one", Uint8Array.of(1));
			m.set(2, "two " + (new Date));
			m.set(3, 3);
			this.write(m);
			this.once = true;
		}
	});

	const envelope = parseBMP(new Resource("envelope.bmp"));
	const icon = parseRLE(new Resource("wake-alpha.bm4"));

	let render = new Poco(screen);

	const pebbleFont = new render.Font("18px Gothic");

	// const pebbleIcon = new Poco.PebbleBitmap(73);	// RESOURCE_ID_MUSIC_LARGE_CASSETTE Silk
	// trace(`pebbleIcon ${pebbleIcon.width} x ${pebbleIcon.height}\n`);

	let font = parseBMF(new Resource("ArialNarrow-Bold-20.fnt"));
	font.bitmap = parseRLE(new Resource("ArialNarrow-Bold-20.bm4"));

	const black = render.makeColor(0, 0, 0);
	const white = render.makeColor(255, 255, 255);

	/*
	const reader = new ReadGIF(new Resource("120FrameSweep.gif"));
	const x = (screen.width - reader.width) >> 1;
	const y = (screen.height - reader.height) >> 1;
	reader.timer = Timer.repeat(id => rocky.requestDraw(), 1);
	*/

	globalThis.rocky = new Rocky({});

	globalThis.Date = class extends Date {		// hack around unfinished RTC
		constructor(...args) {
			super(...args);
			if (0 == args.length)
				this.setTime(1_746_480_415_759 + Time.ticks);
		}
	};

	trace(`TicToc running under Moddable SDK\n`);

	const store = KV.open({path: "tictoc", format: "string"});
	let mode = Number(store.read("mode") ?? 0);	// 0 = no seconds, 1 = seconds as text, 2 = second hand, 3 = continuous second hand
	let status = "";
	let statusTimer;

	new PebbleButton({
		type: "select",
		onPush(down) {
			if (down) {
				mode = (mode + 1) % 4;
				store.write("mode", mode);
				rocky.requestDraw();

				status = ["No seconds", "Seconds as text", "Second hand", "Continuous second hand"][mode];
				Timer.clear(statusTimer);
				statusTimer = Timer.set(() => {status = ""; rocky.requestDraw(); statusTimer = undefined}, 2500);
				trace(`Rendering mode: ${status}\n`);
			}
		}
	});

	var WatchfaceHelper = function(date = new Date) {
		function clockwiseRad(fraction) {
		// TODO: figure out if this is actually correct orientation for Canvas APIs
		return (1.5 - fraction) * 2 * Math.PI;
		}
	
		var secondFraction = date.getSeconds() / 60;
		if (3 === mode)
			secondFraction += (date.getMilliseconds() / 1000) / 60;
		var minuteFraction = (date.getMinutes()) / 60;
		var hourFraction = (date.getHours() % 12 + minuteFraction) / 12;
		this.secondAngle = clockwiseRad(secondFraction);
		this.minuteAngle = clockwiseRad(minuteFraction);
		this.hourAngle = clockwiseRad(hourFraction);
	};
	
	// book keeping so that we can easily animate the two hands for the watchface
	// .scale/.angle are updated by tween/event handler (see below)
	var renderState = {
		seconds: 0,
		second: {style: 'black', scale: 0.80, angle: 0, lineWidth: 3},
		minute: {style: 'black', scale: 0.80, angle: 0},
		hour: {style: 'black', scale: 0.51, angle: 0}
	};
	
	// helper function for the draw function (see below)
	// extracted as a standalone function to satisfy common believe in efficient JS code
	// TODO: verify that this has actually any effect on byte code level
	var drawHand = function(handState, ctx, cx, cy, maxRadius) {
		ctx.lineWidth = handState.lineWidth ?? 8;
		ctx.strokeStyle = handState.style;
		ctx.beginPath();
		ctx.moveTo(cx, cy);
		ctx.lineTo(cx + Math.sin(handState.angle) * handState.scale * maxRadius,
					cy + Math.cos(handState.angle) * handState.scale * maxRadius);
		ctx.stroke();
	};
	
	// the 'draw' event is being emitted after each call to rocky.requestDraw() but
	// at most once for each screen update, even if .requestDraw() is called frequently
	// the 'draw' event might also fire at other meaningful times (e.g. upon launch)
	rocky.on('draw', function(drawEvent) {
		if (0) {
			reader.next();

			Timer.schedule(reader.timer, reader.frameDuration, reader.frameDuration);

			render.begin(x + reader.frameX, y + reader.frameY, reader.frameWidth, reader.frameHeight);
				render.fillRectangle(white, 0, 0, render.width, render.height);
				render.drawMonochrome(reader, black, white, x, y);
			render.end();
			return;
		}

		if (1) {
			render.begin();
				render.fillRectangle(white, 0, 0, render.width, render.height);
				render.fillRectangle(black, 20, 20, render.width - 40, render.height - 40);
				render.fillRectangle(white, 40, 40, render.width - 80, render.height - 80);
				render.drawPixel(black, render.width >> 1, render.height >> 1);
				// render.drawMonochrome(envelope, black, white, 14, 10)
				render.drawMonochrome(icon, black, white, 40, 60 /* , 0, 0, 72, 72 */);
				render.fillPattern(icon, 0, 0, render.width, render.height, 0, 0, 72, 72);
				render.fillRectangle(black, 0, 0, render.width, pebbleFont.height + 2);
				render.drawText("This is a gTest!?*$", pebbleFont, white, 0, 0);

				// render.drawBitmap(pebbleIcon, render.width - pebbleIcon.width, render.height - pebbleIcon.height, 10, 10, pebbleIcon.width - 10, pebbleIcon.height - 10);
				render.drawLine(0, 0, render.width, render.height, black, 5);
				render.drawLine(render.width, 0, 0, render.height, black, 5);
				render.drawLine(0, 0, render.width, render.height, white);
				render.drawLine(render.width, 0, 0, render.height, white);
			render.end();
			return;
		}
		if (3 === mode) {
			var wfh = new WatchfaceHelper;
			renderState.second.angle = wfh.secondAngle;
			Timer.set(() => rocky.requestDraw());		// requestDraw ignored if called from draw event
		}
		var ctx = drawEvent.context;
		var w = ctx.canvas.clientWidth;
		var h = ctx.canvas.clientHeight;
		// clear canvas on each render
		ctx.fillStyle = 'white';
		// ctx.fillRect(0, 0, w, h);
	
		// center point
		var cx = w / 2;
		var cy = h / 2;
		var maxRadius = Math.min(w, h - 2 * 10) / 2;
		drawHand(renderState.minute, ctx, cx, cy, maxRadius);
		drawHand(renderState.hour, ctx, cx, cy, maxRadius);
		if ((2 === mode) || (3 === mode))
			drawHand(renderState.second, ctx, cx, cy, maxRadius);
		// overdraw center so that no white part of the minute hand is visible
		drawHand({style: 'black', scale: 0, angle: 0}, ctx, cx, cy, 0);
	
		if (status) {
			ctx.font = '14px bold Gothic';
			ctx.textAlign = 'center';
			ctx.fillStyle = 'black';
			ctx.fillText(status, cx, 0, w);
		}
		else {
			// Draw a 12 o clock indicator
			drawHand({style: 'black', scale: 0, angle: 0}, ctx, cx, 8, 0);
		}
	
		if (1 === mode) {
			ctx.font = '20px bold Leco-numbers';
			ctx.textAlign = 'center';
			ctx.fillStyle = 'black';
			ctx.fillText(renderState.seconds.toString().padStart(2, "0"), cx, h - 23, w);
		}
	});
	
	// listener is called on each full minute and once immediately after registration
	rocky.on('minutechange', function(e) {
		// WatchfaceHelper will later be extracted as npm module
		var wfh = new WatchfaceHelper(e.date);
		renderState.minute.angle = wfh.minuteAngle;
		renderState.hour.angle = wfh.hourAngle;
		if (mode === 0)
			rocky.requestDraw();
	});
	
	rocky.on('secondchange', function(e) {
		var wfh = new WatchfaceHelper(e.date);
		renderState.seconds = e.date.getSeconds();
		renderState.second.angle = wfh.secondAngle;
		if (0 !== mode) 
			rocky.requestDraw();
	});

	let d = new Date;
	d.toTimeString();
}