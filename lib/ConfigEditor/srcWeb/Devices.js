import React from "react";
import PropTypes from "prop-types";
import Panel from "./util/Panel";
import Checkbox from "./inputs/Checkbox";
import Number from "./inputs/Number";

const renderAsioField = "renderAsio";
const asioBufferSizeField = "asioBufferSize";
const asioNumChannelsField = "asioNumChannels";

const Devices = ({ jsonContext }) => {
    const json = jsonContext.json.devices;
    const disableAsio = getDisableAsio(json);

    const onChange = () => {
        if (!disableAsio && getDisableAsio(json)) {
            delete json[asioBufferSizeField];
            delete json[asioNumChannelsField];
        }

        jsonContext.reRender();
    }

    const renderAsio = () => {
        if (disableAsio) {
            return null;
        }
        return (
            <React.Fragment>
                <Number json={json} field={asioBufferSizeField} label="ASIO: Buffer size" integer={true} min={0} onChange={onChange} />
                <Number json={json} field={asioNumChannelsField} label="ASIO: Num channels" integer={true} min={0} onChange={onChange} />
            </React.Fragment>
        );
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
                    <Checkbox json={json} field={renderAsioField} label="Render ASIO" onChange={onChange} />
                    {renderAsio()}
                </tbody>
            </table>
        </Panel>
    );
}

Devices.propTypes = {
    jsonContext: PropTypes.object.isRequired
};

export default Devices;

function getDisableAsio(json) {
    return !json.renderAsio;
}