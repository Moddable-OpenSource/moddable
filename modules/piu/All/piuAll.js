/*
 * Copyright (c) 2016-2025 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// APPEARANCE

// PiuColor.c

export function blendColors(a, c1, c2) { return native("Piu_blendColors").call(this, a, c1, c2); };
globalThis.blendColors = blendColors;
export function hsl(r, g, b) { return native("Piu_hsl").call(this, r, g, b); };
globalThis.hsl = hsl;
export function hsla(r, g, b, a) { return native("Piu_hsla").call(this, r, g, b, a); };
globalThis.hsla = hsla;
export function rgb(r, g, b) { return native("Piu_rgb").call(this, r, g, b); };
globalThis.rgb = rgb;
export function rgba(r, g, b, a) { return native("Piu_rgba").call(this, r, g, b, a); };
globalThis.rgba = rgba;

// PiuSkin.c

export class Skin extends Native("PiuSkinDelete") {
	constructor(it) { super(); native("PiuSkinCreate").call(this, it); }
	get borders() { return native("PiuSkin_get_borders").call(this); }
	get bottom() { return native("PiuSkin_get_bottom").call(this); }
	get bounds() { return native("PiuSkin_get_bounds").call(this); }
	get color() { return native("PiuSkin_get_color").call(this); }
	get fill() { return native("PiuSkin_get_fill").call(this); }
	get left() { return native("PiuSkin_get_left").call(this); }
	get right() { return native("PiuSkin_get_right").call(this); }
	get states() { return native("PiuSkin_get_states").call(this); }
	get stroke() { return native("PiuSkin_get_stroke").call(this); }
	get texture() { return native("PiuSkin_get_texture").call(this); }
	get tiles() { return native("PiuSkin_get_tiles").call(this); }
	get top() { return native("PiuSkin_get_top").call(this); }
	get variants() { return native("PiuSkin_get_variants").call(this); }
	get x() { return native("PiuSkin_get_x").call(this); }
	get y() { return native("PiuSkin_get_y").call(this); }
	get width() { return native("PiuSkin_get_width").call(this); }
	get height() { return native("PiuSkin_get_height").call(this); }
	static template(i) {
		const it = i;
		return function() {
			let skin;
			if (globalThis.assetMap)
				skin = assetMap.get(it);
			else
				globalThis.assetMap = new Map;
			if (!skin) {
				skin = new Skin(it);
				assetMap.set(it, skin);
			}
			return skin;
		}
	}
}
Object.freeze(Skin.prototype);
globalThis.Skin = Skin;

// PiuStyle.c

export class Style extends Native("PiuStyleDelete") {
	constructor(it) { super(); native("PiuStyleCreate").call(this, it); }
	get bottom() { return native("PiuStyle_get_bottom").call(this); }
	get color() { return native("PiuStyle_get_color").call(this); }
	get family() { return native("PiuStyle_get_family").call(this); }
	get horizontal() { return native("PiuStyle_get_horizontal").call(this); }
	get indentation() { return native("PiuStyle_get_indentation").call(this); }
	get leading() { return native("PiuStyle_get_leading").call(this); }
	get left() { return native("PiuStyle_get_left").call(this); }
	get margins() { return native("PiuStyle_get_margins").call(this); }
	get right() { return native("PiuStyle_get_right").call(this); }
	get size() { return native("PiuStyle_get_size").call(this); }
	get stretch() { return native("PiuStyle_get_stretch").call(this); }
	get style() { return native("PiuStyle_get_style").call(this); }
	get top() { return native("PiuStyle_get_top").call(this); }
	get vertical() { return native("PiuStyle_get_vertical").call(this); }
	get weight() { return native("PiuStyle_get_weight").call(this); }
	
	measure(string) { return native("PiuStyle_measure").call(this, string); }
	static template(i) {
		const it = i;
		return function() {
			let style;
			if (globalThis.assetMap)
				style = assetMap.get(it);
			else
				globalThis.assetMap = new Map;
			if (!style) {
				style = new Style(it);
				assetMap.set(it, style);
			}
			return style;
		}
	}
}
Object.freeze(Style.prototype);
globalThis.Style = Style;

// BEHAVIOR

export class Behavior {
}
Object.freeze(Behavior.prototype);
globalThis.Behavior = Behavior;

// DISPATCH

export function template(f) {
	let constructor = this;
	let result = function($, it1) {
		let self = (this) ? this : Object.create(constructor.prototype);
		let it = f.call(self, $, it1);
		if (it1) {
			let contents = null;
			if ("contents" in it)
				contents = it.contents;
			for (let i in it1) {
				if (contents && (i == "contents"))
					contents = contents.concat(it1.contents);
				else
					it[i] = it1[i];
			}
			if (contents)
				it.contents = contents;
		}
		return constructor.call(self, $, it);
	}
	result.prototype = this.prototype;
	result.template = template;
	return result;
}
globalThis.template = template;

export function Template(prototype) {
	const proto = prototype;
	const result = function($, it = {}) {
		let self = (this) ? this : Object.create(proto);
		self._create($, it);
		return self;
	}
	result.prototype = proto;
	result.template = template;
	return result;
}
globalThis.Template = Template;

// CONTENTS

const proto = new (Native("PiuContentDelete"));
Object.freeze(proto);

// PiuContent.c

export const Content = Template(Object.freeze({
	__proto__: proto,
	_create($, it) { native("PiuContent_create").call(this, $, it); },
	
	get active() { return native("PiuContent_get_active").call(this); },
	get application() { return native("PiuContent_get_application").call(this); },
	get backgroundTouch() { return native("PiuContent_get_backgroundTouch").call(this); },
	get behavior() { return native("PiuContent_get_behavior").call(this); },
	get bounds() { return native("PiuContent_get_bounds").call(this); },
	get container() { return native("PiuContent_get_container").call(this); },
	get coordinates() { return native("PiuContent_get_coordinates").call(this); },
	get duration() { return native("PiuContent_get_duration").call(this); },
	get exclusiveTouch() { return native("PiuContent_get_exclusiveTouch").call(this); },
	get fraction() { return native("PiuContent_get_fraction").call(this); },
	get index() { return native("PiuContent_get_index").call(this); },
	get interval() { return native("PiuContent_get_interval").call(this); },
	get loop() { return native("PiuContent_get_loop").call(this); },
	get multipleTouch() { return native("PiuContent_get_multipleTouch").call(this); },
	get name() { return native("PiuContent_get_name").call(this); },
	get next() { return native("PiuContent_get_next").call(this); },
	get offset() { return native("PiuContent_get_offset").call(this); },
	get position() { return native("PiuContent_get_position").call(this); },
	get previous() { return native("PiuContent_get_previous").call(this); },
	get running() { return native("PiuContent_get_running").call(this); },
	get size() { return native("PiuContent_get_size").call(this); },
	get skin() { return native("PiuContent_get_skin").call(this); },
	get state() { return native("PiuContent_get_state").call(this); },
	get style() { return native("PiuContent_get_style").call(this); },
	get time() { return native("PiuContent_get_time").call(this); },
	get type() { return native("PiuContent_get_type").call(this); },
	get variant() { return native("PiuContent_get_variant").call(this); },
	get visible() { return native("PiuContent_get_visible").call(this); },
	get x() { return native("PiuContent_get_x").call(this); },
	get y() { return native("PiuContent_get_y").call(this); },
	get width() { return native("PiuContent_get_width").call(this); },
	get height() { return native("PiuContent_get_height").call(this); },
	
	set active(it) { native("PiuContent_set_active").call(this, it); },
	set backgroundTouch(it) { native("PiuContent_set_backgroundTouch").call(this, it); },
	set behavior(it) { native("PiuContent_set_behavior").call(this, it); },
	set coordinates(it) { native("PiuContent_set_coordinates").call(this, it); },
	set duration(it) { native("PiuContent_set_duration").call(this, it); },
	set exclusiveTouch(it) { native("PiuContent_set_exclusiveTouch").call(this, it); },
	set fraction(it) { native("PiuContent_set_fraction").call(this, it); },
	set interval(it) { native("PiuContent_set_interval").call(this, it); },
	set loop(it) { native("PiuContent_set_loop").call(this, it); },
	set multipleTouch(it) { native("PiuContent_set_multipleTouch").call(this, it); },
	set name(it) { native("PiuContent_set_name").call(this, it); },
	set offset(it) { native("PiuContent_set_offset").call(this, it); },
	set position(it) { native("PiuContent_set_position").call(this, it); },
	set size(it) { native("PiuContent_set_size").call(this, it); },
	set skin(it) { native("PiuContent_set_skin").call(this, it); },
	set state(it) { native("PiuContent_set_state").call(this, it); },
	set style(it) { native("PiuContent_set_style").call(this, it); },
	set time(it) { native("PiuContent_set_time").call(this, it); },
	set variant(it) { native("PiuContent_set_variant").call(this, it); },
	set visible(it) { native("PiuContent_set_visible").call(this, it); },
	set x(it) { native("PiuContent_set_x").call(this, it); },
	set y(it) { native("PiuContent_set_y").call(this, it); },
	set width(it) { native("PiuContent_set_width").call(this, it); },
	set height(it) { native("PiuContent_set_height").call(this, it); },
	
	adjust(x, y) { native("PiuContent_adjust").call(this, x, y); },
	bubble(id) { native("PiuContent_bubble").call(this, id); },
	captureTouch(id, x, y, ticks) { native("PiuContent_captureTouch").call(this, id, x, y, ticks); },
	defer(id) { native("PiuContent_defer").call(this, id); },
	delegate(id) { native("PiuContent_delegate").call(this, id); },
	distribute(id) { native("PiuContent_distribute").call(this, id); },
	focus() { native("PiuContent_focus").call(this); },
	hit(x, y) { return native("PiuContent_hit").call(this, x, y); },
	measure() { return native("PiuContent_measure").call(this); },
	moveBy(x, y) { native("PiuContent_moveBy").call(this, x, y); },
	render() { native("PiuContent_render").call(this); },
	sizeBy(x, y) { native("PiuContent_sizeBy").call(this, x, y); },
	start() { native("PiuContent_start").call(this); },
	stop() { native("PiuContent_stop").call(this); },
}));
globalThis.Content = Content;

// PiuLabel.c

export const Label = Template(Object.freeze({
	__proto__: Content.prototype,
	_create($, it) { native("PiuLabel_create").call(this, $, it); },

	get string() { return native("PiuLabel_get_string").call(this); },
	
	set string(it) { native("PiuLabel_set_string").call(this, it); },
}));
globalThis.Label = Label;

// PiuText.c

export const Text = Template(Object.freeze({
	__proto__: Content.prototype,
	_create($, it) { native("PiuText_create").call(this, $, it); },

	get blocks() { return native("PiuText_get_blocks").call(this); },
	get string() { return native("PiuText_get_string").call(this); },
	
	set blocks(it) { native("PiuText_set_blocks").call(this, it); },
	set string(it) { native("PiuText_set_string").call(this, it); },
	
	begin() { native("PiuText_begin").call(this); },
	beginBlock(style, behavior) { native("PiuText_beginBlock").call(this, style, behavior); },
	beginSpan(style, behavior) { native("PiuText_beginSpan").call(this, style, behavior); },
	concat(string) { native("PiuText_concat").call(this, string); },
	end() { native("PiuText_end").call(this); },
	endBlock() { native("PiuText_endBlock").call(this); },
	endSpan() { native("PiuText_endSpan").call(this); },
}));
globalThis.Text = Text;

export const Link = Template(Object.freeze({
	__proto__: proto,
	_create($, it) { native("PiuTextLink_create").call(this, $, it); },

	get container() { return native("PiuContent_get_container").call(this); },
	get state() { return native("PiuTextLink_get_state").call(this); },

	set state(it) { native("PiuTextLink_set_state").call(this, it); },
	
	captureTouch(id, x, y, ticks) { native("PiuContent_captureTouch").call(this, id, x, y, ticks); },
}));
globalThis.Link = Link;

// PiuPort.c

export const Port = Template(Object.freeze({
	__proto__: Content.prototype,
	_create($, it) { native("PiuPort_create").call(this, $, it); },
	
	set skin(it) { native("PiuPort_set_skin").call(this, it); },
	set state(it) { native("PiuPort_set_state").call(this, it); },
	set variant(it) { native("PiuPort_set_variant").call(this, it); },

	drawContent(x, y, w, h) { native("PiuPort_drawContent").call(this, x, y, w, h); },
	drawLabel(string, x, y, w, h) { native("PiuPort_drawLabel").call(this, string, x, y, w, h); },
	drawSkin(skin, x, y, w, h, variant, state) { native("PiuPort_drawSkin").call(this, skin, x, y, w, h, variant, state); },
	drawString(string, style, color, x, y, w) { native("PiuPort_drawString").call(this, string, style, color, x, y, w); },
	drawStyle(string, style, x, y, w, h, ellipsis, state) { native("PiuPort_drawStyle").call(this, string, style, x, y, w, h, ellipsis, state); },
	drawTexture(texture, color, x, y, sx, sy, sw, sh) { native("PiuPort_drawTexture").call(this, texture, color, x, y, sx, sy, sw, sh); },
	fillColor(color, x, y, w, h) { native("PiuPort_fillColor").call(this, color, x, y, w, h); },
	fillTexture(texture, color, x, y, w, h, sx, sy, sw, sh) { native("PiuPort_fillTexture").call(this, texture, color, x, y, w, h, sx, sy, sw, sh); },
	invalidate(x, y, w, h) { native("PiuPort_invalidate").call(this, x, y, w, h); },
	popClip() { native("PiuPort_popClip").call(this); },
	pushClip(x, y, w, h) { native("PiuPort_pushClip").call(this, x, y, w, h); },
	measureString(string, style) { return native("PiuPort_measureString").call(this, string, style); },
}));
globalThis.Port = Port;

// CONTAINERS

// PiuContainer.c

export const Container = Template(Object.freeze({
	__proto__: Content.prototype,
	_create($, it) { native("PiuContainer_create").call(this, $, it); },
	_recurse(it) {
		if (it) {
			if (it instanceof Array)
				it.forEach(this._recurse, this);
			else if (it instanceof Content)
				this.add(it);
			else
				throw new ReferenceError("No contents!"); 
		}
	},
	
	get clip() { return native("PiuContainer_get_clip").call(this); },
	get first() { return native("PiuContainer_get_first").call(this); },
	get last() { return native("PiuContainer_get_last").call(this); },
	get length() { return native("PiuContainer_get_length").call(this); },
	get transitioning() { return native("PiuContainer_get_transitioning").call(this); },	
	
	set clip(it) { native("PiuContainer_set_clip").call(this, it); },
	
	add(content) { native("PiuContainer_add").call(this, content); },
	content(at) { return native("PiuContainer_content").call(this, at); },
	empty(start, stop) { native("PiuContainer_empty").call(this, start, stop); },
	firstThat(id) { return native("PiuContainer_firstThat").call(this, id); },
	insert(content, before) { native("PiuContainer_insert").call(this, content, before); },
	lastThat(id) { return native("PiuContainer_lastThat").call(this, id); },
	remove(content) { native("PiuContainer_remove").call(this, content); },
	replace(content, by) { native("PiuContainer_replace").call(this, content, by); },
	run(transition) { native("PiuContainer_run").call(this, transition); },
	swap(content0, content1) { native("PiuContainer_swap").call(this, content0, content1); },
}));
globalThis.Container = Container;

// PiuColumn.c

export const Column = Template(Object.freeze({
	__proto__: Container.prototype,
	_create($, it) { native("PiuColumn_create").call(this, $, it); },
}));
globalThis.Column = Column;

// PiuLayout.c

export const Layout = Template(Object.freeze({
	__proto__: Container.prototype,
	_create($, it) { native("PiuLayout_create").call(this, $, it); },
}));
globalThis.Layout = Layout;

// PiuRow.c

export const Row = Template(Object.freeze({
	__proto__: Container.prototype,
	_create($, it) { native("PiuRow_create").call(this, $, it); },
}));
globalThis.Row = Row;

// PiuScroller.c

export const Scroller = Template(Object.freeze({
	__proto__: Container.prototype,
	_create($, it) { native("PiuScroller_create").call(this, $, it); },

	get constraint() { return native("PiuScroller_get_constraint").call(this); },
	get looping() { return native("PiuScroller_get_looping").call(this); },
	get scroll() { return native("PiuScroller_get_scroll").call(this); },
	get tracking() { return native("PiuScroller_get_tracking").call(this); },

	set looping(it) { native("PiuScroller_set_looping").call(this, it); },
	set scroll(it) { native("PiuScroller_set_scroll").call(this, it); },
	set tracking(it) { native("PiuScroller_set_tracking").call(this, it); },

	reveal(bounds) { native("PiuScroller_reveal").call(this, bounds); },
	scrollBy(dx, dy) { native("PiuScroller_scrollBy").call(this, dx, dy); },
	scrollTo(x, y) { native("PiuScroller_scrollTo").call(this, x, y); },
}));
globalThis.Scroller = Scroller;

// DEFER

export class DeferLink extends Native("PiuDeferLinkDelete") {
	constructor() { super(); native("PiuDeferLinkCreate").call(this); }
}
Object.freeze(DeferLink.prototype);
globalThis.DeferLink = DeferLink;

// TOUCH

export class TouchLink extends Native("PiuTouchLinkDelete") {
	constructor() { super(); native("PiuTouchLinkCreate").call(this); }
	get length() { return native("PiuTouchLink_get_length").call(this); }
	peek(index) { return native("PiuTouchLink_peek").call(this, index); }
}
Object.freeze(TouchLink.prototype);
globalThis.TouchLink = TouchLink;

// TRANSITION

// PiuTransition.c

export class Transition extends Native("PiuTransitionDelete") {
	constructor(it) { super(); native("PiuTransitionCreate").call(this, it); }
	
	get duration() { return native("PiuTransition_get_duration").call(this); }
	
	set duration(it) { native("PiuTransition_set_duration").call(this, it); }
	
	onBegin(/* container */) {
	}
	onEnd(/* container */) {
	}
	onStep(/* fraction */) {
	}
}
Object.freeze(Transition.prototype);
globalThis.Transition = Transition;

// LOCALS

// PiuLocals.c

export class Locals  extends Native("PiuLocalsDelete") {
	constructor(name, language) { super(); native("PiuLocalsCreate").call(this, name, language); }
	get language() { return native("PiuLocals_get_language").call(this); }
	set language(it) { native("PiuLocals_set_language").call(this, it); }
	get() { return native("PiuLocals_get").call(this); }
}
Object.freeze(Locals.prototype);
globalThis.Locals = Locals;
