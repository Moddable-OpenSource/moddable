# Creating fonts for applications built on the Moddable SDK

Copyright 2017 Moddable Tech, Inc.

Revised: November 16, 2017

Moddable uses the [BMFont](http://www.angelcode.com/products/bmfont/doc/file_format.html) format for fonts

The Moddable SDK supports any input character (up to 4-byte UTF-8 values) making it possible to render the glyphs for any language written with glyphs having a Unicode representation.

Each font asset consists of a (.png) glyph file and a corresponding font metrics file (.fnt) saved in BMFont Binary format.

This allows anti-aliased or non-anti-aliased font sets to be generated as desired. The Poco renderer in the Moddable SDK does not implement a font scaler. Consequently, each required font size/weight must be generated and included as an asset in the application.

BMFont allows an arbitrary number of glyphs in the font.
Moddable supports non-contiguous glyphs ranges allowing the developer or graphic designer to create an efficient font asset with just the characters needed for the application. This can significant reducing asset storage size.

Fonts are generated in black. This allows the Moddable SDK build tools to create alpha masks of the characters. To render the fonts in color, applications apply styles.

The Moddable team uses [Glyph Designer](https://71squared.com/glyphdesigner) (macOS). Other tools are available to create bitmap fonts in the BMFont format.

## Glyph Designer instructions

Select font and weight. Set font color to black and no outline.

![](../assets/create-fonts/screen01.png)

Using the character selection tool select needed characters.

![](../assets/create-fonts/screen02.png)

Export .png glyph and .fnt font metrics files in the BMFont Binary format.

![](../assets/create-fonts/screen03.png)