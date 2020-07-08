import React, { useContext } from "react";
import JsonContext from "./util/JsonContext";
import Panel from "./util/Panel";
import Checkbox from "./inputs/Checkbox";
import Text from "./inputs/Text";

const General = () => {
    const jsonContext = useContext(JsonContext);
    const json = jsonContext.json;

    const onChange = () => {
        jsonContext.reRender();
    }

    return (
        <Panel header="General" defaultOpen={true}>
            <table>
                <tbody>
                    <Checkbox json={json} field="startWithOS" label="Start with OS" title="Start WinDSP with Windows(Operating System)" onChange={onChange} />
                    <Checkbox json={json} field="minimize" label="Minimize window" title="Minimize application window on startup" onChange={onChange} />
                    <Checkbox json={json} field="hide" label="Hide window" title="Hide application window and move to system tray" onChange={onChange} />
                    <Checkbox json={json} field="debug" label="Debug mode" title="In debug mode additional info is shown in application window" onChange={onChange} />
                    <Text json={json} field="description" label="Description" title="General description of this config file" onChange={onChange} />
                </tbody>
            </table>
        </Panel>
    );
}

export default General;