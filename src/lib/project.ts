import fs from "fs";

export class Project {
	name: string = null;
	path: string = null;

	data: object = null;
}

export class ProjectManager {
	static load(path: string): Project {
		const project = new Project();

		project.path = path;
		project.name = path.replace(/^.*[\\/]/, "");
		project.data = JSON.parse(fs.readFileSync(path).toString());
		return project;
	}

	static save(path: string, project: Project) {
		// console.log(project.data);
		// console.log(JSON.stringify(project.data));
		fs.writeFileSync(path, JSON.stringify(project.data));
	}
}
