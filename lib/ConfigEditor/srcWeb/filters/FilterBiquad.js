import React from "react";
import PropTypes from "prop-types";
import Number from "../inputs/Number";

const FilterBiquad = ({ json, onChange }) => {
    return (
        <React.Fragment>
            <Number json={json} field="b0" label="b0" onChange={onChange} />
            <Number json={json} field="b1" label="b1" onChange={onChange} />
            <Number json={json} field="b2" label="b2" onChange={onChange} />
            <Number json={json} field="a1" label="a1" onChange={onChange} />
            <Number json={json} field="a2" label="a2" onChange={onChange} />
            <Number json={json} field="a3" label="a3" onChange={onChange} />
        </React.Fragment>
    );
}

FilterBiquad.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

FilterBiquad.init = (json) => {
    json.b0 = 0;
    json.b1 = 0;
    json.b2 = 0;
    json.a0 = 1.0;
    json.a1 = 0;
    json.a2 = 0;
}

FilterBiquad.toString = (json) => {
    return `b0: ${json.b0}\n`
        + `b1: ${json.b1}\n`
        + `b2: ${json.b2}\n`
        + `a0: ${json.a0}\n`
        + `a1: ${json.a1}\n`
        + `a2: ${json.a2}`;
}

export default FilterBiquad;