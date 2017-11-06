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

import {} from "piu/MC";

const backgroundSkin = new Skin({ fill:"white" });

const centerStyle = new Style({ horizontal:"center" });
const leftStyle = new Style({ horizontal:"left" });
const rightStyle = new Style({ horizontal:"right" });
const justifyStyle = new Style({ horizontal:"justify" });
const absoluteLeadingStyle = new Style({ horizontal:"left", leading:30 });
const relativeLeadingStyle = new Style({ horizontal:"left", leading:-80 });
const styles = [ centerStyle, leftStyle, rightStyle, justifyStyle, absoluteLeadingStyle, relativeLeadingStyle ];

const normalStyle = new Style({ font:"semibold 18px Open Sans", color:"black" });
const bigStyle = new Style({ font:"28px" });
const italicStyle = new Style({ font:"italic" });
const linkStyle = new Style({ color:["blue","red"] });

class div extends TextComponent {
	constructor($, it) {
		super($, it);
		this.style = normalStyle;
	}
}
class big extends TextComponent {
	constructor($, it) {
		super($, it);
		this.style = bigStyle;
	}
}
class i extends TextComponent {
	constructor($, it) {
		super($, it);
		this.style = italicStyle;
	}
}
class a extends LinkComponent {
	constructor($, it) {
		super($, it);
		this.style = linkStyle;
	}
	onCreate(link, $) {
		this.$ = $;
	}
	onTouchBegan(link) {
		link.state = 1;
	}
	onTouchEnded(link) {
		link.state = 0;
		trace(this.$ + "\n");
	}
}

class Lorem extends Component {
	onDisplaying(text) {
		this.index = 0;
		text.duration = 1000;
		text.start();
	}
	onFinished(text) {
		let index = this.index + 1;
		if (index >= styles.length) index = 0;
		this.index = index;
		text.style = styles[index];
		text.time = 0;
		text.start();
	}
	render($, it) {
		return <Text {...it} style={centerStyle} active/>
	}
}

export default <Application skin={backgroundSkin} displayListLength={8192} touchCount={1}>
	<Lorem left={0} right={0} top={0}>
		<div>
			<a data="tutu">Lorem ipsum</a> dolor sit amet, consectetur adipiscing elit. <i>Nulla</i> faucibus <big>sodales</big> ligula eu accumsan.
		</div>
		<div>
			Aliquam consectetur eleifend <a data="toto">molestie. <i>Sed</i> dui est, <big>suscipit</big> vitae </a> consequat a, aliquam eget nisl.
		</div>
	</Lorem>
</Application>
