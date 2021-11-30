import Vue from "vue";
import vgl from "vue-golden-layout";
//import vgl from "vue-golden-layout/src";
import "golden-layout/src/css/goldenlayout-dark-theme.css";
import "../public/css/scrollbar.css";
import "./utils/inspectelement";
import "boxicons/css/boxicons.css";
require("typeface-open-sans");
import * as Sentry from "@sentry/electron";

// toast
import VueToast from "vue-toast-notification";
//import 'vue-toast-notification/dist/theme-default.css';
import "vue-toast-notification/dist/theme-sugar.css";

// https://github.com/EmbeddedEnterprises/ng6-golden-layout/blob/master/README.md
import * as $ from "jquery";
(<any>window).$ = $;
(<any>window).JQuery = $;

if (process.env.VUE_APP_SENTRY_DNS) {
	Sentry.init({ dsn: process.env.VUE_APP_SENTRY_DNS });
}

import App from "./App.vue";
import router from "./router";
import store from "./store";
import VfmPlugin from "vue-final-modal";

Vue.config.productionTip = false;
Vue.use(vgl);
Vue.use(VueToast);
Vue.use(VfmPlugin);

import { Titlebar, Color } from "custom-electron-titlebar";
const titleBar = new Titlebar({
	backgroundColor: Color.fromHex("#333333"),
	icon: "./icon.png",
	shadow: true
});

let app = new Vue({
	router,
	store,
	render: h =>
		h(App, {
			props: {
				titleBar: titleBar
			}
		})
});

app.$mount("#app");
