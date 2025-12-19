export { Archive } from "Archive";

export default class ArchivePebbleResource extends Native("ArchivePebbleResourceDelete") {
	constructor(id) { super(); native("ArchivePebbleResourceCreate").call(this, id); }
	close() { return native("ArchivePebbleResource_close").call(this); }
	get archive() { return native("ArchivePebbleResource_get_archive").call(this); }
}
