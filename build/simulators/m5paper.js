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
  onCButtonDown(container) {
    this.postJSON(container, { cButton: 1 });
  }
  onCButtonUp(container) {
    this.postJSON(container, { cButton: 0 });
  }

  onKeyDown(container, key) {
    const code = key.charCodeAt(0);
    if (code == 97 /* a */) this.onAButtonDown(container);
    if (code == 98 /* b */) this.onBButtonDown(container);
    if (code == 99 /* c */) this.onCButtonDown(container);
  }
  onKeyUp(container, key) {
    const code = key.charCodeAt(0);
    if (code == 97 /* a */) this.onAButtonUp(container);
    if (code == 98 /* b */) this.onBButtonUp(container);
    if (code == 99 /* c */) this.onCButtonUp(container);
  }
  onBatteryChanged(container, data) {
    this.postJSON(container, { battery: data.value });
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

export default {
  applicationName: 'm5paper/debug/.*',
  sortingTitle: '0004',
  title: 'M5Paper',
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
          {
            eventDown: 'onCButtonDown',
            eventUp: 'onCButtonUp',
            label: 'C',
          },
        ],
      }),
      SliderRow({
        anchor: 'BATTERY_SLIDER',
        label: 'Battery',
        unit: 'mV',
        active: true,
        min: 3300,
        max: 4300,
        value: 4300,
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
    ],
  })),
  DeviceTemplates: {
    0: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, {
          skin: {
            texture: { base, path: 'm5paper/0.png' },
            x: 0,
            y: 0,
            width: 1125,
            height: 662,
          },
        }),
        DeviceScreen($, {
          left: 50,
          width: 960,
          top: 63,
          height: 540,
        }),
      ],
    })),
    90: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, {
          skin: {
            texture: { base, path: 'm5paper/90.png' },
            x: 0,
            y: 0,
            width: 662,
            height: 1125,
          },
        }),
        DeviceScreen($, {
          left: 63,
          width: 540,
          top: 115,
          height: 960,
        }),
      ],
    })),
    180: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, {
          skin: {
            texture: { base, path: 'm5paper/0.png' },
            x: 0,
            y: 0,
            width: 1125,
            height: 662,
          },
        }),
        DeviceScreen($, {
          left: 50,
          width: 960,
          top: 63,
          height: 540,
        }),
      ],
    })),
    270: DeviceContainer.template(($) => ({
      Behavior: MockupBehavior,
      contents: [
        Content($, {
          skin: {
            texture: { base, path: 'm5paper/270.png' },
            x: 0,
            y: 0,
            width: 662,
            height: 1125,
          },
        }),
        DeviceScreen($, {
          left: 59,
          width: 540,
          top: 50,
          height: 960,
          pixelFormat: 'Gray16',
        }),
      ],
    })),
  },
};
