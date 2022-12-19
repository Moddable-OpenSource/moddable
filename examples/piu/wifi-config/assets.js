/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

const WHITE = "#FFFFFF";
const BLACK = "#000000";
const LIGHTEST_GRAY = "#e6e6e6";
const SEPARATOR_GRAY = "#666666";
const LIGHT_BLUE = "#0082ff";

const WhiteSkin = Skin.template({ fill: WHITE });
const LightestGraySkin = Skin.template({ fill: LIGHTEST_GRAY });
const SeparatorGraySkin = Skin.template({ fill: SEPARATOR_GRAY }); 
const LightBlueSkin = Skin.template({ fill: LIGHT_BLUE });

const OpenSans16 = Style.template({ font: "semibold 16px Open Sans", vertical: "middle", horizontal: "center" });
const OpenSans18 = Style.template({ font: "semibold 18px Open Sans", vertical: "middle" });
const OpenSans20 = Style.template({ font: "20px Open Sans", vertical: "middle", horizontal: "left", color: BLACK });
const BlackStyle = Style.template({ color: BLACK });
const WhiteStyle = Style.template({ color: WHITE });
const CenterStyle = Style.template({ horizontal: "center" });

const BackArrowTexture = Texture.template({ path:"header-arrow.png" });
const BackArrowSkin = Skin.template({ 
	Texture: BackArrowTexture, 
	color: [WHITE, LIGHTEST_GRAY], 
	x: 0, y: 0, width: 12, height: 24 
});

const WiFiStripTexture = Texture.template({ path:"wifi-strip.png" });
const WiFiStripSkin = Skin.template({ 
	Texture: WiFiStripTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 28, height: 28, 
	states: 27, variants: 28 
});

const ErrorTexture = Texture.template({ path:"page-1.png" });
const ErrorSkin = Skin.template({ 
	Texture: ErrorTexture, 
  color: "red",
	x: 0, y: 0, width: 48, height: 48 
});

const Header = Container.template($ => ({
	active: true, top: 0, height:40, left: 0, right: 0,
	Skin: LightBlueSkin, Style: OpenSans20,
	contents: [
		($.backArrowBehavior)? Content($, { active: true, left: 12, Skin: BackArrowSkin, Behavior: $.backArrowBehavior }) : null,
		Label($, { left: 33, Style: WhiteStyle, string: $.title }),
	],
  Behavior: $.backArrowBehavior
}));

// Have to use easing function to get x coordinates of circle to make squares look evenly distributed
// Simplified easeOutSine from https://github.com/danro/jquery-easing/blob/master/jquery.easing.js
// t: current time, b: beginning value, c: change in value, d: duration
const halfOfPi = (Math.PI/2);
function getX(t, b, c, d) {
  return c * Math.sin(t/d * halfOfPi) + b;
}
function getY(x, r) {
  return Math.sqrt((r*r) - (x*x));
}

class CircleOfSquaresBehavior extends Behavior {
  onDisplaying(port) {
    this.r = (port.width-8) / 2;
    this.centerCoord = this.r;
  }
  onDraw(port) {
    port.fillColor(WHITE, 0,0, port.width, port.height);
    let r = this.r;
    let centerCoord = this.centerCoord;
    let x, y;
    for (let i = 0; i < 9; i++) {
      x = getX(i, 0, r, 9);
      y = getY(x, r);
      // Top left quadrant
      port.fillColor("#cce6ff", centerCoord+x, centerCoord-y, 8, 8);
      // Bottom left quadrant
      port.fillColor("#cce6ff", centerCoord+y, centerCoord+x, 8, 8);
      // Bottom right quadrant
      port.fillColor("#cce6ff", centerCoord-x, centerCoord+y, 8, 8);
      // Top right quadrant
      port.fillColor("#cce6ff", centerCoord-y, centerCoord-x, 8, 8);
    }
  }
}
Object.freeze(CircleOfSquaresBehavior.prototype);

class AnimatedCircleOfSquaresBehavior extends Behavior {
  onDisplaying(port) {
    let r = this.r = (port.width-8) / 2;
    let centerCoord = this.centerCoord = r;
    let last6coords = this.last6coords = new Int16Array(12).fill(centerCoord - r);
    let startY = centerCoord;
    for (let i = 1; i < 12; i += 2) {
      last6coords[i] = startY;
    }
    this.index = 0;
    port.interval = port.duration / 36; // 36 is number of squares
    port.start();
  }
  onTimeChanged(port) {
    this.index++;
    if (this.index == 36) this.index = 0;
    let index = this.index;
    let r = this.r;
    let last6coords = this.last6coords;
    let centerCoord = this.centerCoord;
    let step = index % 9;
    let nextX, nextY;
    let x = getX(step, 0, r, 9);
    let y = getY(x, r);
    if (index < 9) {
      nextX = centerCoord-y;
      nextY = centerCoord-x;
    }
    else if (index < 18) {
      nextX = centerCoord+x;
      nextY = centerCoord-y;
    }
    else if (index < 27) {
      nextX = centerCoord+y;
      nextY = centerCoord+x;
    }
    else {
      nextX = centerCoord-x;
      nextY = centerCoord+y;
    }
    last6coords.copyWithin(2, 0);
    last6coords[0] = nextX;
    last6coords[1] = nextY;
    port.invalidate();
  }
  onDraw(port) {
    let last6coords = this.last6coords;
    port.fillColor("#005cb3", last6coords[1], last6coords[0], 8, 8);
    port.fillColor(LIGHT_BLUE, last6coords[3], last6coords[2], 8, 8);
    port.fillColor("#339cff", last6coords[5], last6coords[4], 8, 8);
    port.fillColor("#66b5ff", last6coords[7], last6coords[6], 8, 8);
    port.fillColor("#99ceff", last6coords[9], last6coords[8], 8, 8);
    port.fillColor("#cce6ff", last6coords[11], last6coords[10], 8, 8);
  } 
}
Object.freeze(AnimatedCircleOfSquaresBehavior.prototype);

export const WiFiStatusSpinner = Container.template($ => ({
  left: 0, right: 0, top: 0, bottom: 0, Skin: WhiteSkin,
  contents: [
    Port($, {
      top: 30, height: 145, width: 145,
      Behavior: CircleOfSquaresBehavior,
    }),
    Port($, {
      loop: true, duration: 2000,
      top: 30, height: 145, width: 145,
      Behavior: AnimatedCircleOfSquaresBehavior,
    }),
    Label($, {
      bottom: 30, height: 18, left:0, right: 0, Style: OpenSans16, string: $.status,
    }),
  ],
}));

export default {
	WHITE,
	BLACK,
	LIGHTEST_GRAY,
	LIGHT_BLUE,
	SEPARATOR_GRAY,
	WhiteSkin,
	LightestGraySkin,
	SeparatorGraySkin,
	LightBlueSkin,
	OpenSans16,
	OpenSans18,
	OpenSans20,
	BlackStyle,
	WhiteStyle,
	CenterStyle,
	WiFiStripTexture,
	WiFiStripSkin,
	ErrorSkin,
	Header,
  WiFiStatusSpinner
	// LoadingSpinner,
	// LoadingBubbles
}


