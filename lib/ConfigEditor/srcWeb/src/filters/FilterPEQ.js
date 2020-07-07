import React from "react";
import PropTypes from "prop-types";
import Number from "../inputs/Number";

const FilterPEQ = ({ json, onChange }) => {
    return (
        <React.Fragment>
            <Number json={json} field="freq" label="Freq" min={1} onChange={onChange} />
            <Number json={json} field="gain" label="Gain" onChange={onChange} />
            <Number json={json} field="q" label="Q-value" min={0.1} max={10} step="0.001" onChange={onChange} />
        </React.Fragment>
    );
}

FilterPEQ.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

FilterPEQ.init = (json) => {
    json.freq = 0;
    json.gain = 0;
    json.q = 0;
}

FilterPEQ.toString = (json) => {
    return `Frequency: ${json.freq}Hz\n`
        + `Gain: ${json.gain}dB\n`
        + `Q-value: ${json.q}`;
}

export default FilterPEQ;