/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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

import {} from "piu/MC";

let qr_message = 'https://blog.moddable.com/blog/qrcode';

let QRcodeObj = new QRCode(null, {
  width: 110,
  top: 30,
  height: 110,
  skin: { fill: "white", stroke: "blue" },
  string: qr_message,
  maxVersion: 4,
});

class AppBehavior extends Behavior {
  onCreate(app) {
    app.add(QRcodeObj);
  }
}

export default new Application(null, {
  skin: new Skin({
    fill: "silver",
  }),
  Behavior: AppBehavior,
});
