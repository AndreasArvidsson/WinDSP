import React from "react";
import PropTypes from "prop-types";
import Dropdown from "../inputs/Dropdown";
import Number from "../inputs/Number";
import StringUtil from "../util/StringUtil";
import QValues from "./QValues";

const crossoverTypes = ["BUTTERWORTH", "LINKWITZ_RILEY", "BESSEL", "CUSTOM"].map(type => {
    return { value: type, text: StringUtil.clean(type) };
});

const CrossoverFilter = ({ json, onChange }) => {
    if (!json.crossoverType) {
        json.crossoverType = crossoverTypes[0].value;
    }
    if (!json.order) {
        json.order = getOrders(json.crossoverType)[0].value;
    }

    const isCustom = () => {
        return json.crossoverType === "CUSTOM";
    }

    const myOnchange = (field, value) => {
        if (isCustom()) {
            if (json.qOffset !== undefined) {
                delete json.qOffset;
            }
        }
        else {
            if (json.q !== undefined) {
                delete json.q;
            }
        }
        if (field === "crossoverType") {
            json.order = getOrders(value)[0].value;
        }
        onChange();
    }

    return (
        <React.Fragment>
            <Dropdown json={json} field="crossoverType" label="Crossover type" values={crossoverTypes} onChange={myOnchange} />
            <Dropdown json={json} field="order" label="Order" values={getOrders(json.crossoverType)} onChange={myOnchange} />
            <Number json={json} field="freq" label="Freq" min={1} onChange={myOnchange} />
            {(isCustom() && json.order > 1) &&
                <QValues key={json.order} json={json} onChange={myOnchange} />
            }
            {!isCustom() &&
                <Number json={json} field="qOffset" label="Q-offset" onChange={myOnchange} />
            }
        </React.Fragment>
    );
}

CrossoverFilter.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

CrossoverFilter.init = (json) => {
    json.crossoverType = crossoverTypes[0].value;
    json.order = getOrders(json.crossoverType)[0].value;
    json.freq = 80;
    json.qOffset = 0;
}

CrossoverFilter.toString = (json) => {
    const type = crossoverTypes.find(t => t.value === json.crossoverType);
    return `Type: ${type.text}\n`
        + `Frequency: ${json.freq}Hz\n`
        + `Order: ${json.order} (${json.order * 6}dB/oct)`
        + (json.qOffset !== undefined ? `\nQ-offset: ${json.qOffset}` : "");
}

export default CrossoverFilter;

function getOrders(crossoverType) {
    let orders;
    switch (crossoverType) {
        case "BESSEL":
            orders = [2, 3, 4, 5, 6, 7, 8];
            break;
        case "LINKWITZ_RILEY":
            orders = [2, 4, 8];
            break;
        default:
            orders = [1, 2, 3, 4, 5, 6, 7, 8];
            break;
    }
    return orders.map(order => {
        return {
            value: order,
            text: `${order} (${order * 6}dB/oct)`
        }
    });
}