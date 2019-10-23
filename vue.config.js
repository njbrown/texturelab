// vue.config.js

module.exports = {
	pluginOptions: {
		electronBuilder: {
			chainWebpackMainProcess: config => {
				config.plugins.delete("uglify");
			},
			chainWebpackRendererProcess: config => {
				config.plugins.delete("uglify");
			}
		}
	}
};
