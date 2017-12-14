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

/*
Responsive soft keyboard module.

dictionary options:
  style (required):       Style object to use for the text on keys

  bgColor:                A string specifying the background fill color 
  keyColor:               A string specifying the key fill color when not pressed
  specialKeyColor:        A string specifying the special key fill color when not pressed
  textColor:              A string specifying the color of text on keys
  specialTextColor:       A string specifying the color of text and masks on the bottom row of keys
  keyDownColor:           A string specifying the color to highlight keys in while they are being pressed
  keyToggledColor:        A string specifying the color to indicate toggle on the shift and symbol keys
  submit:                 A string to render on the submit key
  doTransition:           A boolean specifying whether or not to transition in the keyboard when first displayed (Default: false)
  transitionTime:         A number specifying the duration of the keyboard transition in ms (Default: 250)
*/
import {
  Texture,
  Behavior,
  Port,
} from "piu/MC";
import Timeline from "piu/Timeline";

const BGDEFAULT = "#5b5b5b";
const KEYDEFAULT = "#d8d8d8";
const SPECIALKEYDEFAULT = "#999999";
const TEXTCOLORDEFAULT = "#000000";
const SPECIALTEXTDEFAULT = "#ffffff"
const KEYDOWNDEFAULT = SPECIALKEYDEFAULT;
const KEYTOGGLEDDEFAULT = "#7b7b7b";

const SUBMITDEFAULT = "OK";

const TOPMARGIN = 4;
const BOTTOMMARGIN = 2;
const LEFTRIGHTMARGIN = 6;

export const BACKSPACE = "\b";
export const SUBMIT = "\r";

const ShiftTexture = Texture.template({ path:"shift-arrow.png" });
const BackspaceTexture = Texture.template({path:"delete-arrow.png"});
const NumKeysTexture = Texture.template({path:"num-keys.png"});

class KeyboardBehavior extends Behavior{
  
  getKey(port, x, y){
    y -= TOPMARGIN;
    
    if (y >= 0){
      let ySpot = y % (this.rowHeight + this.internalPad);
      if (ySpot <= this.rowHeight){ //hit a row
        let row = Math.floor(y / (this.rowHeight + this.internalPad));
        x -= this.rowStarts[row];
        if (x >= 0){
          if (row <= 2){
            let xSpot = x % (this.keyWidth + this.internalPad);
            if (xSpot <= this.keyWidth){
              let col = Math.floor(x / (this.keyWidth + this.internalPad));
              return {key: this.state[row].charAt(col), x: col, y: row};
            }
          }else{ //bottom row
            let result = {y: row};
            if (x < this.shiftWidth){
              result.x = 0;
              result.key = this.SHIFT;
              return result;
            }
            x -= (this.shiftWidth + this.internalPad);
            if (x >= 0 && x < this.symbolWidth){
              result.x = 1;
              result.key = this.CHANGE;
              return result;
            }
            x -= (this.symbolWidth + this.internalPad);
            if (x >= 0 && x < this.spaceBarWidth){
              result.x = 2;
              result.key = this.SPACE;
              return result;
            }
            x -= (this.spaceBarWidth + this.internalPad);
            if (x >= 0 && x < this.shiftWidth){
              result.x = 3;
              result.key = BACKSPACE;
              return result;
            }
            x -= (this.shiftWidth + this.internalPad);
            if (x >= 0 && x < this.shiftWidth){
              result.x = 4;
              result.key = SUBMIT;
              return result;
            }
          }
        }
      }
    }  
  }
  
  onTouchBegan(port, id, x, y){
    x = x - port.x;
    y = y - port.y;
    
    this.keyDown = this.getKey(port, x, y);
    if (this.keyDown){
      port.invalidate();
      port.bubble("onKeyDown", this.keyDown.key);
    }
  }
  
