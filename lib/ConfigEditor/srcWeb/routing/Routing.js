import React from "react";
import PropTypes from "prop-types";
import Tabs from "react-bootstrap/Tabs";
import Tab from "react-bootstrap/Tab";
import Panel from "../util/Panel";
import BasicRouting from "./BasicRouting";
import AdvancedRouting from "./AdvancedRouting";

const Routing = ({ jsonContext }) => {

    const routingTabChanged = (tab) => {
        if (tab === "basic") {
            jsonContext.useBasic();
        }
        else if (tab === "advanced") {
            jsonContext.useAdvanced();
        }
    }

    const key = jsonContext.json.advanced ? "advanced" : "basic";
    return (
        <Panel header="Routing" defaultOpen={true}>
            <Tabs activeKey={key} onSelect={routingTabChanged}>
                <Tab eventKey="basic" title="Basic" style={{ paddingTop: 10 }}>
                    {key === "basic" ? <BasicRouting jsonContext={jsonContext} /> : null}
                </Tab>
                <Tab eventKey="advanced" title="Advanced">
                    {key === "advanced" ? <AdvancedRouting jsonContext={jsonContext} /> : null}
                </Tab>
            </Tabs>
        </Panel>
    );
}

Routing.propTypes = {
    jsonContext: PropTypes.object.isRequired
};

export default Routing;