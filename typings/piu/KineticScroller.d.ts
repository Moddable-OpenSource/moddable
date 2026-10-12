/*
 * Copyright (c) 2026 Moddable Tech, Inc
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// modules/piu/MC/kineticScroller/piuKineticScroller.js

declare module "piu/KineticScroller" {
  import { Behavior, Scroller } from "piu/MC";

  /** the touch samples Piu passes to touch events after the ticks */
  interface TouchSamples {
    readonly length: number;
    peek(index: number): { x: number, y: number, ticks: number };
  }

  // the events of behaviors that a kinetic scroller behavior does not implement itself, like onScrolled
  interface KineticScrollerBehavior extends Behavior<Scroller> {}
  abstract class KineticScrollerBehavior {
    /** the data passed to the constructor of the scroller; its scroll property, if any, is restored and saved */
    data: any;

    /** override to refuse to track a touch by returning false */
    captureTouch(scroller: Scroller, id: number, x: number, y: number, ticks: number): boolean;
    /** override to stop on specific positions; returns the position to stop at */
    snap(scroller: Scroller, position: number, direction: number): number;

    onCreate(scroller: Scroller, data: any, context?: any): void;
    onDisplaying(scroller: Scroller): void;
    onFinished(scroller: Scroller): void;
    onMouseScrolled(scroller: Scroller, dx: number, dy: number): void;
    onTimeChanged(scroller: Scroller): void;
    onTouchBegan(scroller: Scroller, id: number, x: number, y: number, ticks: number, samples?: TouchSamples): void;
    onTouchCancelled(scroller: Scroller, id: number, x?: number, y?: number, ticks?: number): void;
    onTouchEnded(scroller: Scroller, id: number, x: number, y: number, ticks: number, samples?: TouchSamples): void;
    onTouchMoved(scroller: Scroller, id: number, x: number, y: number, ticks: number, samples?: TouchSamples): void;
    onUndisplaying(scroller: Scroller): void;
  }

  /** scrolls the first content of a scroller horizontally */
  export class HorizontalScrollerBehavior extends KineticScrollerBehavior {}
  /** scrolls the first content of a scroller vertically */
  export class VerticalScrollerBehavior extends KineticScrollerBehavior {}
}
