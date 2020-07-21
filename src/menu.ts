// https://www.tutorialspoint.com/electron/electron_menus.htm
// https://programmer.help/blogs/use-electron-to-customize-menus.html
// https://electronjs.org/docs/api/menu
// https://alan.fyi/renderer-menu-functions-in-electron-vue/

const { app, Menu, BrowserWindow } = require("electron");

export enum MenuCommands {
	FileNew = "file_new",
	FileOpen = "file_open",
	FileSave = "file_save",
	FileSaveAs = "file_saveas",
	FileExit = "file_exit",

	EditUndo = "edit_undo",
	EditRedo = "edit_redo",
	EditCut = "edit_cut",
	EditCopy = "edit_copy",
	EditPaste = "edit_paste",

	ExportZip = "export_zip",
	ExportUnity = "export_unity",
	ExportUnityZip = "export_unity_zip",

	ExamplesGoldLinesMarbleTiles = "samples_1",
	ExamplesGrenade = "samples_2",
	ExamplesScrews = "samples_3",
	ExamplesWoodenPlanks = "samples_4",

	HelpTutorials = "help_tutorials",
	HelpAbout = "help_about",
	HelpSubmitBug = "help_submitbug"
}

export function setupMenu() {
	const template = [
		{
			label: "File",
			submenu: [
				{
					label: "New",
					accelerator: "CmdOrCtrl+N",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.FileNew);
					}
				},
				{
					label: "Open",
					accelerator: "CmdOrCtrl+O",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.FileOpen);
					}
				},
				{
					label: "Save",
					accelerator: "CmdOrCtrl+S",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.FileSave);
					}
				},
				{
					label: "Save As..",
					accelerator: "CmdOrCtrl+Shift+S",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.FileSaveAs);
					}
				},
				{
					label: "Exit",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.FileExit);
					}
				}
			]
		},
		{
			label: "Edit",
			submenu: [
				{
					label: "Undo",
					accelerator: "CmdOrCtrl+Z",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.EditUndo);
					}
				},
				{
					label: "Redo",
					accelerator: "CmdOrCtrl+Shift+Z",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.EditRedo);
					}
				},
				{
					label: "Cut",
					accelerator: "CmdOrCtrl+X",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.EditCut);
					}
				},
				{
					label: "Copy",
					accelerator: "CmdOrCtrl+C",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.EditCopy);
					}
				},
				{
					label: "Paste",
					accelerator: "CmdOrCtrl+V",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.EditPaste);
					}
				}
			]
		},
		{
			label: "Export",
			submenu: [
				{
					label: "Zip",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.ExportZip);
					}
				},
				{
					label: "Unity Material",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.ExportUnity);
					}
				},
				{
					label: "Unity (Zip)",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.ExportUnityZip);
					}
				}
			]
		},
		{
			label: "Examples",
			submenu: [
				{
					label: "GoldLinedMarbleTiles",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(
							MenuCommands.ExamplesGoldLinesMarbleTiles
						);
					}
				},
				{
					label: "Grenade",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.ExamplesGrenade);
					}
				},
				{
					label: "Screws",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.ExamplesScrews);
					}
				},
				{
					label: "WoodenPlanks",
					click: (item, focusedWindow) => {
						focusedWindow.webContents.send(MenuCommands.ExamplesWoodenPlanks);
					}
				}
			]
		},
		// {
		//   label: "Help",
		//   submenu: [
		//     {
		//       label: "Tutorials"
		//     },
		//     {
		//       label: "About"
		//     },
		//     {
		//       label: "Submit Bug"
		//     }
		//   ]
		// },
		...(process.env.NODE_ENV !== "production"
			? [
					{
						label: "Dev",
						submenu: [
							{ role: "reload" },
							{
								role: "forcereload",
								click: function(item, focusedWindow) {
									if (focusedWindow) {
										// After overloading, refresh and close all secondary forms
										if (focusedWindow.id === 1) {
											BrowserWindow.getAllWindows().forEach(function(win) {
												if (win.id > 1) {
													win.close();
												}
											});
										}
										focusedWindow.reload();
									}
								}
							},
							{ role: "toggledevtools" },
							{ type: "separator" },
							{ role: "resetzoom" },
							{ role: "zoomin" },
							{ role: "zoomout" },
							{ type: "separator" },
							{ role: "togglefullscreen" }
						]
					}
			  ]
			: [])
	];

	//console.log(template);

	const menu = Menu.buildFromTemplate(template as any);
	Menu.setApplicationMenu(menu);
}
