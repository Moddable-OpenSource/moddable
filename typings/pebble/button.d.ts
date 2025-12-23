type ButtonType = "back" | "up" | "down" | "select";

interface PebbleButtonSingleOptions {
  type: ButtonType;
  onPush: (pushed: 0 | 1, type: ButtonType) => void;
}

interface PebbleButtonMultipleOptions {
  types: ButtonType[];
  onPush: (pushed: 0 | 1, type: ButtonType) => void;
}

type PebbleButtonOptions = PebbleButtonSingleOptions | PebbleButtonMultipleOptions;

declare class PebbleButton {
  constructor(options: PebbleButtonOptions);
  close(): void;
}

export default PebbleButton;
