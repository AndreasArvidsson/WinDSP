import React from "react";
import PropTypes from "prop-types";
import Text from "../inputs/Text";

const FilterFIR = ({ json, onChange }) => {
    return (
        <React.Fragment>
            <Text json={json} field="file" label="File" onChange={onChange} />
        </React.Fragment>
    );
}

FilterFIR.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

FilterFIR.init = (json) => {
    json.file = "";
}

FilterFIR.toString = (json) => {
    return `File: ${json.file}`;
}

export default FilterFIR;