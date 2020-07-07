
import React from "react";
import PropTypes from "prop-types";
import InputBase from "./InputBase";

export default class Dropdown extends InputBase {

    static propTypes = {
        json: PropTypes.object.isRequired,
        field: PropTypes.string.isRequired,
        label: PropTypes.string.isRequired,
        values: PropTypes.array.isRequired,
        onChange: PropTypes.func,
        disabled: PropTypes.bool
    };

    getInput = () => {
        const value = this.props.json[this.props.field];
        return (
            <select
                id={this.uuid}
                className="custom-select"
                disabled={this.props.disabled}
                value={value}
                onChange={e => this.onChange(e.target.value)}
            >
                {value === undefined && this.getMissingOption()}
                {this.props.values.map(this.getOption)}
            </select>
        );
    }

    getMissingOption() {
        return <option key={undefined} value={undefined}>Select value</option>;
    }

    getOption(v) {
        if (typeof v === "object") {
            return <option key={v.value} value={v.value}>{v.text}</option>;
        }
        return <option key={v} value={v}>{v}</option>;
    }

}