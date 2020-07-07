
import React from "react";
import FormControl from "react-bootstrap/FormControl";
import PropTypes from "prop-types";
import InputBase from "./InputBase";

export default class Text extends InputBase {

    static propTypes = {
        json: PropTypes.object.isRequired,
        field: PropTypes.string.isRequired,
        label: PropTypes.string.isRequired,
        onChange: PropTypes.func.isRequired,
        disabled: PropTypes.bool
    };

    getInput = () => {
        return <FormControl
            id={this.uuid}
            type="text"
            disabled={this.props.disabled}
            value={this.props.json[this.props.field] || ""}
            onChange={e => this.onChange(e.target.value, true)}
        />
    }

}