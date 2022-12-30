/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import CLI from "cli";
import Flash from "flash";

CLI.install(function(command, parts) {
	if ("help" === command) {
		this.line("archive dump - display installed archive");
		this.line("archive erase - uninstall archive");
		return;
	}

	if ("archive" !== command)
		return false;

	switch (parts.shift().toLowerCase()) {
		case "dump":
			let archive = new Flash("xs");

			let root = readAtom(archive, 0);
			if (root.name != "XS_A")
				throw new Error("unexpected atom " + root.name);

			this.line(`Archive size: ${root.byteLength} bytes`);

			for (let walker = 8; walker < root.byteLength; ) {
				let atom = readAtom(archive, walker), bytes;
				let position = walker + 8;

				this.line(`Atom ${atom.name}, position ${walker}, bytes ${atom.byteLength - 8}`);

				switch (atom.name) {
					case "VERS":
						bytes = new Uint8Array(archive.read(position, 4));
						this.line(` XS_MAJOR_VERSION: ${bytes[0]}`);
						this.line(` XS_MINOR_VERSION: ${bytes[1]}`);
						this.line(` XS_PATCH_VERSION: ${bytes[2]}`);
						this.line(` FLAG: ${bytes[3]}`);
						break;

					case "SYMB":
						bytes = new Uint8Array(archive.read(position, 2));
						position += 2;
						let count = (bytes[0] << 8) | bytes[1];
						count = (count >> 8) | ((count & 0xFF) << 8);
						while (count--) {
							const name = readString(archive, position);
							position += name.length + 1;
							this.line(` ${name}`);
						}
						break;

			//@@ MODS & RSRC
					case "NAME":
						this.line(` ${readString(archive, position)}`);
						break;
				}

				walker += atom.byteLength;
			}
			break;

		case "erase":
			(new Flash("xs")).erase(0);
			break;

		default:
			throw new Error("unrecognized option");
	}

	return true;
});

function readAtom(archive, position) {
	let bytes = new Uint8Array(archive.read(position, 8));
	let byteLength = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
	if ((byteLength < 8) || (byteLength > 0x100000))
		throw new Error("invalid atom size");
	let name = String.fromCharCode(bytes[4], bytes[5], bytes[6], bytes[7]);
	return {name, byteLength};
}

function readString(archive, position) {
	let result = "";
	while (true) {
		let bytes = new Uint8Array(archive.read(position++, 1));
		if (0 == bytes[0])
			break;
		result += String.fromCharCode(bytes[0]);
	}
	return result;
}
