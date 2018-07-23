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

const OpenSans16 = Style.template({ font: "semibold 16px Open Sans", vertical: "middle" });
const OpenSans18 = Style.template({ font: "semibold 18px Open Sans", vertical: "middle" });
const OpenSans20 = Style.template({ font: "normal normal normal 20px Open Sans", vertical: "middle" });
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
	x: 0, y: 0, width: 48, height: 48 
});

const Header = Container.template($ => ({
	top: 0, height:40, left: 0, right: 0,
	Skin: LightBlueSkin, Style: OpenSans20,
	contents: [
		Content($, { active: true, left: 12, Skin: BackArrowSkin, Behavior: $.backArrowBehavior }),
		Label($, { left: 33, Style: WhiteStyle, string: $.title }),
	]
}));

const DOWN = 0;
const RIGHT = 1;
const UP = 2;
const LEFT = 3;
const PAUSE = 4;

const LoadingSpinner = Port.template($ => ({
  top: $.top, left: $.left, height: $.squareSize*5, width: $.squareSize*5,
  Behavior: class extends Behavior {
    onCreate(port, data) {
      this.data = data;
      data.color1 = hsl(data.color[0], data.color[1], data.color[2]);
      data.color2 = hsl(data.color[0], data.color[1], data.color[2]+0.17);
      data.color3 = hsl(data.color[0], data.color[1], data.color[2]+0.34);
      data.color4 = hsl(data.color[0], data.color[1], data.color[2]+0.51);
      this.direction = DOWN;
      this.index = 0;
    }
    onDisplaying(port) {
      this.dxy = Math.round(port.height / 10);
      this.x = 0;
      this.y = 0;
      this.last6coords = new Int16Array(12);
      port.interval = this.data.frequency / 30;
      port.time = 0;
      port.start();
    }
    onDraw(port) {
      let dxy = this.dxy
      let last6coords = this.last6coords;
      port.fillColor(this.data.color4, last6coords[10], last6coords[11], this.data.squareSize, this.data.squareSize);
      port.fillColor(this.data.color3, last6coords[6], last6coords[7], this.data.squareSize, this.data.squareSize);
      port.fillColor(this.data.color2, last6coords[2], last6coords[3], this.data.squareSize, this.data.squareSize);
      switch (this.direction) {
        case DOWN:
          this.y += this.dxy;
          break;
        case RIGHT:
          this.x += this.dxy;
          break;
        case UP:
          this.y -= this.dxy;
          break;
        case LEFT:
          this.x -= this.dxy;
          break;
        case PAUSE:
          break;
      }
      port.fillColor(this.data.color1, this.x, this.y, this.data.squareSize, this.data.squareSize);
      last6coords.copyWithin(2, 0);
      last6coords[0] = this.x;
      last6coords[1] = this.y;
    }
    onTimeChanged(port) {
      this.index++;
      if (this.index == 8) {
        this.index = 0;
        this.direction++;
        if (this.direction > PAUSE) {
          this.x = 0;
          this.direction = DOWN;
        }
      }
      port.invalidate();
    }
  }
}))

const LoadingBubbles = Port.template($ => ({
  top: $.top, left: $.left, height: $.squareSize, width: $.squareSize*5,
  Behavior: class extends Behavior {
    onCreate(port, data) {
      this.data = data;
      data.color1 = hsl(data.color[0], data.color[1], data.color[2]);
      data.color2 = hsl(data.color[0], data.color[1], data.color[2]+0.51);
      this.index = 0;
    }
    onDisplaying(port) {
      port.interval = this.data.frequency / 30;
      port.time = 0;
      port.start();
    }
    onDraw(port) {
    	let sq1color, sq2color, sq3color;
    	switch (this.index) {
    		case 0:
    			sq1color = this.data.color1;
    			sq2color = sq3color = this.data.color2;
    			break;
    		case 1:
    			sq1color = sq3color = this.data.color2;
    			sq2color = this.data.color1;
    			break;
    		case 2:
    			sq3color = this.data.color1;
    			sq2color = sq1color = this.data.color2;
    			break;
    		case 3:
    			sq1color = sq3color = this.data.color2;
    			sq2color = this.data.color1;
    			break;
    	}
    	port.fillColor(sq1color, 0, 0, this.data.squareSize, this.data.squareSize);
    	port.fillColor(sq2color, this.data.squareSize*2, 0, this.data.squareSize, this.data.squareSize);
    	port.fillColor(sq3color, this.data.squareSize*4, 0, this.data.squareSize, this.data.squareSize);
    }
    onTimeChanged(port) {
      this.index++;
      if (this.index > 3) this.index = 0;
      port.invalidate();
    }
  }
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
	LoadingSpinner,
	LoadingBubbles
}


