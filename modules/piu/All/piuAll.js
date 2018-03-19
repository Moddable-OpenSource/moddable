/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

export function blendColors(a, c1, c2) @ "Piu_blendColors";
export function hsl(r, g, b) @ "Piu_hsl";
export function hsla(r, g, b, a) @ "Piu_hsla";
export function rgb(r, g, b) @ "Piu_rgb";
export function rgba(r, g, b, a) @ "Piu_rgba";

// PiuSkin.c

export class Skin @ "PiuSkinDelete" {
	constructor(it) @ "PiuSkinCreate"
	get borders() @ "PiuSkin_get_borders"
	get bottom() @ "PiuSkin_get_bottom"
	get bounds() @ "PiuSkin_get_bounds"
	get color() @ "PiuSkin_get_color"
	get fill() @ "PiuSkin_get_fill"
	get left() @ "PiuSkin_get_left"
	get right() @ "PiuSkin_get_right"
	get states() @ "PiuSkin_get_states"
	get stroke() @ "PiuSkin_get_stroke"
	get texture() @ "PiuSkin_get_texture"
	get tiles() @ "PiuSkin_get_tiles"
	get top() @ "PiuSkin_get_top"
	get variants() @ "PiuSkin_get_variants"
	get x() @ "PiuSkin_get_x"
	get y() @ "PiuSkin_get_y"
	get width() @ "PiuSkin_get_width"
	get height() @ "PiuSkin_get_height"
	static template(it) {
		return function() {
			let map = global.assetMap;
			let skin;
			if (map)
				skin = map.get(it);
			else
				map = global.assetMap = new Map;
			if (!skin) {
				skin = new Skin(it);
				map.set(it, skin);
			}
			return skin;
		}
	}
}
Object.freeze(Skin.prototype);

// PiuStyle.c

export class Style @ "PiuStyleDelete" {
	constructor(it) @ "PiuStyleCreate"
	get bottom() @ "PiuStyle_get_bottom"
	get color() @ "PiuStyle_get_color"
	get family() @ "PiuStyle_get_family"
	get horizontal() @ "PiuStyle_get_horizontal"
	get indentation() @ "PiuStyle_get_indentation"
	get leading() @ "PiuStyle_get_leading"
	get left() @ "PiuStyle_get_left"
	get margins() @ "PiuStyle_get_margins"
	get right() @ "PiuStyle_get_right"
	get size() @ "PiuStyle_get_size"
	get stretch() @ "PiuStyle_get_stretch"
	get style() @ "PiuStyle_get_style"
	get top() @ "PiuStyle_get_top"
	get vertical() @ "PiuStyle_get_vertical"
	get weight() @ "PiuStyle_get_weight"
	
	measure(string) @ "PiuStyle_measure"
	static template(it) {
		return function() {
			let map = global.assetMap;
			let style;
			if (map)
				style = map.get(it);
			else
				map = global.assetMap = new Map;
			if (!style) {
				style = new Style(it);
				map.set(it, style);
			}
			return style;
		}
	}
}
Object.freeze(Style.prototype);

// BEHAVIOR

export class Behavior {
	constructor() {
	}
}
Object.freeze(Behavior.prototype);

class DebugBehavior extends Behavior{
	onAdapt(content) {
		debugger
	}
	onCreate(content, $, it) {
		debugger
	}
	onDisplaying(content) {
		debugger
	}
	onDraw(view, x, y, width, height) {
		debugger
	}
	onFinished(content) {
		debugger
	}
	onFitHorizontally(layout, width) {
		debugger
	}
	onFitVertically(layout, height) {
		debugger
	}
	onFocused(content) {
		debugger
	}
	onKeyDown(content, key) {
		debugger
	}
	onKeyUp(content, key) {
		debugger
	}
	onMeasureHorizontally(layout, width) {
		debugger
		return width;
	}
	onMeasureVertically(layout, height) {
		debugger
		return height;
	}
	onMessage(application, json) {
		debugger
	}
	onMouseEntered(content, x, y) {
		debugger
	}
	onMouseExited(content, x, y) {
		debugger
	}
	onMouseMoved(content, x, y) {
		debugger
	}
	onMouseScrolled(content, dx, dy) {
		debugger
	}
	onScrolled(content) {
		debugger
	}
	onTimeChanged(content) {
		debugger
	}
	onTouchBegan(content, index, x, y, ticks) {
		debugger
	}
	onTouchCancelled(content, index, x, y, ticks) {
		debugger
	}
	onTouchEnded(content, index, x, y, ticks) {
		debugger
	}
	onTouchMoved(content, index, x, y, ticks) {
		debugger
	}
	onTransitionBeginning(container) {
		debugger
	}
	onTransitionEnded(container) {
		debugger
	}
	onUnfocused(content) {
		debugger
	}
}
Object.freeze(DebugBehavior.prototype);

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