  onTouchEnded(port, id, x, y){
    if (this.keyDown){
      x = x - port.x;
      y = y - port.y;
      
      let key = this.getKey(port, x, y);
      if (key && key.key == this.keyDown.key){
        if (key.key == this.CHANGE){
          if (this.state == this.CAPS || this.state == this.LOWER){
            this.state = this.SYMBOL;
          }else{
            if (this.shiftState){
              this.state = this.CAPS;
            }else{
              this.state = this.LOWER;
            }
          }
        }else if(key.key == this.SHIFT){
          this.shiftState = !(this.shiftState);
          if (this.state == this.CAPS){
            this.state = this.LOWER;
          }else if(this.state == this.LOWER){
            this.state = this.CAPS;
          }
        }else{
          port.bubble("onKeyUp", key.key);
        }
      }
      delete this.keyDown;
      port.invalidate();
    }
  }
  
  resizePadding(port){
    this.expectedWidth = port.width;
    this.expectedHeight = port.height;
    this.internalPad = Math.floor(this.expectedWidth / 100);
    this.verticalPad = TOPMARGIN + BOTTOMMARGIN + (this.internalPad * 3);
    this.fullPad = this.internalPad * 9;
  }
  
  onCreate(port, $, data){
    this.CAPS = [ "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM',."];
    this.LOWER = [ "qwertyuiop", "asdfghjkl", "zxcvbnm',."];
    this.SYMBOL = [ "1234567890", "#$%&*()_@", "!?/\\;:=+-\""];
    
    this.data = data;
    this.state = this.LOWER;
    this.shiftState = false;
    
    this.SHIFT = "SHIFT";
    this.CHANGE = "CHANGE";
    this.SPACE = " ";
    
    this.rowStarts = [0, 0, 0, LEFTRIGHTMARGIN];
    
    this.shiftArrow = new ShiftTexture();
    this.backspaceArrow = new BackspaceTexture();
    this.numKeys = new NumKeysTexture();
    
    this.style = data.style;

	this.metrics = new Uint8Array(128);
	for (let i = 32; i < 128; i++)
		this.metrics[i] = this.style.measure(String.fromCharCode(i)).width;
	this.metrics.height = this.style.measure(String.fromCharCode(33)).height;
    
    this.bgColor = (data.bgColor) ? data.bgColor : BGDEFAULT;
    this.keyColor = (data.keyColor) ? data.keyColor : KEYDEFAULT;
    this.specialKeyColor = (data.specialKeyColor) ? data.specialKeyColor : SPECIALKEYDEFAULT;
    this.textColor = (data.textColor) ? data.textColor : TEXTCOLORDEFAULT;
    this.specialTextColor = (data.specialTextColor) ? data.specialTextColor : SPECIALTEXTDEFAULT;
    this.submit = (data.submit) ? data.submit : SUBMITDEFAULT;
    this.keyDownColor = (data.keyDownColor) ? data.keyDownColor : KEYDOWNDEFAULT;
    this.keyToggledColor = (data.keyToggledColor) ? data.keyToggledColor : KEYTOGGLEDDEFAULT;
    
    this.doTransition = (data.doTransition !== undefined) ? data.doTransition : false;
    this.transitionTime = (data.transitionTime) ? data.transitionTime : 250;
    
    this.expectedWidth = 0;
    this.expectedHeight = 0;
  }
  
  drawRow(port, row, y, height, keyWidth, rowNum){
    let keys = row.length;
    let pads = this.internalPad * (keys - 1);
    let x = (port.width - ((keys * keyWidth) + pads)) >> 1;
    this.rowStarts[rowNum] = x;

	let keyDown = this.keyDown, metrics = this.metrics;
	let cy = y + ((height - metrics.height) >> 1), dx = keyWidth + this.internalPad;
    for (let i = 0; i < keys; i++){
      let color = this.keyColor;
      if (keyDown && keyDown.y == rowNum && keyDown.x == i) color = this.keyDownColor;
      port.fillColor(color, x, y, keyWidth, height);

	  let char = row.charAt(i);
      port.drawString(char, this.style, this.textColor,
				x + ((keyWidth - metrics[char.charCodeAt()]) >> 1), cy);
      x += dx;
    }
  }
  
