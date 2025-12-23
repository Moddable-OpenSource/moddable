interface MessageOptions {
  onReadable?: () => void;
  onWritable?: (count: number) => void;
  onSuspend?: () => void;
  format?: "map";
  input?: number;
  output?: number;
  keys?: Map<string, number>;
}

type MessageKey = string | number;
type MessageReadValue = number | string | ArrayBuffer;
type MessageWriteValue = number | string | BufferLike | boolean;

declare class Message {
  constructor(options: MessageOptions);
  close(): void;
  read(): Map<MessageKey, MessageReadValue>;
  write(map: Map<MessageKey, MessageWriteValue>): void;
  get format(): "map";
  set format(value: "map");
}

export default Message;
