import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";
import WiFi from "wifi";

function getVariantFromSignalLevel(value) {
	let low = -120;
	let high = -40;
	if (value < low) value = low;
	if (value > high) value = high;
	let result = Math.round(4 * ((value - low) / (high - low)));
// 	trace(value + " " + result + "\n");
	return result;
}

function wifiScan(target) {
	controller.wifiTarget = target;
	if (controller.wifiScanning)
		return;
	controller.wifiScanning = true;
	WiFi.scan({}, item => {
		if (item && item.ssid)
			controller.wifiTarget.delegate("onWifiScan", item);
		else {
			controller.wifiScanning = false;
			controller.wifiTarget.delegate("onWifiScanDone");
		}
	});
}

class NetworksBehavior extends View.Behavior {
	onBack(container) {
		container.stop();
		wifiScan(application);
		controller.goBack();
	}
	onCreate(container, view) {
		super.onCreate(container, view);
		container.duration = 3000;
	}
	onDialogNo(container) {
	}
	onDialogYes(container) {
		const view = this.view;
		const column = view.LIST;
		const current = column.first;
		const data = current.behavior.data;
		column.remove(current);
		if (data.rssi) {
			delete data.current;
			delete view.itemData;
			let row = column.first;
			while (row) {
				const delta = row.behavior.data.ssid.localeCompare(data.ssid);
				if (delta > 0) {
					column.insert(new NetworkContainer(data, { view }), row);
					break;
				}
				row = row.next;
			}
			if (!row)
				column.add(new NetworkContainer(data, { view }), row);
		}
		controller.wifi = {};
	}
	onDisplaying(container) {
		wifiScan(container);
	}
	onFinished(container) {
		wifiScan(container);
	}
	onItemSelected(container, data) {
		const view = this.view;
		const column = view.LIST;
		if (data.current) {
			column.first.delegate("onTouchCancelled");
			container.run(new View.DialogTransition(200, 1), new View.DialogContainer(`Forget this network?`));
		}
		else {
			container.stop();
			wifiScan(application);
		
			let networks = new Array(column.length);
			let row = column.first;
			let index = 0;
			while (row) {
				networks[index] = row.behavior.data;
				row = row.next;
				index++;
			}
			view.networks = networks;
		
			if (data.authentication == "none")
				controller.goTo("GetTime", data);
			else
				controller.goTo("Password", data);
		}
	}
	onWifiScan(container, item) {
		const view = this.view;
		const column = view.LIST;
		const { ssid, rssi, authentication } = item;
		const password = "";
		let row = column.first;
		while (row) {
			const data = row.behavior.data;
			const delta = data.ssid.localeCompare(item.ssid);
			if (delta == 0) {
				if (data.rssi != item.rssi) {
					data.rssi = item.rssi;
					row.last.previous.variant = getVariantFromSignalLevel(item.rssi);
				}
				if (data.authentication != item.authentication) {
					data.authentication = item.authentication;
					row.last.previous.state = (item.authentication == "none") ? 0 : 1;
				}
				break;
			}
			else if ((!data.current) && (delta > 0)) {
				column.insert(new NetworkContainer({ ssid, rssi, authentication, password }, { view }), row);
				break;
			}
			row = row.next;
		}
		if (!row)
			column.add(new NetworkContainer({ ssid, rssi, authentication, password }, { view }), row);
	}
	onWifiScanDone(container) {
		container.time = 0;
		container.start();
	}
}

const NetworksContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:NetworksBehavior,
	contents: [
		Container($, {
			left:0, top:0, skin:assets.skins.topArc2,
			contents: [
				Content($, { left:0, width:50, top:0, height:50, skin:assets.skins.back, active:true, Behavior:View.BackBehavior }),
				Label($, { top:5, style:assets.styles.title, string:$.data.title }),
			]
		}),
		Container($, {
			left:0, width:240, top:60, bottom:0,
			contents: [
				Content($, { left:0, right:0, top:0, height:10, skin:assets.skins.topShadow }),
				Scroller($, {
					left:0, width:240, top:0, bottom:0, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
					contents: [
						Column($, { 
							anchor:"LIST", left:0, right:0, top:0, contents: $.networks.map($$ => new NetworkContainer($$, { view:$ })),
						}),
						VerticalScrollbar($, {}),
					]
				}),
			]
		}),
	]
}));

const NetworkContainer = Row.template($ => ({
	left:0, right:22, height:40, 
	skin:$.current ? assets.skins.current : assets.skins.setting, 
	style:$.current ? assets.styles.current : assets.styles.setting, 
	active:true, Behavior:View.ItemBehavior,
	contents: [
		Label($, { left:20, right:0, style:assets.styles.LEFT, string:$.ssid }),
		Content($, { skin:assets.skins.wifiStrip, variant:getVariantFromSignalLevel($.rssi), state:($.authentication == "none") ? 0 : 1 }),
		Content($, { width:8 }),
	],
}));

class VerticalScrollbarBehavior extends Behavior {
	onCreate(scrollbar) {
		this.former = 0;
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		var thumb = scrollbar.first;
		var size = scroller.height;
		var range = scroller.first.height;
		if (size < range) {
			var height = scrollbar.height;
			thumb.y = scrollbar.y + Math.round(scroller.scroll.y * height / range);
			thumb.height = Math.round(height * size / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.height = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 3;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbY = thumb.y;
		let thumbHeight = thumb.height;
		if ((y < thumbY) || ((thumbY + thumbHeight) <= y))
			this.anchor = 0 - (thumbHeight >> 1);
		else
			this.anchor = thumbY - y;
		this.min = scrollbar.y;
		this.max = scrollbar.y + scrollbar.height - thumbHeight;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.container;
		var thumb = scrollbar.first;
		y += this.anchor;
		if (y < this.min)
			y = this.min;
		else if (y > this.max)
			y = this.max;
		thumb.y = y;

		scroller.scrollTo(scroller.scroll.x, (y - this.min) * (scroller.first.height / scrollbar.height));
	}
};

const VerticalScrollbar = Container.template($ => ({
	width:40, right:0, top:5, bottom:2, active:true, Behavior:VerticalScrollbarBehavior,
	contents: [
		Content($, { right:5, width:10, top:0, height:0, skin:assets.skins.scrollbarThumb }),
	],
}));

export default class extends View {
	constructor(data) {
		super(data);
		const wifi = controller.wifi;
		if (wifi.ssid)
			this.networks = [ wifi ];
		else
			this.networks = [];
		this.wifi = wifi;
	}
	get Template() { return NetworksContainer }
};