  drawSpaceBarRow(port, state, y, height){
    let onX = LEFTRIGHTMARGIN;
    
    //Shift
    let color = this.specialKeyColor;
    if (this.keyDown && this.keyDown.y == 3 && this.keyDown.x == 0){
      color = this.keyColor;
    }else if (this.shiftState){
      color = this.keyToggledColor;
    }
    port.fillColor(color, onX, y, this.shiftWidth, height);
    let xOff = onX + ((this.shiftWidth - 16) >> 1);
    let yOff = y + ((height - 18) >> 1);
    port.drawTexture(this.shiftArrow, this.specialTextColor, xOff, yOff, 0, 0, 16, 18);
    onX += this.shiftWidth + this.internalPad;
    
    //Symbol vs Alpha
    color = this.specialKeyColor;
    if (this.keyDown && this.keyDown.y == 3 && this.keyDown.x == 1){
      color = this.keyColor;
    }else if(state == this.SYMBOL){
      color = this.keyToggledColor;
    }
    port.fillColor(color, onX, y, this.symbolWidth, height);
    xOff = onX + ((this.symbolWidth - 32) >> 1);
    yOff = y + ((height - 11) >> 1);
    port.drawTexture(this.numKeys, this.specialTextColor, xOff, yOff, 0, 0, 32, 11);
    onX += this.symbolWidth + this.internalPad;
    
    //space bar
    color = this.keyColor;
    if (this.keyDown && this.keyDown.y == 3 && this.keyDown.x == 2) color = this.keyDownColor;
    port.fillColor(color, onX, y, this.spaceBarWidth, height);
    onX += this.spaceBarWidth + this.internalPad;
    
    //Backspace
    color = this.specialKeyColor;
    if (this.keyDown && this.keyDown.y == 3 && this.keyDown.x == 3) color = this.keyColor;
    port.fillColor(color, onX, y, this.shiftWidth, height);
    xOff = onX + ((this.shiftWidth - 14) >> 1);
    yOff = y + ((height - 15) >> 1);
    port.drawTexture(this.backspaceArrow, this.specialTextColor, xOff, yOff, 0, 0, 14, 15);
    onX += this.shiftWidth + this.internalPad;
    
    //Submit
    color = this.specialKeyColor;
    if (this.keyDown && this.keyDown.y == 3 && this.keyDown.x == 4) color = this.keyColor;
    port.fillColor(color, onX, y, this.shiftWidth, height);
    let measure = port.measureString(this.submit, this.style);
    xOff = onX + ((this.shiftWidth - measure.width) >> 1);
    yOff = y + ((height - measure.height) >> 1);
    port.drawString(this.submit, this.style, this.specialTextColor, xOff, yOff);
  }
  
  onFinished(port){
    this.transitioning = false;
    port.active = true;
    this.onTransitionRow = -1;
    if (!this.transitioningOut){
      port.invalidate();
    }else{
      this.drawNothing = true;
      port.invalidate();
      port.bubble("onKeyboardTransitionFinished");
    }
  }
  
