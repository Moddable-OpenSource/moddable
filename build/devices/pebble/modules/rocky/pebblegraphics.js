import Timer from "timer";

class Context @ "pebble_graphics_context_destructor" {
	constructor(options) @ "pebble_graphics_context"
	get fillStyle() @ "pebble_graphics_context_get_fillStyle"
	set fillStyle() @ "pebble_graphics_context_set_fillStyle"
	get strokeStyle() @ "pebble_graphics_context_get_strokeStyle"
	set strokeStyle() @ "pebble_graphics_context_set_strokeStyle"
	get lineWidth() @ "pebble_graphics_context_get_lineWidth"
	set lineWidth() @ "pebble_graphics_context_set_lineWidth"
	clearRect() @ "pebble_graphics_context_clearRect"
	fillRect() @ "pebble_graphics_context_fillRect"
	rockyFillRadial() @ "pebble_graphics_context_rockyFillRadial"
	drawImage() @ "pebble_graphics_context_drawImage"
	beginPath() @ "pebble_graphics_context_beginPath"
	closePath() @ "pebble_graphics_context_closePath"
	moveTo(x, y) @ "pebble_graphics_context_moveTo"
	lineTo(x, y) @ "pebble_graphics_context_lineTo"
	arc() @  "pebble_graphics_context_arc"
	rect() @  "pebble_graphics_context_rect"
	stroke() @ "pebble_graphics_context_stroke"
	fill() @ "pebble_graphics_context_fill"
	save() @ "pebble_graphics_context_save"
	restore() @ "pebble_graphics_context_restore"

	fillText() @ "pebble_graphics_context_fillText"
	measureText() @ "pebble_graphics_context_measureText"
	get font() @ "pebble_graphics_context_get_font"
	set font() @ "pebble_graphics_context_set_font"
	get textAlign() @ "pebble_graphics_context_get_textAlign"
	set textAlign() @ "pebble_graphics_context_set_textAlign"

	// extended
	get dirty() @ "pebble_graphics_context_get_dirty"
	set dirty(value) @ "pebble_graphics_context_set_dirty"
}

//@@ canvas.ctx = parent context
class Canvas @ "pebble_graphics_canvas_destructor" {
	get clientWidth() @ "pebble_graphics_canvas_get_clientWidth"
	get clientHeight() @ "pebble_graphics_canvas_get_clientHeight"
	get unobstructedWidth() @ "pebble_graphics_canvas_get_unobstructedWidth"
	get unobstructedHeight() @ "pebble_graphics_canvas_get_unobstructedHeight"
}
//    layer_get_unobstructed_bounds(layer, &uo_rect);
//#define ROCKY_CANVAS_UNOBSTRUCTEDLEFT "unobstructedLeft"
//#define ROCKY_CANVAS_UNOBSTRUCTEDTOP "unobstructedTop"

function setSystemResources() @ "pebble_system_setResources";


export class Rocky {
	static events = Object.freeze([
		"draw",
		"secondchange",
		"minutechange",
		"minutechange",
		"daychange"
	]);
	#events = new Map;
	#ctx = new Context({
		onUpdate: () => {
			this.do("draw", {context: this.#ctx});				
		}
	});
	#timeChange;

	constructor(options) {
		this.#ctx.canvas = new Canvas;
		this.#ctx.canvas.context = this;

		this.#ctx.dirty = true;
	}
	on(event, callback) {
		if (!Rocky.events.includes(event))
			return;

		if (!this.#events.has(event))
			this.#events.set(event, [callback]);
		else
			this.#events.get(event).push(callback);

		const seconds = this.#events.has("secondchange"), minutes = this.#events.has("minutechange"),
						hours = this.#events.has("hourchange"), days = this.#events.has("daychange")  
		if (!seconds && !minutes && !hours && !days) {
			Timer.clear(this.#timeChange);
			this.#timeChange = undefined;
		}
		else if (seconds) {
			this.#timeChange ??= Timer.repeat(() => this.#tick(), 1000);
			Timer.schedule(this.#timeChange, 1000 - (Date.now() % 1000), 1000);
		}
		else if (minutes) {
			this.#timeChange ??= Timer.repeat(() => this.#tick(), 1000);
			Timer.schedule(this.#timeChange, 1000 * 60 - (Date.now() % (1000 * 60)), 1000 * 60);
		}
		else if (hours) {
			this.#timeChange ??= Timer.repeat(() => this.#tick(), 1000);
			Timer.schedule(this.#timeChange, 1000 * 60 * 60 - (Date.now() % (1000 * 60 * 60)), 1000 * 60 * 60);
		}
		else if (days) {
		//@@
		}

		// should be a deferred callback, but this is what it does....
		if ("draw" === event)
			callback({context: this.#ctx});
		else
			callback({date: new Date});
	}
	off(event, callback) {
		throw new Error("unimplemented");
	}
	requestDraw() {
		this.#ctx.dirty = true;
	}
	do(event, arg) {
		try {
			this.#events.get(event)?.forEach(cb => cb(arg));
		}
		catch (e) {
			trace(`Rocky.do("${event}") exception: ${e.toString()}\n`);
			trace(e.stack, "\n");
		}
	}
	getResource(id) @ "pebble_system_getResource"
	getBitmap(id) @ "pebble_system_getBitmap"
	#tick() {
		const now = new Date;
		if (this.#events.has("secondchange"))
			this.do("secondchange", {date: new Date(now)});
		if (this.#events.has("minutechange") && !now.getSeconds())
			this.do("minutechange", {date: new Date(now)});
		if (this.#events.has("hourchange") && !now.getSeconds() && !now.getMinutes())
			this.do("hourchange", {date: new Date(now)});
//@@		if (this.#events.has("daychange") && !now.getSeconds() && !now.getMinutes())
//@@			this.do("daychange", {date: new Date(now)});
			
	}
}
