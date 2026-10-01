/*
* Copyright (c) 2026  Moddable Tech, Inc.
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

declare module "embedded:io/image/in/camera" {
  // pixel format constants are those of commodetto/Bitmap; "jpeg" selects compressed frames
  export type ImageType = number | "jpeg";

  export interface CameraFrame extends HostBuffer, Disposable {
    readonly camera: Camera;
    close(): void;
  }

  export interface CameraConfiguration {
    brightness?: number;
    contrast?: number;
    saturation?: number;
    sharpness?: number;
    denoise?: number;
    whiteBalance?: number;
  }

  export type Format = "buffer" | "buffer/disposable";

  export interface Options<F extends Format = Format> {
    width?: number;
    height?: number;
    imageType?: ImageType;
    format?: F;
    onReadable?: (this: Camera<F>, count: number) => void;
    target?: any;
  }

  /*
    F is the format named in the options when the camera is created. Naming it there
    (format: "buffer") fixes the type read() with no argument returns:
    a CameraFrame for "buffer/disposable", an ArrayBuffer for "buffer".
    When the options name no format, read() is typed as returning either, because the
    format can be changed later through the format property.
  */
  interface Camera<F extends Format = Format> extends Disposable {
    target?: any;
    close(): void;
    // format "buffer/disposable" returns the frame without copying; close it to release it to the camera
    read(): (F extends "buffer/disposable" ? CameraFrame : ArrayBuffer) | undefined;
    read(buffer: ByteBuffer): number | undefined;
    start(): void;
    stop(): void;
    configure(options: CameraConfiguration): void;
    get format(): F;
    set format(value: Format);
    readonly imageType: ImageType;
    readonly width: number;
    readonly height: number;
    readonly identification: { model: string };
    readonly configuration: CameraConfiguration;
    [Symbol.dispose](): void;
  }
  interface CameraConstructor {
    new <F extends Format = Format>(options: Options<F>): Camera<F>;
    readonly prototype: Camera;
  }
  const Camera: CameraConstructor;

  export default Camera;
}
