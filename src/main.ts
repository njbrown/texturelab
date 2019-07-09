import Vue from "vue";
import vgl from "vue-golden-layout/src";
import "golden-layout/src/css/goldenlayout-dark-theme.css";

// https://github.com/EmbeddedEnterprises/ng6-golden-layout/blob/master/README.md
import * as $ from "jquery";
(<any>window).$ = $;
(<any>window).JQuery = $;

import App from "./App.vue";
import router from "./router";
import store from "./store";

Vue.config.productionTip = false;
Vue.use(vgl);

new Vue({
  router,
  store,
  render: h => h(App)
}).$mount("#app");
