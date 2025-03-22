const BLACK = "black";
const WHITE = "white";
const backgroundSkin = new Skin({ fill: BLACK });
const colorSkin = new Skin({ texture: { path:"color.png" }, x:0, y:0, width:200, height:200 });
const texture =  new Texture({ path:"brightness.png" });
const brightnessSkin = new Skin({ texture, color:BLACK, x:0, y:0, width:200, height:40 });
const sliderSkin = new Skin({ texture, color:BLACK, x:0, y:40, width:200, height:40, left:40, right:40 });
const thumbSkin = new Skin({ texture, color:[BLACK,BLACK,WHITE], x:0, y:80, width:40, height:40, variants:40 });

function HSVtoRGB(h, s, v) {
    var r, g, b, i, f, p, q, t;
    if (arguments.length === 1) {
        s = h.s, v = h.v, h = h.h;
    }
    h /= 360;
    i = Math.floor(h * 6);
    f = h * 6 - i;
    p = v * (1 - s);
    q = v * (1 - f * s);
    t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    return {
        r: Math.round(r * 255),
        g: Math.round(g * 255),
        b: Math.round(b * 255)
    };
}

class ApplicationBehavior extends Behavior {
	onCreate(application, data) {
		this.hsv = { h:0, s:0, v:1 };
		this.led = new Host.LED.Default;
	}
	onDisplaying(application) {
		this.onColorChanging(application);
	}
	onColorChanging(application, h, s, v) {
		if (h !== undefined)
			this.hsv.h = h;
		if (s !== undefined)
			this.hsv.s = s;
		if (v !== undefined)
			this.hsv.v = v;
		const color = HSVtoRGB(this.hsv);
		application.distribute("onUpdateColor", this.hsv, rgb(color.r, color.g, color.b));
    	const led = this.led;
    	led.setPixel(0, led.makeRGB(color.r, color.g, color.b));
    	led.update();
	}
    toHex(val) {
    	return val.toString(16).padStart(2, 0).toUpperCase();
    }
}

class ColorSliderBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.onTouchMoved(container, id, x, y, ticks);
	}
	onTouchMoved(container, id, x, y, ticks) {
		const data = this.data;
		const thumb = container.last;
		const thumbRadius = thumb.width >> 1;
		const wheelRadius = container.width >> 1;
		const r = wheelRadius - thumbRadius;
		const cx = container.x + wheelRadius;
		const cy = container.y + wheelRadius;
		x -= cx;
		y -= cy;
		let a = Math.atan2(y, x);
		let d = Math.sqrt(x ** 2 + y ** 2);
		if (d > r) {
			x = Math.cos(a) * r;
			y = Math.sin(a) * r;
			d = r;
		}
		thumb.x = cx + x - thumbRadius;
		thumb.y = cy + y - thumbRadius;
		a = -a * 180 / Math.PI;
		if (a < 0)
			a += 360;
		container.bubble("onColorChanging", a, d / r, undefined, undefined);
	}
	onTouchEnded(container, id, x, y, ticks) {
		container.bubble("onColorChanged");
	}
	onUpdateColor(container, hsv, color) {
		const thumb = container.last;
		const thumbRadius = thumb.width >> 1;
		const wheelRadius = container.width >> 1;
		const r = wheelRadius - thumbRadius;
		const cx = container.x + wheelRadius;
		const cy = container.y + wheelRadius;
		const a = -hsv.h * Math.PI / 180;
		const d = hsv.s * r;
		const x = Math.cos(a) * d;
		const y = Math.sin(a) * d;
		thumb.x = cx + x - thumbRadius;
		thumb.y = cy + y - thumbRadius;
	}
}

class BrightnessSliderBehavior extends Behavior {
	onFractionChanging(container, fraction) {
		container.bubble("onColorChanging", undefined, undefined, Math.sqrt(fraction), undefined);
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.onTouchMoved(container, id, x, y, ticks);
	}
	onTouchMoved(container, id, x, y, ticks) {
		const thumb = container.last.last;
		const width = thumb.width;
		let fraction = (x - (container.x + (width >> 1))) / (container.width - width);
		if (fraction < 0)
			fraction = 0;
		else if (fraction > 1)
			fraction = 1;
		this.onFractionChanging(container, fraction);
	}
	onTouchEnded(container, id, x, y, ticks) {
		container.bubble("onColorChanged");
	}
	onUpdateColor(container, hsv, color) {
		const thumb = container.last;
		const width = container.width - thumb.width;
		thumb.x = container.x + Math.round((hsv.v ** 2) * width);
	}
}

const ColorThumb = Container.template($ => ({
	left:0, width:40, top:0, height:40,
	contents: [
		Content($, { skin:thumbSkin, state:2, variant:2 }),
		Content($, { skin:thumbSkin, state:1, variant:1 }),
		Port($, {
			width:40, height:40,
			Behavior: class extends Behavior {
				onCreate(port, data) {
					this.color = 0;
					this.skin = thumbSkin;
				}
				onDraw(port) {
					const skin = this.skin;
					port.drawTexture(skin.texture, this.color, 0, 0, skin.x, skin.y, skin.width, skin.height);
				}
				onUpdateColor(port, hsv, color) {
					this.color = color;
					port.invalidate();
				}
			},
		}),
	],
}));

const LEDColorApplication = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, skin: backgroundSkin, Behavior: ApplicationBehavior,
	contents: [
		Container($, {
			left:20, width:200, top:30, height:200, active:true, Behavior:ColorSliderBehavior,
			contents: [
				Content($, { left:0, right:0, skin:colorSkin }),
				ColorThumb($, {}),
			],
		}),
		Container($, {
			left:20, width:200, top:250, height:40, active:true, Behavior:BrightnessSliderBehavior,
			contents: [
				Port($, { left:0, right:0, top:0, bottom:0,
					Behavior: class extends Behavior {
						onCreate(port, data) {
							this.color = 0;
							this.skin = brightnessSkin;
						}
						onDraw(port) {
							const skin = this.skin;
							port.fillTexture(skin.texture, this.color, 0, 0, port.width, port.height, skin.x, skin.y, skin.width, skin.height);
						}
						onUpdateColor(port, hsv, color) {
							color = HSVtoRGB(hsv.h, hsv.s, 1);
							this.color = rgb(color.r, color.g, color.b);
							port.invalidate();
						}
					},
				}),
				Content($, { left:0, right:0, height:40, skin:sliderSkin }),
				ColorThumb($, {}),
			],
		}),
	],
}));

export default new LEDColorApplication({});
