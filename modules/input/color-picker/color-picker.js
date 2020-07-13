/*
 * Copyright (c) 2020 Moddable Tech, Inc.
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

import Poco from "commodetto/Poco";
import BufferOut from "commodetto/BufferOut";
import Resource from "Resource";
import parseBMP from "commodetto/parseBMP";

const BLACK = "black";
const WHITE = "white";

const BackgroundSkin = Skin.template({ fill: BLACK });

const ColorIndicatorTexture = Texture.template({ path: "color-indicator.png" });
const ColorIndicatorSkin = Skin.template({
    Texture: ColorIndicatorTexture, color: WHITE,
    x: 0, y: 0, width: 36, height: 36
});

const ColorCircleTexture = Texture.template({ path: "color-color.bmp" });
const ColorCircleSkin = Skin.template({
    Texture: ColorCircleTexture,
    x: 0, y: 0, width: 152, height: 152
});

const ColorLimiterTexture = Texture.template({ path: "color-limiter.png" });
const ColorLimiterSkin = Skin.template({
    Texture: ColorLimiterTexture, color: WHITE,
    x: 0, y: 0, width: 185, height: 185
});

class ColorWheelBehavior extends Behavior {
    onCreate(wheel, data) {
        this.data = data;

        this.wheelGraphic = parseBMP(new Resource("color-color.bmp"));
        let offscreen = new BufferOut({ width: 1, height: 1, pixelFormat: screen.pixelFormat });
        this.pocoOff = new Poco(offscreen);
    }
    onTouchBegan(wheel, id, x, y, ticks) {
        this.update(wheel, x, y);
    }
    onTouchMoved(wheel, id, x, y, ticks) {
        this.update(wheel, x, y);
    }
    onTouchEnded(wheel, id, x, y, ticks) {
        this.update(wheel, x, y);
    }
    update(wheel, x, y) {
        let newx = x - wheel.x - 92;
        let newy = y - wheel.y - 92;
        let z = newx * newx + newy * newy;
        if (z > 5184) {
            let j = 72 / Math.sqrt(z);
            newx *= j;
            newy *= j;
        }
        newx += wheel.x + 92;
        newy += wheel.y + 92;
        let color = this.getColor(newx - wheel.x - 18, newy - wheel.y - 18);
        this.data["COLOR_INDICATOR"].position = { x: newx - 18, y: newy - 18 };
        if (color != "#000000") application.distribute("onColorPicked", color);
    }
    getColor(x, y) {
        this.pocoOff.begin();
        this.pocoOff.drawBitmap(this.wheelGraphic, 0, 0, x, y, 1, 1);
        this.pocoOff.end();

        let buff = this.pocoOff.pixelsOut.buffer;
        let view = new DataView(buff);
        let testcolor = view.getUint16(0, true);

        let red = ((testcolor >> 11) & 0x1F);
        let green = ((testcolor >> 5) & 0x3F);
        let blue = (testcolor & 0x1F);
        red = red << 3 | red >> 2;
        green = green << 2 | green >> 6;
        blue = blue << 3 | blue >> 2;

        return { red, green, blue };
    }
}

const ColorPicker = Container.template($ => ({
    left: 0, right: 0, top: 0, height: 185, Skin: BackgroundSkin,
    contents: [
        Content($, {
            top: 15, Skin: ColorCircleSkin
        }),
        Content($, {
            top: 0, Skin: ColorLimiterSkin,
            active: true, Behavior: ColorWheelBehavior
        }),
        Content($, {
            anchor: "COLOR_INDICATOR", top: 74, height: 36, left: 97, width: 36, Skin: ColorIndicatorSkin
        })
    ]
}));

export default ColorPicker;