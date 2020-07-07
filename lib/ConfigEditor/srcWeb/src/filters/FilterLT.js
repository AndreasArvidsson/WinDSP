import React from "react";
import PropTypes from "prop-types";
import Number from "../inputs/Number";

const FilterLT = ({ json, onChange }) => {
    return (
        <React.Fragment>
            <Number json={json} field="f0" label="f(0)" min={1} onChange={onChange} />
            <Number json={json} field="q0" label="Q(0)" min={0.1} max={10} step="0.001" onChange={onChange} />
            <Number json={json} field="fp" label="f(p)" min={1} onChange={onChange} />
            <Number json={json} field="qp" label="Q(p)" min={0.1} max={10} step="0.001" onChange={onChange} />
        </React.Fragment>
    );
}

FilterLT.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

FilterLT.init = (json) => {
    json.f0 = 0;
    json.q0 = 0;
    json.fp = 0;
    json.qp = 0;
}

FilterLT.toString = (json) => {
    return `f(0): ${json.f0}\n`
        + `Q(0): ${json.q0}\n`
        + `f(p): ${json.fp}\n`
        + `q(p): ${json.qp}`;
}

export default FilterLT;