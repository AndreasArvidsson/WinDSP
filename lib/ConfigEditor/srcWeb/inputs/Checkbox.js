
import React from "react";
import FormCheck from "react-bootstrap/FormCheck";
import PropTypes from "prop-types";
import InputBase from "./InputBase";

export default class Checkbox extends InputBase {

    static propTypes = {
        json: PropTypes.object.isRequired,
        field: PropTypes.string.isRequired,
        label: PropTypes.string.isRequired,
        onChange: PropTypes.func.isRequired,
        disabled: PropTypes.bool
    };

    getInput = () => {
        return <FormCheck
            id={this.uuid}
            type="checkbox"
            disabled={this.props.disabled}
            checked={this.props.json[this.props.field] === true}
            onChange={e => this.onChange(e.target.checked)}
        />
    }

}