export function Template(prototype) {
	let result = function($, it = {}) {
		let self = (this) ? this : Object.create(prototype);
		self._create($, it);
		return self;
	}
	result.prototype = prototype;
	result.template = template;
	return result;
}

// CONTENTS

var proto = @ "PiuContentDelete";
Object.freeze(proto);

// PiuContent.c

var content = {
	__proto__: proto,
	_create($, it) @ "PiuContent_create",
	
	get active() @ "PiuContent_get_active",
	get application() @ "PiuContent_get_application",
	get backgroundTouch() @ "PiuContent_get_backgroundTouch",
	get behavior() @ "PiuContent_get_behavior",
	get bounds() @ "PiuContent_get_bounds",
	get container() @ "PiuContent_get_container",
	get coordinates() @ "PiuContent_get_coordinates",
	get duration() @ "PiuContent_get_duration",
	get exclusiveTouch() @ "PiuContent_get_exclusiveTouch",
	get fraction() @ "PiuContent_get_fraction",
	get index() @ "PiuContent_get_index",
	get interval() @ "PiuContent_get_interval",
	get loop() @ "PiuContent_get_loop",
	get multipleTouch() @ "PiuContent_get_multipleTouch",
	get name() @ "PiuContent_get_name",
	get next() @ "PiuContent_get_next",
	get offset() @ "PiuContent_get_offset",
	get position() @ "PiuContent_get_position",
	get previous() @ "PiuContent_get_previous",
	get running() @ "PiuContent_get_running",
	get size() @ "PiuContent_get_size",
	get skin() @ "PiuContent_get_skin",
	get state() @ "PiuContent_get_state",
	get style() @ "PiuContent_get_style",
	get time() @ "PiuContent_get_time",
	get variant() @ "PiuContent_get_variant",
	get visible() @ "PiuContent_get_visible",
	get x() @ "PiuContent_get_x",
	get y() @ "PiuContent_get_y",
	get width() @ "PiuContent_get_width",
	get height() @ "PiuContent_get_height",
	
	set active(it) @ "PiuContent_set_active",
	set backgroundTouch(it) @ "PiuContent_set_backgroundTouch",
	set behavior(it) @ "PiuContent_set_behavior",
	set coordinates(it) @ "PiuContent_set_coordinates",
	set duration() @ "PiuContent_set_duration",
	set exclusiveTouch(it) @ "PiuContent_set_exclusiveTouch",
	set fraction() @ "PiuContent_set_fraction",
	set interval() @ "PiuContent_set_interval",
	set loop(it) @ "PiuContent_set_loop",
	set multipleTouch(it) @ "PiuContent_set_multipleTouch",
	set name(it) @ "PiuContent_set_name",
	set offset(it) @ "PiuContent_set_offset",
	set position(it) @ "PiuContent_set_position",
	set size(it) @ "PiuContent_set_size",
	set skin(it) @ "PiuContent_set_skin",
	set state(it) @ "PiuContent_set_state",
	set style(it) @ "PiuContent_set_style",
	set time() @ "PiuContent_set_time",
	set variant(it) @ "PiuContent_set_variant",
	set visible(it) @ "PiuContent_set_visible",
	set x(it) @ "PiuContent_set_x",
	set y(it) @ "PiuContent_set_y",
	set width(it) @ "PiuContent_set_width",
	set height(it) @ "PiuContent_set_height",
	
	adjust(x, y) @ "PiuContent_adjust",
	bubble(id) @ "PiuContent_bubble",
	captureTouch(id, x, y, ticks) @ "PiuContent_captureTouch",
	defer(id) @ "PiuContent_defer",
	delegate(id) @ "PiuContent_delegate",
	distribute(id) @ "PiuContent_distribute",
	focus() @ "PiuContent_focus",
	hit(x, y) @ "PiuContent_hit",
	measure() @ "PiuContent_measure",
	moveBy(x, y) @ "PiuContent_moveBy",
	render() @ "PiuContent_render",
	sizeBy(x, y) @ "PiuContent_sizeBy",
	start() @ "PiuContent_start",
	stop() @ "PiuContent_stop",
};
export var Content = Template(content);
Object.freeze(content);

