/*
	SVGImage
*/

interface SVGImageDictionary extends ContentDictionary {
	clip?: boolean;
	id?: number;
}

interface SVGImage extends Content {
	opacity: number;
	center(x: number, y: number): void;
	cx: number;
	cy: number;
	rotate(a: number): void;
	r: number;
	scale(x: number, y: number): void;
	s: number;
	sx: number;
	sy: number;
	translate(x: number, y: number): void;
	tx: number;
	ty: number;
}

interface SVGImageConsructor {
	new(behaviorData?: any, dictionary?: SVGImageDictionary): SVGImage;
	template<T>(this: T, fn: (arg: any) => SVGImageDictionary): T;
}

/*
	RoundRect
*/

interface RoundRectDictionary extends ContentDictionary {
	corners?: number;
	radius?: number;
}

interface RoundRect extends Content {
	corners: number;
	radius: number;
}

interface RoundRectConstructor {
	new(behaviorData?: any, dictionary?: RoundRectDictionary): RoundRect;
	template<T>(this: T, fn: (arg: any) => RoundRectDictionary): T;

	readonly topLeft: 1;
	readonly topRight: 2;
	readonly bottomLeft: 4;
	readonly bottomRight: 8;
}

/*
	Inverter
*/

interface Inverter extends Content {
}

interface InverterConstructor {
	new(behaviorData?: any, dictionary?: ContentDictionary): Inverter;
	template<T>(this: T, fn: (arg: any) => ContentDictionary): T;
}

/*
	ScreenBuffer
*/

interface ScreenBuffer extends Content {
}

interface ScreenBufferConstructor {
	new(behaviorData?: any, dictionary?: ContentDictionary): ScreenBuffer;
	template<T>(this: T, fn: (arg: any) => ContentDictionary): T;
}
