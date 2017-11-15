# Piu

Copyright 2016 Moddable Tech, Inc.

Revised: December 23, 2016

Warning: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Introduction

Piu is a simple user interface framework based on a subset of the programming interface of KinomaJS. However the implementation is brand new in order to run on a micro-controller.

KinomaJS has C and JS programming interfaces and uses volatile host objects to map C records into JS objects. Piu has only a JS programming interface and uses handles to benefit from the XS garbage colletor. See [Handle](./handle.md) for details.

This document highlights the main differences between Piu and KinomaJS. See `$MODDABLE/modules/piu/All/piuAll.js` for the complete JS programming interface.

## Colors

To specify a color in the dictionaries of the `Skin` and `Style` constructors, you can use CSS names (Level 2) or hexadecimal notations (`"#RGB"`, `"#RGBA"`, `"#RRGGBB"`, `"#RRGGBBAA"`):
	
	const whiteSkin = new Skin({ fill:"white" });
	const redSkin = new Skin({ fill:"#F00" });
	const halfRedSkin = new Skin({ fill:"#F008" });
	const greenSkin = new Skin({ fill:"#00FF00" });
	const halfGreenSkin = new Skin({ fill:"#00FF0088" });

To specify a color with an alpha channel, you can also use a mere hexadecimal number `0xRRGGBBAA`:

	const blueSkin = new Skin({ fill:0x0000FFFF });
	const halfBlueSkin = new Skin({ fill:0x0000FF88 });
	
To build such hexadecimal numbers, Piu exports functions similar to the CSS functional notations:

	import { rgb, rgba, hsl, hsla } from "piu/All";
	const yellowSkin = new Skin({ fill:rgb(255, 255, 0) });
	const halfYellowSkin = new Skin({ fill:rgba(255, 255, 0, 0.5) });
	const cyanSkin = new Skin({ fill:hsl(180, 1, 0,5) });
	const halfCyanSkin = new Skin({ fill:hsl(180, 1, 0.5, 0.5) });

In dictionaries, colors can be a single color or an array of 2, 3, or 4 colors. The `skin` and `style` objects blend a color based on the state of the `content` object that uses them.

	const stateSkin = new Skin({ fill:["white", "black"] });
	var content = new Content(null, { width:100, height:100, state:0.5 }); // will draw a gray square
	
## Textures

Textures are assets and must be defined in the resources of your manifest.
	
At runtime a texture uses color and/or alpha bitmaps. The **mcconfig** tool parses the manifest and tells the **png2bitmap** tool to convert PNG files into color and/or alpha bitmaps. By default you get both:

	"resources": {
		"*": balls,
	}

To get only the color bitmaps, use the `-color` pseudo target:

	"resources": {
		"*-color": pop,
	}
	

To get only the alpha bitmaps, use the `-alpha` pseudo target:

	"resources": {
		"*-alpha": logo,
	}

The `Texture` constructor requires a dictionary:

	const ballsTexture = new Texture({ path:"balls.png" });

Piu maps the `path` property to initialize color and/or alpha bitmaps, depending on their presence in the resources.

## Skins

Skins are read-only. There are getters but no setters for skin properties. But of course you can change the skin of content objects at any time.

The `Skin` constructor requires a dictionary. If there is a `texture` property in the dictionary, the constructor returns a texture skin, otherwise the constructor returns a color skin.

### Color Skins

Piu handles the `fill`, `stroke` and `borders` properties of the dictionary as usual.

To spare one object, instead of a `borders` property, the dictionary itself can contain the `left`, `right`, `top`, and `bottom` properties.

	const frameSkin = new Skin({ fill:"white", stroke:"black", 
									left:1, right:1, top:1, bottom:1 }); 

### Texture Skins

Piu handles the `texture`, `x`, `y`, `width`, `height`, `variants`, `states` and `tiles` properties  of the dictionary as usual.
	
To spare one object, instead of a `tiles` property, the dictionary itself can contain the `left`, `right`, `top`, and `bottom` properties.

	const buttonSkin = new Skin({ texture:buttonTexture, x:0, y:0, width:60, height:40, 
									states:40, left:20, right:20 });

The dictionary can also contain a `color` property. If the texture has only an alpha bitmap, the value of the `color` property will be used to colorize the bitmap.

## Styles

Styles are read-only. There are getters but no setters for style properties. But of course you can change the style of content objects at any time.

The `Style` constructor requires a dictionary

Piu handles the `color`, `horizontal`, `vertical`, `indentation`, `leading` and `margins` properties of the dictionary as usual.

To spare one object, instead of a `margins ` property, the dictionary itself can contain the `left`, `right`, `top`, and `bottom` properties.

	const buttonStyle = new Style({ color:["white", "black"], left:20, right:20 });

### Fonts

Piu uses bitmap fonts. The metrics are provided by binary FNT files, the glyphs are provided by PNG files. 

Fonts are assets and must be defined in the resources of your manifest. Like for textures here above, use the defaut target or the `-alpha` or `-color` pseudo targets for fonts.