// PiuLabel.c

var label = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuLabel_create",

	get string() @ "PiuLabel_get_string",
	
	set string(it) @ "PiuLabel_set_string",
};
export var Label = Template(label);
Object.freeze(label);

// PiuText.c

var text = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuText_create",

	get blocks() @ "PiuText_get_blocks",
	get string() @ "PiuText_get_string",
	
	set blocks(it) @ "PiuText_set_blocks",
	set string(it) @ "PiuText_set_string",
	
	begin() @ "PiuText_begin",
	beginBlock(style, behavior) @ "PiuText_beginBlock",
	beginSpan(style, behavior) @ "PiuText_beginSpan",
	concat(string) @ "PiuText_concat",
	end() @ "PiuText_end",
	endBlock() @ "PiuText_endBlock",
	endSpan() @ "PiuText_endSpan",
};
export var Text = Template(text);
Object.freeze(text);

var link = {
	__proto__: proto,
	_create($, it) @ "PiuTextLink_create",

	get container() @ "PiuContent_get_container",
	get state() @ "PiuTextLink_get_state",

	set state(it) @ "PiuTextLink_set_state",
	
	captureTouch(id, x, y, ticks) @ "PiuContent_captureTouch",
}
export var Link = Template(link);
Object.freeze(link);

// PiuPort.c

var port = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuPort_create",
	
	set skin(it) @ "PiuPort_set_skin",
	set state(it) @ "PiuPort_set_state",
	set variant(it) @ "PiuPort_set_variant",

	drawContent(x, y, w, h) @ "PiuPort_drawContent",
	drawLabel(string, x, y, w, h) @ "PiuPort_drawLabel",
	drawSkin(skin, x, y, w, h, variant, state) @ "PiuPort_drawSkin",
	drawString(string, style, color, x, y, w) @ "PiuPort_drawString",
	drawStyle(string, style, x, y, w, h, ellipsis, state) @ "PiuPort_drawStyle",
	drawTexture(texture, color, x, y, sx, sy, sw, sh) @ "PiuPort_drawTexture",
	fillColor(color, x, y, w, h) @ "PiuPort_fillColor",
	fillTexture(texture, color, x, y, w, h, sx, sy, sw, sh) @ "PiuPort_fillTexture",
	invalidate(x, y, w, h) @ "PiuPort_invalidate",
	popClip() @ "PiuPort_popClip",
	pushClip(x, y, w, h) @ "PiuPort_pushClip",
	measureString(string, style) @ "PiuPort_measureString",
};
export var Port = Template(port);
Object.freeze(port);

// CONTAINERS

// PiuContainer.c

var container = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuContainer_create",
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
	
	get clip() @ "PiuContainer_get_clip",
	get first() @ "PiuContainer_get_first",
	get last() @ "PiuContainer_get_last",
	get length() @ "PiuContainer_get_length",
	get transitioning() @ "PiuContainer_get_transitioning",	
	
	set clip(it) @ "PiuContainer_set_clip",
	
	add(content) @ "PiuContainer_add",
	content(at) @ "PiuContainer_content",
	empty(start, stop) @ "PiuContainer_empty",
	firstThat(id) @ "PiuContainer_firstThat",
	insert(content, before) @ "PiuContainer_insert",
	lastThat(id) @ "PiuContainer_lastThat",
	remove(content) @ "PiuContainer_remove",
	replace(content, by) @ "PiuContainer_replace",
	run(transition) @ "PiuContainer_run",
	swap(content0, content1) @ "PiuContainer_swap",
};
export var Container = Template(container);
Object.freeze(container);

// PiuColumn.c

var column = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuColumn_create",
};
export var Column = Template(column);
Object.freeze(column);

// PiuLayout.c

var layout = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuLayout_create",
};
export var Layout = Template(layout);
Object.freeze(layout);

// PiuRow.c

