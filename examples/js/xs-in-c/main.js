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

import Rectangle from "rectangle";

let r1 = new Rectangle(0, 0, 200, 100);
let r2 = new Rectangle(20, 40, 300, 50);
trace(`r1.x: ${r1.x}, r2.w: ${r2.w}\n`);
trace(`r1.contains(50, 50): ${r1.contains(50, 50)}\n`);
trace(`r2.contains(100, 100): ${r2.contains(100, 100)}\n`);

let r3 = new Rectangle(r2);
trace(`r3: (${r3.x}, ${r3.y}, ${r3.w}, ${r3.h})\n`);

r2.x = 30;
trace(`r2.x: ${r2.x}\n`);

r1.union(r2);
trace(`r1.union(r2): (${r1.x}, ${r1.y}, ${r1.w}, ${r1.h})\n`);

let r4 = new Rectangle();
let r5 = new Rectangle(0, 0, 10, 10);
let r6 = new Rectangle(10, 10, 10, 10);
let r7 = new Rectangle(20, 20, 10, 10);
r4.union(r5, r6, r7);
trace(`r4.union(r5, r6, r7): (${r4.x}, ${r4.y}, ${r4.w}, ${r4.h})\n`);


