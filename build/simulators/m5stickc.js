import { Screen } from 'piu/Screen';

import {
  ControlsColumn,
  ButtonsRow,
  PopupRow,
  SliderRow,
  StatusRow,
  SwitchRow,
  TimerRow,
} from 'ControlsPane';

import {
  DeviceBehavior,
  DeviceContainer,
  DeviceScreen,
  DeviceWorker,
} from 'DevicePane';

const base = import.meta.uri;
const ledSkin = { texture:{ base, path:"m5stickc/led.png" }, x:0, y:0, width:160, height:160 };

class MockupBehavior extends DeviceBehavior {
  onAbort(container, status) {
    if (this.reloadOnAbort) {
      application.defer('doReloadFile');
      return true;
    }
  }
  onCreate(container, device) {
    super.onCreate(container, device);
    this.reloadOnAbort = false;
  }

  onAButtonDown(container) {
    this.postJSON(container, { aButton: 1 });
  }
  onAButtonUp(container) {
    this.postJSON(container, { aButton: 0 });
  }
  onBButtonDown(container) {
    this.postJSON(container, { bButton: 1 });
  }
  onBButtonUp(container) {
    this.postJSON(container, { bButton: 0 });
  }
  onKeyDown(container, key) {
    const code = key.charCodeAt(0);
    if (code == 97 /* a */) this.onAButtonDown(container);
    if (code == 98 /* b */) this.onBButtonDown(container);
  }
  onKeyUp(container, key) {
    const code = key.charCodeAt(0);
    if (code == 97 /* a */) this.onAButtonUp(container);
    if (code == 98 /* b */) this.onBButtonUp(container);
  }
  onBatteryChanged(container, data) {
    this.postJSON(container, { battery: data.value });
  }
  onBatteryChanged(container, data) {
    this.postJSON(container, { battery: data.value / 100 });
  }
  onJSON(container, json) {
    if ('xsbug' in json) {
      if (json.xsbug == 'abort') application.defer('doReloadFile');
    }
  }
  onReloadOnAbortChanged(container, data) {
    this.reloadOnAbort = data.value;
  }
}

class LEDBehavior extends Behavior {
	onLEDChanged(content, state) {
		content.visible = state;
	}
}

export default {
  applicationName: 'm5stickc/debug/.*',
  title: 'M5stickc',
  Workers: {},
  ControlsTemplate: ControlsColumn.template(($) => ({
    contents: [
      ButtonsRow({
        label: 'Button',
        buttons: [
          {
            eventDown: 'onAButtonDown',
            eventUp: 'onAButtonUp',
            label: 'A',
          },
          {
            eventDown: 'onBButtonDown',
            eventUp: 'onBButtonUp',
            label: 'B',
          },
        ]
      }),
      SliderRow({
        anchor: 'BATTERY_SLIDER',
        label: 'Battery',
        unit: '%',
        active: true,
        min: 0,
        max: 100,
        value: 100,
        step: 1,
        event: 'onBatteryChanged',
      }),
      SwitchRow({
        event: 'onReloadOnAbortChanged',
        label: 'Reload On Abort',
        on: 'Yes',
        off: 'No',
        value: false,
      }),
    ]
  })),
  DeviceTemplates: {
    0: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, { Behavior: LEDBehavior, right:0, bottom:0, skin:ledSkin, visible:false }),
        Content($, {
          skin: {
            texture: { base, path: 'm5stickc/0.png' },
            x: 0,
            y: 0,
            width: 342,
            height: 170,
          },
        }),
        DeviceScreen($, {
          left: 31,
          width: 160,
          top: 45,
          height: 80,
        }),
      ],
    })),
    90: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, { Behavior: LEDBehavior, right:0, bottom:0, skin:ledSkin, visible:false }),
        Content($, {
          skin: {
            texture: { base, path: 'm5stickc/90.png' },
            x: 0,
            y: 0,
            width: 170,
            height: 342,
          },
        }),
        DeviceScreen($, {
          left: 45,
          width: 80,
          top: 31,
          height: 160,
        }),
      ],
    })),
    180: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, { Behavior: LEDBehavior, right:0, bottom:0, skin:ledSkin, visible:false }),
        Content($, {
          skin: {
            texture: { base, path: 'm5stickc/180.png' },
            x: 0,
            y: 0,
            width: 342,
            height: 170,
          },
        }),
        DeviceScreen($, {
          left: 151,
          width: 160,
          top: 45,
          height: 80,
        }),
      ],
    })),
    270: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, { Behavior: LEDBehavior, right:0, bottom:0, skin:ledSkin, visible:false }),
        Content($, {
          skin: {
            texture: { base, path: 'm5stickc/270.png' },
            x: 0,
            y: 0,
            width: 170,
            height: 342,
          },
        }),
        DeviceScreen($, {
          left: 45,
          width: 80,
          top: 151,
          height: 160
        }),
      ],
    })),
  },
};
