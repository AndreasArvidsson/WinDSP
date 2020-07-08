
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

    static getDerivedStateFromProps(nextProps, prevState) {
        const state = super.getDerivedStateFromProps(nextProps, prevState);
        if (state && !state.value) {
            state.value = 0;
        }
        return state;
    }

    myOnChange = (e) => {
        let value = e.target.value.trim();
        if (value === "") {
            this.setState({ value, ignoreNewValue: true });
        }
        else if (value === "-") {
            this.setState({ value, ignoreNewValue: true });
        }
        else {
            if (this.props.integer) {
                value = parseInt(value);
            }
            else {
                value = parseFloat(value);
            }
            if (!isNaN(value)) {
                this.setState({ ignoreNewValue: false });
                this.onChange(value);
            }
        }
    }

    getInput = () => {
        return <FormControl
            id={this.uuid}
            type="number"
            disabled={this.props.disabled}
            value={this.state.value}
            onChange={this.myOnChange}
            onBlur={e => {
                if (!e.target.value) {
                    this.setState({ value: 0 });
                }
            }}
            step={this.step}
            min={this.props.min}
            max={this.props.max}
        />
    }

}