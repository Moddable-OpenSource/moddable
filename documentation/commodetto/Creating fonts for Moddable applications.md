# Creating fonts for Moddable applications

Copyright 2017 Moddable Tech, Inc.

Revised: November 16, 2017

Moddable uses the BMFont format for fonts

The Moddable SDK supports any input character (up to 4-byte UTF-8 values) making it possible to render the glyphs for any language written with glyphs having a Unicode representation.


Each font asset consists of a (.png) glyph file and a corresponding font metrics file (.fnt) saved in BMFont Binary format.

This allows anti-aliased or non-anti-aliased font sets to be generated as desired. There is no font scaling in Moddable apps so every font size/weight needs to be generated as an asset for the application.

BMFont allows an arbitrary number of glyphs in the font.
Moddable supports non-contigious character glyphs so the developer or graphic designer can create an effiecent font asset with just the characters needed for the application greatly reducing asset storage sizes.

Fonts are generated in black. This allows the Moddable SDK build tool to create alpha masks of the characters. Developers can apply sytles the fonts to color them in the application.

The Moddable team uses Glyph Designer (macOS). There are other tools to create bitmap fonts in the BMFont format.

## Glyph Designer instructions

Select font and wieght. Set font color to black and no outline.

![](../assets/create-fonts/screen01.png)

Using the character selection tool select needed characters.

![](../assets/create-fonts/screen02.png)

Export .png glyph and .fnt font metrics files in the BMFont Binary format.

![](../assets/create-fonts/screen03.png)