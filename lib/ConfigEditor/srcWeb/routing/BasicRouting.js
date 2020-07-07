import React from "react";
import PropTypes from "prop-types";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import Dropdown from "../inputs/Dropdown";
import Checkbox from "../inputs/Checkbox";
import Number from "../inputs/Number";
import FilterCrossover from "../filters/FilterCrossover";

const values4 = ["Large", "Small", "Sub", "Off"];
const stereoBassFields = "stereoBass";
const expandSurroundField = "expandSurround";

const BasicRouting = ({ jsonContext }) => {
    if (!jsonContext.json.basic) {
        jsonContext.json.basic = {};
    }
    const json = jsonContext.json.basic;
    const disableStereoBass = getDisableStereoBass(json);
    const disableExpandSurround = getDisableExpandSurround(json);

    const onChange = () => {
        if (!disableStereoBass && getDisableStereoBass(json)) {
            json[stereoBassFields] = false;
        }
        if (!disableExpandSurround && getDisableExpandSurround(json)) {
            json[expandSurroundField] = false;
        }
        jsonContext.reRender();
    }

    const getFiltersPanel = (json) => {
        const showLP = hasType(json, "Sub");
        const showHP = hasType(json, "Small");
        if (!json.lowPass) {
            json.lowPass = {};
        }
        if (!json.highPass) {
            json.highPass = {};
        }
        json.lowPass.crossoverType = json.lowPass.crossoverType || "BUTTERWORTH";
        json.highPass.crossoverType = json.highPass.crossoverType || "BUTTERWORTH";
        json.lowPass.freq = json.lowPass.freq || 80;
        json.highPass.freq = json.highPass.freq || 80;
        json.lowPass.order = json.lowPass.order || 5;
        json.highPass.order = json.highPass.order || 3;
        return (
            <React.Fragment>
                {showLP &&
                    <React.Fragment>
                        <label><b>Lowpass crossover</b></label>
                        <table><tbody>
                            <FilterCrossover json={json.lowPass} onChange={jsonContext.reRender} />
                        </tbody></table>
                    </React.Fragment>
                }
                {(showLP && showHP) && <hr />}
                {showHP &&
                    <React.Fragment>
                        <label><b>Highpass crossover</b></label>
                        <table><tbody>
                            <FilterCrossover json={json.highPass} onChange={jsonContext.reRender} />
                        </tbody></table>
                    </React.Fragment>
                }
            </React.Fragment>
        );
    }

    const getLeftPanel = (json) => {
        return (
            <table>
                <tbody>
                    <Dropdown json={json} field="front" label="Front" values={["Large", "Small"]} onChange={onChange} />
                    <Dropdown json={json} field="center" label="Center" values={values4} onChange={onChange} />
                    <Dropdown json={json} field="subwoofer" label="Subwoofer" values={["Sub", "Off"]} onChange={onChange} />
                    <Dropdown json={json} field="surround" label="Surround" values={values4} onChange={onChange} />
                    <Dropdown json={json} field="surroundBack" label="Surround back" values={values4} onChange={onChange} />
                    <Number json={json} field="lfeGain" label="LFE gain" onChange={onChange} />
                    <Checkbox
                        json={json}
                        field={stereoBassFields}
                        disabled={disableStereoBass}
                        label="Stereo bass"
                        title={disableStereoBass ? "Require at least 2 subwoofers. If more to be divisible by 2. e.g. 2, 4, 6" : null}
                        onChange={onChange}
                    />
                    <Checkbox
                        json={json}
                        field={expandSurroundField}
                        disabled={disableExpandSurround}
                        label="Expand surround"
                        title={disableExpandSurround ? "Requie both surround side and back" : null}
                        onChange={onChange}
                    />
                </tbody>
            </table>
        );
    }

    return (
        <Row>
            <Col xs="5">
                {getLeftPanel(json)}
            </Col>
            <Col style={{ borderLeft: "1px solid rgba(0, 0, 0, 0.1)" }}>
                {getFiltersPanel(json)}
            </Col>
        </Row>
    );
};

BasicRouting.propTypes = {
    jsonContext: PropTypes.object.isRequired
};

export default BasicRouting;

function hasType(json, type) {
    return json.front === type
        || json.center === type
        || json.subwoofer === type
        || json.surround === type
        || json.surroundBack === type;
}

function isSub(type) {
    return type === "Sub";
}

function getDisableExpandSurround(json) {
    return (json.surround !== "Large" && json.surround !== "Small")
        || (json.surroundBack !== "Large" && json.surroundBack !== "Small");
}

function getDisableStereoBass(json) {
    const count = countSubwoofers(json);
    return !count || count % 2 === 1;
}

function countSubwoofers(json) {
    let count = 0;
    count += isSub(json.center) ? 1 : 0;
    count += isSub(json.subwoofer) ? 1 : 0;
    count += isSub(json.surround) ? 2 : 0;
    count += isSub(json.surroundBack) ? 2 : 0;
    return count;
}