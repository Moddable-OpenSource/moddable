import Net from "net";

class System {
}
System.resolve = Net.resolve;

globalThis.System = System;
