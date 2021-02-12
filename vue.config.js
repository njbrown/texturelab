// vue.config.js

module.exports = {
	pluginOptions: {
		electronBuilder: {
			chainWebpackMainProcess: config => {
				config.plugins.delete("uglify");
			},
			chainWebpackRendererProcess: config => {
				config.plugins.delete("uglify");
			},
			builderOptions:{
				win:{
					icon:"build/icons/icon512x512.png"
				}
			}
		}
	}
};
