import React, { useContext } from "react";
import JsonContext from "./util/JsonContext";
import Panel from "./util/Panel";
import Number from "./inputs/Number";

const asioBufferSizeField = "asioBufferSize";
const asioNumChannelsField = "asioNumChannels";

const Devices = () => {
    const jsonContext = useContext(JsonContext);
    if (!jsonContext.json.devices) {
        jsonContext.json.devices = {};
    }
    const json = jsonContext.json.devices;
    const disableAsio = getDisableAsio(json);

    const onChange = (field, value) => {
        if (field === "renderAsio") {
            json.renderAsio = value === "true";
        }
        if (!disableAsio && getDisableAsio(json)) {
            delete json[asioBufferSizeField];
            delete json[asioNumChannelsField];
        }
        jsonContext.reRender();
    }

    return (
        <Panel header="Devices" defaultOpen={true}>
            <table>
                <tbody>
                    <tr>
                        <td className="paddingRight">Capture device</td>
                        <td>{json.capture}</td>
                    </tr>
                    <tr>
                        <td className="paddingRight">Render device</td>
                        <td>{json.render}</td>
                    </tr>
                    <tr title="Does the render device use WASAPI(Windows Audio Session API) or ASIO(Audio Stream Input/Output)">
                        <td className="paddingRight">Render protocol</td>
                        <td>{disableAsio ? "WASAPI" : "ASIO"}</td>
                    </tr>
                    {!disableAsio &&
                        <React.Fragment>
                            <Number
                                json={json}
                                field={asioBufferSizeField}
                                label="ASIO: Buffer size"
                                title="Number of samples in ASIO buffer"
                                integer={true}
                                min={0}
                                onChange={onChange}
                            />
                            <Number
                                json={json}
                                field={asioNumChannelsField}
                                label="ASIO: Num channels"
                                title="Number of output channels"
                                integer={true}
                                min={0}
                                onChange={onChange}
                            />
                        </React.Fragment>
                    }
                </tbody>
            </table>
        </Panel>
    );
}

export default Devices;

function getDisableAsio(json) {
    return !json.renderAsio;
}