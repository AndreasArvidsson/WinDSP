import React from "react";
import OverlayTrigger from "react-bootstrap/OverlayTrigger";
import PropTypes from "prop-types";
import Channels from "../util/Channels";
import AdvancedRoutingPopover from "./AdvancedRoutingPopover";
import "./AdvancedRouting.css";

const AdvancedRouting = ({ jsonContext }) => {
    if (!jsonContext.json.advanced) {
        jsonContext.json.advanced = {};
    }
    const json = jsonContext.json.advanced;

    const renderHeader = () => {
        return (
            <tr>
                <th />
                {Channels.getList().map(c =>
                    <th key={c.code} title={c.name + " out"}>
                        {c.code} out
                    </th>
                )}
            </tr>
        );
    }

    const renderRoute = (channelIn, channelOut, routes) => {
        let gain = null;

        if (routes) {
            const route = routes.find(r => r.out === channelOut.code);
            if (route) {
                gain = route.gain || 0;
            }
        }
        else if (channelIn === channelOut) {
            gain = 0;
        }

        const title = `Route '${channelIn.name}' to '${channelOut.name}'`;

        const popover = <AdvancedRoutingPopover
            jsonContext={jsonContext}
            json={json}
            channelIn={channelIn}
            channelOut={channelOut}
            gain={gain}
            title={title}
        />;

        return (
            <OverlayTrigger key={channelOut.code} trigger="click" placement="top" overlay={popover} rootClose={true}>
                <th className={gain !== null ? "routeOn" : "routeOff"} title={title}>
                    {gain !== null ? gain + "dB" : "Off"}
                </th>
            </OverlayTrigger>
        );
    }

    const renderRoutes = (channelIn) => {
        const routes = json[channelIn.code]
        return (
            <tr key={channelIn.code}>
                <th title={channelIn.name + " in"}>
                    {channelIn.code} in
                </th>
                {Channels.getList().map(channelOut =>
                    renderRoute(channelIn, channelOut, routes)
                )}
            </tr>
        )
    }

    return (
        <table className="advancedRouting">
            <tbody>
                {renderHeader()}
                {Channels.getList().map(renderRoutes)}
            </tbody>
        </table>
    );
};

AdvancedRouting.propTypes = {
    jsonContext: PropTypes.object.isRequired
};

export default AdvancedRouting;