interface Trace {
	(log: string):void;
	left(log: string):void;
	center(log: string):void;
	right(log: string):void;
}
declare const trace:Trace;
type HostBuffer = {
  slice(begin: number, end?: number): ArrayBuffer
};
// Compartment?