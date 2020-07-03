interface Trace {
	(log: string):void;
	left(log: string):void;
	center(log: string):void;
	right(log: string):void;
}
declare const trace:Trace;

// Compartment?