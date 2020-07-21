import Vue from "vue";
import vgl from "vue-golden-layout";
//import vgl from "vue-golden-layout/src";
import "golden-layout/src/css/goldenlayout-dark-theme.css";
import "../public/css/scrollbar.css";
import "./utils/inspectelement";
import "boxicons/css/boxicons.css";
require("typeface-open-sans");

// https://github.com/EmbeddedEnterprises/ng6-golden-layout/blob/master/README.md
import * as $ from "jquery";
(<any>window).$ = $;
(<any>window).JQuery = $;

import App from "./App.vue";
import router from "./router";
import store from "./store";

Vue.config.productionTip = false;
Vue.use(vgl);

import { Titlebar, Color } from "custom-electron-titlebar";
const titleBar = new Titlebar({
	backgroundColor: Color.fromHex("#333333"),
	icon: "./favicon.svg",
	shadow: true
});

new Vue({
	router,
	store,
	render: h => h(App)
}).$mount("#app");
