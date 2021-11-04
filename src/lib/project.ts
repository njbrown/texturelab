import fs from "fs";

interface ExportSettings {
	destination: string;
	pattern: string;
}

export class Project {
	name: string = null;
	path: string = null;

	data: object = null;

	getExportSettings() {}
}

export class ProjectManager {
	static load(path: string): Project {
		const project = new Project();

		project.path = path;

		const fileName = path.replace(/^.*[\\/]/, "");
		project.name = fileName.substr(0, fileName.lastIndexOf(".")) || fileName;

		// project.name = path.replace(/^.*[\\/]/, "");
		project.data = JSON.parse(fs.readFileSync(path).toString());
		return project;
	}

	static save(path: string, project: Project) {
		// console.log(project.data);
		// console.log(JSON.stringify(project.data));
		fs.writeFileSync(path, JSON.stringify(project.data));
	}
}
