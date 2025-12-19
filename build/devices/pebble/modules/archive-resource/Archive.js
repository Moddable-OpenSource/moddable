export class Archive extends Native("Archive_destructor") {
	constructor() { super(); native("Archive_constructor").call(this); }
	get modulePaths() { return native("Archive_get_modulePaths").call(this); }
	get resourcePaths() { return native("Archive_get_resourcePaths").call(this); }
	get name() { return native("Archive_get_name").call(this); }
};