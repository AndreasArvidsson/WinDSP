import React from "react";
import PropTypes from "prop-types";
import Panel from "./util/Panel";
import Checkbox from "./inputs/Checkbox";
import Text from "./inputs/Text";

const General = ({ jsonContext }) => {
    const json = jsonContext.json;

    const onChange = () => {
        jsonContext.reRender();
    }

    return (
        <Panel header="General" defaultOpen={true}>
            <table>
                <tbody>
                    <Checkbox json={json} field="startWithOS" label="Start with OS" onChange={onChange} />
                    <Checkbox json={json} field="minimize" label="Minimize" onChange={onChange} />
                    <Checkbox json={json} field="hide" label="Hide" onChange={onChange} />
                    <Checkbox json={json} field="debug" label="Debug" onChange={onChange} />
                    <Text json={json} field="description" label="Description" onChange={onChange} />
                </tbody>
            </table>
        </Panel>
    )
}

General.propTypes = {
    jsonContext: PropTypes.object.isRequired
};

export default General;