var row = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuRow_create",
};
export var Row = Template(row);
Object.freeze(row);

// PiuScroller.c

var scroller = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuScroller_create",
	
	get constraint() @ "PiuScroller_get_constraint",
	get loop() @ "PiuScroller_get_loop",
	get scroll() @ "PiuScroller_get_scroll",
	get tracking() @ "PiuScroller_get_tracking",
	
	set loop(it) @ "PiuScroller_set_loop",
	set scroll(it) @ "PiuScroller_set_scroll",
	set tracking(it) @ "PiuScroller_set_tracking",
	
	reveal(bounds) @ "PiuScroller_reveal",
	scrollBy(dx, dy) @ "PiuScroller_scrollBy",
	scrollTo(x, y) @ "PiuScroller_scrollTo",
};
export var Scroller = Template(scroller);
Object.freeze(scroller);

// DEFER

export class DeferLink @ "PiuDeferLinkDelete" {
	constructor() @ "PiuDeferLinkCreate"
}
Object.freeze(DeferLink.prototype);

// TOUCH

export class TouchLink @ "PiuTouchLinkDelete" {
	constructor() @ "PiuTouchLinkCreate"
	get length() @ "PiuTouchLink_get_length"
	peek(index) @ "PiuTouchLink_peek"
}
Object.freeze(TouchLink.prototype);

// TRANSITION

// PiuTransition.c

export class Transition @ "PiuTransitionDelete" {
	constructor(it) @ "PiuTransitionCreate"
	
	get duration() @ "PiuTransition_get_duration"
	
	set duration(it) @ "PiuTransition_set_duration"
	
	onBegin(container) {
	}
	onEnd(container) {
	}
	onStep(fraction) {
	}
}
Object.freeze(Transition.prototype);

Math.backEaseIn = function(fraction) @ "Math_backEaseIn";
Math.backEaseInOut = function(fraction) @ "Math_backEaseInOut";
Math.backEaseOut = function(fraction) @ "Math_backEaseOut";
Math.bounceEaseIn = function(fraction) @ "Math_bounceEaseIn";
Math.bounceEaseInOut = function(fraction) @ "Math_bounceEaseInOut";
Math.bounceEaseOut = function(fraction) @ "Math_bounceEaseOut";
Math.circularEaseIn = function(fraction) @ "Math_circularEaseIn";
Math.circularEaseInOut = function(fraction) @ "Math_circularEaseInOut";
Math.circularEaseOut = function(fraction) @ "Math_circularEaseOut";
Math.cubicEaseIn = function(fraction) @ "Math_cubicEaseIn";
Math.cubicEaseInOut = function(fraction) @ "Math_cubicEaseInOut";
Math.cubicEaseOut = function(fraction) @ "Math_cubicEaseOut";
Math.elasticEaseIn = function(fraction) @ "Math_elasticEaseIn";
Math.elasticEaseInOut = function(fraction) @ "Math_elasticEaseInOut";
Math.elasticEaseOut = function(fraction) @ "Math_elasticEaseOut";
Math.exponentialEaseIn = function(fraction) @ "Math_exponentialEaseIn";
Math.exponentialEaseInOut = function(fraction) @ "Math_exponentialEaseInOut";
Math.exponentialEaseOut = function(fraction) @ "Math_exponentialEaseOut";
Math.quadEaseIn = function(fraction) @ "Math_quadEaseIn";
Math.quadEaseInOut = function(fraction) @ "Math_quadEaseInOut";
Math.quadEaseOut = function(fraction) @ "Math_quadEaseOut";
Math.quartEaseIn = function(fraction) @ "Math_quartEaseIn";
Math.quartEaseInOut = function(fraction) @ "Math_quartEaseInOut";
Math.quartEaseOut = function(fraction) @ "Math_quartEaseOut";
Math.quintEaseIn = function(fraction) @ "Math_quintEaseIn";
Math.quintEaseInOut = function(fraction) @ "Math_quintEaseInOut";
Math.quintEaseOut = function(fraction) @ "Math_quintEaseOut";
Math.sineEaseIn = function(fraction) @ "Math_sineEaseIn";
Math.sineEaseInOut = function(fraction) @ "Math_sineEaseInOut";
Math.sineEaseOut = function(fraction) @ "Math_sineEaseOut";

