import Glyph from "owp.glyphicons";
import React, { useContext } from "react";
import Badge from "react-bootstrap/Badge";
import Button from "react-bootstrap/Button";
import JsonContext from "../util/JsonContext";
import Panel from "../util/Panel";
import Confirm from "../util/Confirm";
import Output from "./Output";

const Outputs = () => {
    const jsonContext = useContext(JsonContext);
    if (!jsonContext.json.outputs) {
        jsonContext.json.outputs = [];
    }
    const json = jsonContext.json.outputs;

    const addOutput = () => {
        const channels = json.length ? [] : ["L", "R"]
        json.push(jsonContext.getDefaultOutput(channels));
        jsonContext.reRender();
    }

    const removeOutput = (index) => {
        json.splice(index, 1);
        jsonContext.reRender();
    }

    const header = (
        <React.Fragment>
            Outputs
            <Badge style={{ marginLeft: 5 }} variant="primary">{json.length}</Badge>
        </React.Fragment>
    );

    const usedChannels = getUsedChannels(json);
    return (
        <Panel header={header} defaultOpen={true}>
            {json.map((output, index) =>
                <React.Fragment key={index}>
                    <table style={{ width: "100%" }}>
                        <tbody>
                            <tr>
                                <td style={{ width: 25 }}>
                                    <Confirm callback={() => removeOutput(index)} text="Remove output">
                                        <Glyph type="remove clickable" style={{ color: "red" }} title={`Remove output\n\n${Output.toString(output)}`} />
                                    </Confirm>
                                </td>
                                <td style={{ paddingLeft: 10 }}>
                                    <Output json={output} usedChannels={usedChannels} />
                                </td>
                            </tr>
                        </tbody>
                    </table>
                    <hr />
                </React.Fragment>
            )}
            <Button size="sm" onClick={addOutput}>
                <Glyph type="plus" /> Add output
            </Button>
        </Panel >
    );
}

export default Outputs;

function getUsedChannels(json) {
    let res = [];
    json.forEach(o => {
        if (o.channels) {
            res = res.concat(o.channels);
        }
    })
    return res;
}