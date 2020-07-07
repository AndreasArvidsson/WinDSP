const list = [
    { name: "Front left", code: "L" },
    { name: "Front right", code: "R" },
    { name: "Center", code: "C" },
    { name: "Subwoofer", code: "SW" },
    { name: "Surround left", code: "SL" },
    { name: "Surround right", code: "SR" },
    { name: "Surround back left", code: "SBL" },
    { name: "Surround back right", code: "SBR" }
];

const byCodeMap = {};
const byNameMap = {};

let i = 0;
for (const lang of list) {
    byCodeMap[lang.code] = lang;
    byNameMap[lang.name] = lang;
    lang.order = i++;
}

export default {
    getList: function () {
        return list;
    },
    getByCode: function (code) {
        return byCodeMap[code];
    },
    getByName: function (name) {
        return byNameMap[name];
    },
    getCode: function (name) {
        return byNameMap[name].code;
    },
    getName: function (code) {
        return byCodeMap[code].name;
    },
    sort: function(langCodes) {
        langCodes.sort(function(a, b) {
            return byCodeMap[a].order - byCodeMap[b].order;
        });
    }
};
