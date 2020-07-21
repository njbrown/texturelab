const remote = require("electron").remote;
const Menu = remote.Menu;
const MenuItem = remote.MenuItem;

let rightClickPosition = null;
if (process.env.NODE_ENV != "production") {
	const menu = new remote.Menu();
	const menuItem = new MenuItem({
		label: "Inspect Element",
		click: () => {
			remote
				.getCurrentWindow()
				.webContents.inspectElement(rightClickPosition.x, rightClickPosition.y);
		}
	});
	menu.append(menuItem);

	window.addEventListener(
		"contextmenu",
		e => {
			e.preventDefault();
			rightClickPosition = { x: e.x, y: e.y };
			menu.popup();
		},
		false
	);
}