The binary FNT file format was defined by
[AngelCode](http://www.angelcode.com/products/bmfont/doc/file_format.html), which also released the [BMFont](http://www.angelcode.com/products/bmfont/) generator on Windows. On Mac you can for instance use [Glyph Designer](https://71squared.com/glyphdesigner) or [bmGlyph](https://www.bmglyph.com) to generate such files.

One bitmap font corresponds to one style, one weight, one stretch, one size and one family. You will need separate bitmap fonts for each variation.

The `Style` constructor use the `font` property of its dictionary to lookup its font. You can just set the `font` property to the name of a binary FNT file:

	const popStyle = new Style({ font:"popFont" });

The `popStyle` object will use the `popFont.fnt` file in your assets.

The lookup happens only when `label` or `text` objects that use the style are bound to the displayed containment hierarchy, or when the `Style.prototype.measure` method is called.

### Cascading Styles

In order to cascade styles, you may want to use something similar to the [CSS font shortcut](https://developer.mozilla.org/en-US/docs/Web/CSS/font).

	const style = new Style({ font:"italic bold 16px Open Sans" });

In Piu, the font property has five optional parts, in that order: 

1. style: `italic` | `normal` | `inherit`
2. weight: `100` | `ultralight` | `200` | `thin` | `300` | `light` | `400` | `normal` | `500` | `medium` | `600` | `semibold` | `700` | `bold` |  `800` | `heavy` | `900` | `black` | `lighter`  | `bolder`  | `inherit `
3. stretch: `condensed` | `normal` | `inherit `
4. size: `xx-small` | `x-small` | `small` | `medium` | `large` | `x-large` | `xx-large` | `smaller` | `larger ` | `[1-9][0-9]+px` | `[1-9][0-9]+%` | `inherit `
5. family: the rest of the font property if any.

The `inherit` value is the default value and is useful only to disambiguate a font property. By default the style of a `content` object inherits the style of its `container` objects.

All parts are optional and are cascaded separately. So you can for instance define a generic style for the application and specific styles for its contents:

	const appStyle = new Style({ font:"16px Fira Sans" });
	const menuStyle = new Style({ font:"bold" });

Styles are only cascaded when `label` or `text` objects that use them are bound to the displayed containment hierarchy.

For Piu to find the corresponding bitmap font files in your assets, you have to adopt the following convention, based on common practices:

* the family, without spaces,
* `-`,
* the capitalized name of the stretch, if not `normal`,
* the capitalized name of the computed weight, if not `normal`, 
* the capitalized name of the style, if not `normal`,
* `Regular`, if the stretch, the computed weight and the style are all `normal`,
* `-`,
* the computed size in pixels without units.

Here above `style ` will look for `OpenSans-BoldItalic-16.fnt`, `appStyle` will look for `FiraSans-Regular-16.fnt` and `menuStyle` will look for `FiraSans-Bold-16.fnt`.

## Behaviors

Use classes or objects to define behaviors. Piu has no templates for behaviors.

Piu triggers the following events:

- `onCreate(content, $, dictionary)`: 
- `onDisplaying(content)`: 
- `onDraw(port, x, y, width, heigth)`: behaviors of ports only.
- `onFinished(content)`: 
- `onFitHorizontally(layout, width)`: behaviors of layouts only.
- `onFitVertically(layout, height)`: behaviors of layouts only.
- `onMeasureHorizontally(layout, width) `: behaviors of layouts only.
- `onMeasureVertically(layout, height)`: behaviors of layouts only.
- `onScrolled(scroller)`: behaviors of scrollers and their contents. 
- `onTimeChanged(content)`: 
- `onTouchBegan(content, index, x, y, ticks)`: 
- `onTouchCancelled(content, index, x, y, ticks)`: 
- `onTouchEnded(content, index, x, y, ticks)`: 
- `onTouchMoved(content, index, x, y, ticks)`: 
- `onTransitionBeginning(container)`: behaviors of containers only,.
- `onTransitionEnded(container)`: behaviors of containers only.

## Contents

Use templates to build containment hierarchies. Piu has no separate constructors for contents. Both `Content` and `new Content` take the same two parameters: `$` and `dictionary`, and both trigger the `onCreate` event.

- `Content`: Piu has no messages so the `cancel`, `invoke` and `wait` methods do not exist.

- `Label`: neither editable nor selectable. All related accessors and methods do not exist.

- `Text`: neither editable nor selectable. All related accessors and methods do not exist. Inline or floating contents are not yet supported.

- `Port`: only methods supported by the Commodetto graphics engine: `drawString`, `drawTexture`, `fillColor`, `fillTexture`, `measureString`, `popClip` and `pushClip`.

- `Media`: not yet supported.

- `Picture`: not yet supported.

- `Thumbnail`: not yet supported.

- `Container`: use the `contents` property to access contents by index or by name, for instance `container.contents[0]` or `container.contents["foo"]`.

- `Column`: supported.

- `Layout`: supported.

- `Layer`: not supported. There are `die` objects to build animations and transitions, see [Die Cut](./die-cut.md).

- `Row`: Piu `row` objects are KinomaJS `line` objects.

- `Scroller`: Supported.

## Application

There is no `application` object. So you have to create the `application` object yourself with a template and export it in your main module. 

	let BallApplication = Application.template($ => ({
		skin:whiteSkin,
		contents: [
			Content(6, { left:0, top:0, skin:ballSkin, variant:0, Behavior: BallBehavior } ),
			Content(5, { right:0, top:0, skin:ballSkin, variant:1, Behavior: BallBehavior } ),
			Content(4, { right:0, bottom:0, skin:ballSkin, variant:2, Behavior: BallBehavior } ),
			Content(3, { left:0, bottom:0, skin:ballSkin, variant:3, Behavior: BallBehavior } ),
		]
	}));
	
	export default new BallApplication(null, { displayListLength:4096, touchCount:0 });

If you want a global `application` object, set a property of the `global` object.

	global.application = new BallApplication(null, { displayListLength:4096, touchCount:0 });
	export default global.application;

Notice that Piu only supports applications. There is no `shell` object and `host` objects do not exist. Also Piu has no messages so `handler` objects do not exist.

	


