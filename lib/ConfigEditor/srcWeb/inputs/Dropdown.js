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
    disabled: PropTypes.bool,
  };

  getInput = () => {
    return (
      <select
        id={this.uuid}
        className="custom-select"
        disabled={this.props.disabled}
        value={this.state.value}
        onChange={(e) => {
          // `e.target.value` is always a string. Find actual value.
          const value = this.props.values
            .map((v) => parseValue(v).value)
            .find((v) => v == e.target.value);
          this.onChange(value);
        }}
      >
        {this.state.value === undefined && this.getMissingOption()}
        {this.props.values.map(this.getOption)}
      </select>
    );
  };

  getMissingOption() {
    return (
      <option key={undefined} value={undefined}>
        Select value
      </option>
    );
  }

  getOption(v) {
    const { text, value } = parseValue(v);
    return (
      <option key={value} value={value}>
        {text}
      </option>
    );
  }
}

function parseValue(v) {
  return typeof v === "object"
    ? { text: v.text, value: v.value }
    : { text: v, value: v };
}
