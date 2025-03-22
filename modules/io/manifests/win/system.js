import Net from "net";
import Timer from "timer";

class System {
}
System.resolve = Net.resolve;
System.setInterval = Timer.repeat;
System.clearInterval = Timer.clear;

globalThis.System = System;
