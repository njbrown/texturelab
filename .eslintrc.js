module.exports = {
	root: true,
	env: {
		node: true
	},
	extends: [
		"plugin:vue/essential",
		"eslint:recommended",
		"@vue/prettier",
		"@vue/typescript"
	],
	rules: {
		// "no-console": process.env.NODE_ENV === "production" ? "error" : "off",
		// "no-debugger": process.env.NODE_ENV === "production" ? "error" : "off",
		"no-console": "off",
		"no-debugger": "off",
		indent: "off",
		allowIndentationTabs: true
	},
	parserOptions: {
		parser: "@typescript-eslint/parser",
		ecmaVersion: 6
	}
};
