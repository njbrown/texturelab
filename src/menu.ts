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

  ExportZip = "export_zip",
  ExportUnity = "export_unity",
  ExportUnityZip = "export_unity_zip",

  SamplesBrick = "samples_brick",

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
      label: "Export",
      submenu: [
        {
          label: "Zip",
          click: (item, focusedWindow) => {
            focusedWindow.webContents.send(MenuCommands.ExportZip);
          }
        },
        // {
        //   label: "Unity Material",
        //   click: (item, focusedWindow) => {
        //     focusedWindow.webContents.send(MenuCommands.ExportUnity);
        //   }
        // },
        {
          label: "Unity (Zip)",
          click: (item, focusedWindow) => {
            focusedWindow.webContents.send(MenuCommands.ExportUnityZip);
          }
        }
      ]
    },
    {
      label: "Samples",
      submenu: [
        {
          label: "Lava"
        },
        {
          label: "Water"
        },
        {
          label: "Wood"
        }
      ]
    },
    {
      label: "Help",
      submenu: [
        {
          label: "Tutorials"
        },
        {
          label: "About"
        },
        {
          label: "Submit Bug"
        }
      ]
    },
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
  ];

  const menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
}
