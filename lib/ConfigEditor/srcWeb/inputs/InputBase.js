
import React from "react";
import PropTypes from "prop-types";
import { v4 as uuid } from "uuid";

export default class InputBase extends React.Component {

    static propTypes = {
        json: PropTypes.object.isRequired,
        field: PropTypes.string.isRequired,
        label: PropTypes.string.isRequired,
        onChange: PropTypes.func.isRequired,
        disabled: PropTypes.bool,
        title: PropTypes.state
    };

    static getDerivedStateFromProps(nextProps, prevState) {
        const newValue = nextProps.json[nextProps.field];
        if (nextProps.json !== prevState.json || (!prevState.ignoreNewValue && newValue !== prevState.value)) {
            return {
                json: nextProps.json,
                value: nextProps.json[nextProps.field]
            };
        }
        return null;
    }

    constructor(props) {
        super(props);
        this.uuid = uuid();
        this.state = {};
    }

    onChange = (value) => {
        const json = this.state.json;
        json[this.props.field] = value;
        this.props.onChange(this.props.field, value);
    }

    render = () => {
        return (
            <tr title={this.props.title}>
                <td className="paddingRight">
                    <label
                        disabled={this.props.disabled}
                        htmlFor={this.uuid}>
                        {this.props.label}
                    </label>
                </td>
                <td>
                    {this.getInput()}
                </td>
            </tr>
        )
    }

}