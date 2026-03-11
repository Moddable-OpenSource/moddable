import files from "embedded:storage/files"

function scan(files, path) {
	const result = [];
	for (const item of files.scan(path))
		result.push(item);
	return result.sort();
}

function deleteTree(files, path) {
	try {
		if (files.status(path).isFile())
			return void files.delete(path);
	}
	catch {
		return;
	}

	if (files.status(path).isDirectory()) {
		for (const name of files.scan(path))
			deleteTree(files, path + "/" + name);
	}

	files.delete(path);
}

files.createDirectory("tmp");
const tmp = files.openDirectory({path: "tmp"});
export {tmp as default, deleteTree, scan}
