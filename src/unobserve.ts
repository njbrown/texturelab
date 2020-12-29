import { Vue } from "vue-property-decorator";

const Observer = (new Vue()).$data
.__ob__
.constructor;

// prevent Vue from auto-binding to large objects
// causes app to crash when cpu-based nodes with large
// arrays get observed
// https://github.com/rpkilby/vue-nonreactive/blob/master/vue-nonreactive.js
export function unobserve(obj):any {
    // unobserve pr
    obj.__ob__ = new Observer({});
    return obj;
}