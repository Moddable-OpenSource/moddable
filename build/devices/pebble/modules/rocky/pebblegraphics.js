import Timer from "timer";

class Context extends Native("pebble_graphics_context_destructor") {
	constructor(options) { super(); native("pebble_graphics_context").call(this, options); }
	get fillStyle() { return native("pebble_graphics_context_get_fillStyle").call(this); }
	set fillStyle(value) { native("pebble_graphics_context_set_fillStyle").call(this, value); }
	get strokeStyle() { return native("pebble_graphics_context_get_strokeStyle").call(this); }
	set strokeStyle(value) { native("pebble_graphics_context_set_strokeStyle").call(this, value); }
	get lineWidth() { return native("pebble_graphics_context_get_lineWidth").call(this); }
	set lineWidth(value) { native("pebble_graphics_context_set_lineWidth").call(this, value); }
	clearRect() { return native("pebble_graphics_context_clearRect").call(this); }
	fillRect() { return native("pebble_graphics_context_fillRect").call(this); }
	rockyFillRadial() { return native("pebble_graphics_context_rockyFillRadial").call(this); }
	drawImage() { return native("pebble_graphics_context_drawImage").call(this); }
	beginPath() { return native("pebble_graphics_context_beginPath").call(this); }
	closePath() { return native("pebble_graphics_context_closePath").call(this); }
	moveTo(x, y) { return native("pebble_graphics_context_moveTo").call(this, x, y); }
	lineTo(x, y) { return native("pebble_graphics_context_lineTo").call(this, x, y); }
	arc() { return native("pebble_graphics_context_arc").call(this); }
	rect() { return native("pebble_graphics_context_rect").call(this); }
	stroke() { return native("pebble_graphics_context_stroke").call(this); }
	fill() { return native("pebble_graphics_context_fill").call(this); }
	save() { return native("pebble_graphics_context_save").call(this); }
	restore() { return native("pebble_graphics_context_restore").call(this); }

	fillText() { return native("pebble_graphics_context_fillText").call(this); }
	measureText() { return native("pebble_graphics_context_measureText").call(this); }
	get font() { return native("pebble_graphics_context_get_font").call(this); }
	set font(value) { native("pebble_graphics_context_set_font").call(this, value); }
	get textAlign() { return native("pebble_graphics_context_get_textAlign").call(this); }
	set textAlign(value) { native("pebble_graphics_context_set_textAlign").call(this, value); }

	// extended
	get dirty() { return native("pebble_graphics_context_get_dirty").call(this); }
	set dirty(value) { native("pebble_graphics_context_set_dirty").call(this, value); }
}

//@@ canvas.ctx = parent context
class Canvas extends Native("pebble_graphics_canvas_destructor") {
	get clientWidth() { return native("pebble_graphics_canvas_get_clientWidth").call(this); }
	get clientHeight() { return native("pebble_graphics_canvas_get_clientHeight").call(this); }
	get unobstructedWidth() { return native("pebble_graphics_canvas_get_unobstructedWidth").call(this); }
	get unobstructedHeight() { return native("pebble_graphics_canvas_get_unobstructedHeight").call(this); }
}
//    layer_get_unobstructed_bounds(layer, &uo_rect);
//#define ROCKY_CANVAS_UNOBSTRUCTEDLEFT "unobstructedLeft"
//#define ROCKY_CANVAS_UNOBSTRUCTEDTOP "unobstructedTop"

function setSystemResources() { return native("pebble_system_setResources").call(this); };

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
	getResource(id) { return native("pebble_system_getResource").call(this, id); }
	getBitmap(id) { return native("pebble_system_getBitmap").call(this, id); }
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
