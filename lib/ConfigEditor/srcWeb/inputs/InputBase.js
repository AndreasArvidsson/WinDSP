
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
        if (nextProps.json !== prevState.json) {
            return { json: nextProps.json };
        }
        return null;
    }

    constructor(props) {
        super(props);
        this.uuid = uuid();
        this.state = {
            json: this.props.json
        };
    }

    onChange = (value, wait) => {
        const json = this.state.json;
        json[this.props.field] = value;
        //If true then wait 1sec before sending change up to parent. Resets with every new change.
        if (wait) {
            this.setState({
                json: this.state.json
            });
            clearTimeout(this.timeoutId);
            this.timeoutId = setTimeout(() => {
                this.props.onChange(this.props.field, value);
            }, 1000);
        }
        else {
            this.props.onChange(this.props.field, value);
        }
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