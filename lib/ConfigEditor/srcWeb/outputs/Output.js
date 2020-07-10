import React, { useContext } from "react";
import PropTypes from "prop-types";
import FormCheck from "react-bootstrap/FormCheck";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import { v4 as uuid } from "uuid";
import JsonContext from "../util/JsonContext";
import Checkbox from "../inputs/Checkbox";
import Number from "../inputs/Number";
import SelectedChannels from "./SelectedChannels";
import OutputFilters from "./OutputFilters";
import "./Outputs.css";

const Output = ({ json, usedChannels }) => {
    const jsonContext = useContext(JsonContext);
    if (json.channel) {
        json.channels = [json.channel];
        delete json.channel;
    }
    if (!json.channels) {
        json.channels = [];
    }
    if (!json.filters) {
        json.filters = [];
    }

    const onChange = () => {
        jsonContext.reRender();
    }

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
                <tr title="Create a delayed and phase inverted signal that cancels out acoustically on the specified frequency">
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

    const renderCompression = () => {
        const id = uuid();
        return (
            <React.Fragment>
                <tr><td colSpan="2"><hr /></td></tr>
                <tr title="Dynamic range compression">
                    <td className="paddingRight">
                        <label htmlFor={id}>
                            Compression
                        </label>
                    </td>
                    <td>
                        <FormCheck
                            id={id}
                            type="checkbox"
                            checked={!!json.compression}
                            onChange={e => {
                                if (e.target.checked) {
                                    json.compression = {
                                        threshold: 0,
                                        ratio: 0.5,
                                        attack: 1,
                                        release: 100,
                                        window: 1
                                    };
                                }
                                else {
                                    delete json.compression;
                                }
                                onChange();
                            }}
                        />
                    </td>
                </tr>
                {!!json.compression &&
                    <React.Fragment>
                        <Number json={json.compression} field="threshold" label="Threshold" title="dB" onChange={onChange} />
                        <Number json={json.compression} field="ratio" label="Ratio" min={0} max={1} title={"[0.0, 1.0]\n0.0 = oo:1\n0.5 = 2:1\n1.0 = 1:1"} onChange={onChange} />
                        <Number json={json.compression} field="attack" label="Attack" min={1} title="ms" onChange={onChange} />
                        <Number json={json.compression} field="release" label="Release" min={1} title="ms" onChange={onChange} />
                        <Number json={json.compression} field="window" label="Window" min={1} title="ms" onChange={onChange} />
                    </React.Fragment>
                }
            </React.Fragment>
        );
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
                        {renderCompression()}
                        {renderCancellation()}
                    </tbody>
                </table>
            </Col>
            <Col>
                <OutputFilters json={json.filters} />
            </Col>
        </Row>
    );
}

Output.propTypes = {
    json: PropTypes.object.isRequired,
    usedChannels: PropTypes.array.isRequired
};

Output.toString = (json) => {
    if (json.channels.length) {
        return json.channels.join(", ");
    }
    return "No channels selected";
};

export default Output;