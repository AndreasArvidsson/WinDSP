import React from "react";
import PropTypes from "prop-types";
import StringUtil from "../util/StringUtil";
import FilterCrossover from "./FilterCrossover";
import FilterShelf from "./FilterShelf";
import FilterPEQ from "./FilterPEQ";
import FilterBP from "./FilterBP";
import FilterNotch from "./FilterNotch";
import FilterLT from "./FilterLT";
import FilterBiquad from "./FilterBiquad";
import FilterFIR from "./FilterFIR";

const Filter = ({ json, onChange }) => {
    const FilterComponent = getFilter(json.type);
    if (FilterComponent) {
        return <FilterComponent json={json} onChange={onChange} />;
    }
    return null;
}

Filter.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

Filter.types = ["LOW_PASS", "HIGH_PASS", "LOW_SHELF", "HIGH_SHELF", "PEQ", "BAND_PASS", "NOTCH", "LINKWITZ_TRANSFORM", "BIQUAD", "FIR"].map(type => {
    let text;
    switch (type) {
        case "PEQ":
        case "FIR":
            text = type;
            break;
        default:
            text = StringUtil.clean(type);
    }
    return { value: type, text };
})

Filter.init = (json) => {
    const FilterComponent = getFilter(json.type);
    if (FilterComponent) {
        FilterComponent.init(json);
    }
}

Filter.toString = (json) => {
    const FilterComponent = getFilter(json.type);
    if (FilterComponent) {
        const type = Filter.types.find(t => t.value === json.type);
        return `${type.text}\n`
            + `${FilterComponent.toString(json)}`;
    }
    return null;
}

export default Filter;

function getFilter(type) {
    switch (type) {
        case "LOW_PASS":
        case "HIGH_PASS":
            return FilterCrossover;
        case "LOW_SHELF":
        case "HIGH_SHELF":
            return FilterShelf;
        case "PEQ":
            return FilterPEQ;
        case "BAND_PASS":
            return FilterBP;
        case "NOTCH":
            return FilterNotch;
        case "LINKWITZ_TRANSFORM":
            return FilterLT;
        case "BIQUAD":
            return FilterBiquad;
        case "FIR":
            return FilterFIR;
    }
    console.error("Unknown filter type: " + type);
    return null;
}