  startTransition(port, transitioningIn){
    if (transitioningIn){
      this.transitioningOut = false;
      let height = port.height;
      let timeline = this.timeline = new Timeline();
      let y = TOPMARGIN;
      
      this.transitions = {onRow: 0, y0: height + 1 + TOPMARGIN, y1: height + 1 + TOPMARGIN, y2: height + 1 + TOPMARGIN, y3: height + 1 + TOPMARGIN};
      let y0, y1, y2, y3;
      this.transitionNames = ["y0", "y1", "y2", "y3"];
      this.onTransitionRow = -1;
      this.transitioningOut = false;
      timeline.to(this.transitions, { y0: y }, this.transitionTime, Math.quadEaseOut, 0);
      y += this.rowHeight + this.internalPad;
      timeline.to(this.transitions, { y1: y }, this.transitionTime, Math.quadEaseOut, 20);
      y += this.rowHeight + this.internalPad;
      timeline.to(this.transitions, { y2: y }, this.transitionTime, Math.quadEaseOut, 20);
      y += this.rowHeight + this.internalPad;
      timeline.to(this.transitions, { y3: y }, this.transitionTime, Math.quadEaseOut, 20);
      this.timeline.seekTo(0);
    }else{
      this.transitioningOut = true;
    }
    
    this.transitioning = true;
    port.active = false;
    port.duration = this.timeline.duration;
    port.time = 0;
    port.start();
  }
  
  doKeyboardTransitionOut(port){
    port.active = false;
    this.startTransition(port, false);
  }
  
  onTimeChanged(port){
    let start = [this.transitions.y0, this.transitions.y1, this.transitions.y2, this.transitions.y3];
    let time = port.time;
    if (this.transitioningOut) time = port.duration - port.time;
    this.timeline.seekTo(time);
    let delta = 0;
    this.onTransitionRow = -1;
    for (let i = 0; i < 4; i++){
      if (this.transitions[this.transitionNames[i]] != start[i]){
        delta = Math.abs(start[i] - this.transitions[this.transitionNames[i]]);
        this.onTransitionRow = i;    
        break;
      }
    }

    if (this.onTransitionRow != -1){
      let pad = this.onTransitionRow == 0 ? TOPMARGIN : 0;
      if (this.transitioningOut){
        port.invalidate(0, start[this.onTransitionRow] - pad - 1, port.width, this.rowHeight + pad + delta + 1);
      }else{
        port.invalidate(0, this.transitions[this.transitionNames[this.onTransitionRow]] - pad, port.width, this.rowHeight + pad + delta + 1);
      }
    }
  }
  
  onDraw(port){
    if (this.drawNothing) return;
    
    if (port.width != this.expectedWidth || port.height != this.expectedHeight){
      this.resizePadding(port);
      this.rowHeight = Math.floor((port.height - this.verticalPad) / 4);
      this.keyWidth = Math.floor((port.width - (LEFTRIGHTMARGIN + LEFTRIGHTMARGIN) - (this.fullPad)) / this.CAPS[0].length);
      this.shiftWidth = Math.floor(this.keyWidth * 1.42);
      this.symbolWidth = Math.floor(this.keyWidth * 1.79);
      this.spaceBarWidth = port.width - (LEFTRIGHTMARGIN + LEFTRIGHTMARGIN + (this.shiftWidth * 3) + this.symbolWidth + (this.internalPad * 4));
      
      if (this.doTransition){
        this.startTransition(port, true);
      }
    }
    
    if (!this.transitioning || (this.transitioningOut && this.onTransitionRow == -1)){
      port.fillColor(this.bgColor, 0, 0, port.width, port.height);
      let onY = TOPMARGIN;    
      for (let i = 0; i < 3; i++){
          this.drawRow(port, this.state[i], onY, this.rowHeight, this.keyWidth, i);
          onY += this.rowHeight + this.internalPad;
      }
      this.drawSpaceBarRow(port, this.state, onY, this.rowHeight);  
    }else if (this.onTransitionRow != -1){
      let row = this.onTransitionRow;
      port.fillColor(this.bgColor, 0, this.transitions.y0 - TOPMARGIN, port.width, port.height);
      let onY = this.transitions[this.transitionNames[row]];
      if (row == 3){
        this.drawSpaceBarRow(port, this.state, onY, this.rowHeight);
      }else{
        this.drawRow(port, this.state[row], onY, this.rowHeight, this.keyWidth, row);
      }
    }
  }  
}

export const Keyboard = Port.template($ => ({
  left: 0, right: 0, top: 0, bottom: 0, active: true,
  Behavior: KeyboardBehavior,
}));
