import React from "react";
import FormCheck from "react-bootstrap/FormCheck";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import { v4 as uuid } from "uuid";
import PropTypes from "prop-types";
import Checkbox from "../inputs/Checkbox";
import Number from "../inputs/Number";
import SelectedChannels from "./SelectedChannels";
import OutputFilters from "./OutputFilters";
import "./Outputs.css";

const Output = ({ json, filtersJson, usedChannels, onChange }) => {

    const renderDelay = () => {
        if (json.delay === undefined) {
            json.delay = {};
        }
        else if (typeof json.delay === "number") {
            json.delay = {
                value: json.delay
            };
        }
        return (
            <React.Fragment>
                <tr><td colSpan="2"><hr /></td></tr>
                {!!json.delay &&
                    <React.Fragment>
                        <Number 
                        json={json.delay} field="value" label="Delay" min={0} onChange={onChange} />
                        <Checkbox json={json.delay} field="unitInMeter" label="Unit meter" disabled={!json.delay.value} onChange={onChange} />
                    </React.Fragment>
                }
            </React.Fragment>
        );
    }

    const renderCancellation = () => {
        const id = uuid();
        return (
            <React.Fragment>
                <tr><td colSpan="2"><hr /></td></tr>
                <tr>
                    <td className="paddingRight">
                        <label htmlFor={id}>
                            Cancellation
                        </label>
                    </td>
                    <td>
                        <FormCheck
                            id={id}
                            type="checkbox"
                            checked={!!json.cancellation}
                            onChange={e => {
                                if (e.target.checked) {
                                    json.cancellation = {
                                        freq: 0,
                                        gain: 0
                                    };
                                }
                                else {
                                    delete json.cancellation;
                                }
                                onChange();
                            }}
                        />
                    </td>
                </tr>
                {!!json.cancellation &&
                    <React.Fragment>
                        <Number json={json.cancellation} field="freq" label="Freq" min={0} onChange={onChange} />
                        <Number json={json.cancellation} field="gain" label="Gain" onChange={onChange} />
                    </React.Fragment>
                }
            </React.Fragment>
        );
    }

    if (!json.filters) {
        json.filters = [];
    }
    return (
        <Row>
            <Col xs="5">
                <table className="output">
                    <tbody>
                        <SelectedChannels json={json.channels} usedChannels={usedChannels} onChange={onChange} />
                        <Checkbox json={json} field="mute" label="Mute" onChange={onChange} />
                        <Checkbox json={json} field="invert" label="Invert" onChange={onChange} />
                        <Number json={json} field="gain" label="Gain" onChange={onChange} />
                        {renderDelay()}
                        {renderCancellation()}
                    </tbody>
                </table>
            </Col>
            <Col>
                <OutputFilters json={json.filters} filtersJson={filtersJson} onChange={onChange} />
            </Col>
        </Row>
    );
}

Output.propTypes = {
    json: PropTypes.object.isRequired,
    filtersJson: PropTypes.object.isRequired,
    usedChannels: PropTypes.array.isRequired,
    onChange: PropTypes.func.isRequired
};

export default Output;