/*
 * Copyright (c) 2020 Moddable Tech, Inc.
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


import Timer from 'timer';
import QwiicTwist from 'qwiictwist';
import ColorPicker from "color-picker";

const RED_MAX = 125; //The red LED on the Qwiic Twist is considerably brighter than the others.

const BLACK = "black";
const WHITE = "white";

const backgroundSkin = new Skin({ fill: BLACK });
const textStyle = new Style({ color: WHITE, font: "18px Open Sans", horizontal: "left" });

class AppBehavior extends Behavior{
    onCreate(app, data){
        this.data = data;

        this.twist = new QwiicTwist();
        this.twist.configure({
            color: { red: RED_MAX, green: 255, blue: 255 },
            connect: { red: 0, green: 0, blue: 0 }
        });
        
        this.twistState = {
            encoderCount: 0,
            buttonDown: false, 
            msSinceMovement: 0,
            msSinceClick: 0
        }

        Timer.repeat(id => {
            const sample = this.twist.sample();
            if (sample.encoderCount != this.twistState.encoderCount){
                this.data.ENCODER_VALUE.string = `Encoder Clicks: ${sample.encoderCount}`;
                this.twistState.encoderCount = sample.encoderCount;
            }
            if (sample.status.buttonDown != this.twistState.buttonDown){
                this.data.BUTTON_VALUE.string = "Button: " + (sample.status.buttonDown ? "Down" : "Up");
                this.twistState.buttonDown = sample.status.buttonDown;
            }
            if (sample.msSinceMovement != this.twistState.msSinceMovement){
                this.data.ENCODER_TIME.string = `Encoder Time: ${Math.floor(sample.msSinceMovement / 1000)} seconds`;
                this.twistState.msSinceMovement = sample.msSinceMovement;
            }
            if (sample.msSinceClick != this.twistState.msSinceClick){
                this.data.BUTTON_TIME.string = `Click Time: ${Math.floor(sample.msSinceClick / 1000)} seconds`;
                this.twistState.msSinceClick = sample.msSinceClick;
            }
        }, 200);
    }

    onColorPicked(app, color){
        color.red = Math.floor(color.red * (RED_MAX / 255));
        this.twist.configure({ color });
    }
}

const QwiicTwistApp = Application.template($ => ({
    left: 0, right: 0, top: 0, bottom: 0, skin: backgroundSkin,
    contents: [
       Column($, {top: 5, right: 0, bottom: 5, left: 10, contents: [
           ColorPicker($, {}),
           Label($, {
                left: 0, right: 0, top: 10, height: 20,
                anchor: "ENCODER_VALUE", string: "Encoder Clicks: 0", style: textStyle
           }),
           Label($, {
               left: 0, right: 0, top: 10, height: 20,
               anchor: "BUTTON_VALUE", string: "Button: Up", style: textStyle
           }),
           Label($, {
               left: 0, right: 0, top: 10, height: 20,
               anchor: "ENCODER_TIME", string: "Encoder Time: 0 seconds", style: textStyle
           }),
           Label($, {
               left: 0, right: 0, top: 10, height: 20,
               anchor: "BUTTON_TIME", string: "Click Time: 0 seconds", style: textStyle
           }),
       ]})
       
    ],
    Behavior: AppBehavior
}));

export default new QwiicTwistApp({}, { displayListLength: 4092, touchCount: 1 });