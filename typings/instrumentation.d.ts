declare module "instrumentation" {
  type PixelsDrawn = 1;
  type FramesDrawn = 2;
  type NetworkBytesRead = 3;
  type NetworkBytesWritten = 4;
  type NetworkSockets = 5;
  type Timers = 6;
  type Files = 7;
  type POCODisplayListUsed = 8;
  type PIUCommandListUsed = 9;
  type SystemFreeMemory = 10;
  type SlotHeapSize = 11;
  type ChunkHeapSize = 12;
  type KeysUsed = 13;
  type GarbageCollectionCount = 14;
  type ModulesLoaded = 15;
  type StackPeak = 16;
  type InstrumentationOption = (
    PixelsDrawn |
    FramesDrawn |
    NetworkBytesRead |
    NetworkBytesWritten |
    NetworkSockets |
    Timers |
    Files |
    POCODisplayListUsed |
    PIUCommandListUsed |
    SystemFreeMemory |
    SlotHeapSize |
    ChunkHeapSize |
    KeysUsed |
    GarbageCollectionCount |
    ModulesLoaded |
    StackPeak
  );
  var Instrumentation: {
   get: (what: InstrumentationOption) => number;
  };
  export {Instrumentation as default};
}
