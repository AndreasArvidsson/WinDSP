import React from "react";
import PropTypes from "prop-types";
import Number from "../inputs/Number";

const FilterNotch = ({ json, onChange }) => {
    return (
        <React.Fragment>
            <Number json={json} field="freq" label="Freq" min={1} onChange={onChange} />
            <Number json={json} field="gain" label="Gain" onChange={onChange} />
            <Number json={json} field="bandwidth" label="Bandwidth" min={0.1} max={10} step="0.001" onChange={onChange} />
        </React.Fragment>
    );
}

FilterNotch.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

FilterNotch.init = (json) => {
    json.freq = 0;
    json.gain = 0;
    json.bandwidth = 0;
}

FilterNotch.toString = (json) => {
    return `Frequency: ${json.freq}Hz\n`
        + `Gain: ${json.gain}dB\n`
        + `Bandwidth: ${json.bandwidth}`;
}

export default FilterNotch;