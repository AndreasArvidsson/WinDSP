
import React from "react";
import FormControl from "react-bootstrap/FormControl";
import PropTypes from "prop-types";
import InputBase from "./InputBase";

export default class Number extends InputBase {

    static propTypes = {
        json: PropTypes.object.isRequired,
        field: PropTypes.string.isRequired,
        label: PropTypes.string.isRequired,
        onChange: PropTypes.func.isRequired,
        integer: PropTypes.bool,
        disabled: PropTypes.bool,
        min: PropTypes.number,
        max: PropTypes.number,
        step: PropTypes.string
    };

    constructor(props) {
        super(props);
        if (props.step) {
            this.step = props.step;
        }
        else {
            this.step = this.props.integer ? "1" : "0.1";
        }
    }

    getInput = () => {
        return <FormControl
            id={this.uuid}
            type="number"
            disabled={this.props.disabled}
            value={this.props.json[this.props.field] || 0}
            onChange={e => this.onChange(
                this.props.integer
                    ? parseInt(e.target.value)
                    : parseFloat(e.target.value)
            )}
            step={this.step}
            min={this.props.min}
            max={this.props.max}
        />
    }

}