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

    const myOnchange = () => {
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
        onChange();
    }

    const getCrossoverType = () => {
        return <Dropdown json={json} field="crossoverType" label="Crossover type" values={crossoverTypes} onChange={myOnchange} />
    }

    const getQOffset = () => {
        if (!isCustom()) {
            return <Number json={json} field="qOffset" label="Q-offset" onChange={myOnchange} />
        }
    }

    const getQ = () => {
        if (isCustom()) {
            return <QValues key={json.order} json={json} onChange={myOnchange} />
        }
    }

    const isCustom = () => {
        return json.crossoverType === "CUSTOM";
    }

    return (
        <React.Fragment>
            {getCrossoverType()}
            <Number json={json} field="freq" label="Freq" min={1} onChange={myOnchange} />
            <Number
                json={json}
                field="order"
                label="Order"
                title={"1order = 6dB/oct\n2order = 12dB/oct\n3order = 18dB/oct"}
                min={1}
                max={8}
                integer={true}
                onChange={myOnchange}
            />
            {getQOffset()}
            {getQ()}
        </React.Fragment>
    );
}

CrossoverFilter.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

CrossoverFilter.init = (json) => {
    json.crossoverType = crossoverTypes[0].value;
    json.order = 0;
    json.freq = 0;
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