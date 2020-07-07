import Glyph from "owp.glyphicons";
import React from "react";
import FormControl from "react-bootstrap/FormControl";
import Popover from "react-bootstrap/Popover";
import Button from "react-bootstrap/Button";
import PropTypes from "prop-types";

const AdvancedRoutingPopover = React.forwardRef(({ jsonContext, json, channelIn, channelOut, title, gain, ...props }, ref) => {

    const onChange = (field, value) => {
        if (!json[channelIn.code]) {
            json[channelIn.code] = [];
        }
        const routes = json[channelIn.code];

        let route = routes.find(r => r.out === channelOut.code);
        if (!route) {
            route = {
                out: channelOut.code
            };
            routes.push(route);
        }

        route[field] = value;

        jsonContext.reRender();
    }

    const disableRoute = () => {
        //Just default in to out routing. Set empty array to remove it.
        if (channelIn === channelOut && !json[channelIn.code]) {
            json[channelIn.code] = [];
        }
        //Other routing. Remove route.
        else {
            json[channelIn.code] = json[channelIn.code].filter(r => r.out !== channelOut.code);
        }
        jsonContext.reRender();
    }

    const enableRoute = () => {
        //Same in as out and there is an empty array. Just remove it to fall back to default in to out routing.
        if (channelIn === channelOut && json[channelIn.code].length === 0) {
            delete json[channelIn.code];
        }
        //Other routing. Add the route.
        else {
            if (!json[channelIn.code]) {
                json[channelIn.code] = [];
            }
            json[channelIn.code].push({
                out: channelOut.code
            });
        }
        jsonContext.reRender();
    }

    return (
        <Popover id="popover-basic" ref={ref} {...props}>
            <Popover.Title as="h4" title={title}>
                {channelIn.code}
            &nbsp;
            <Glyph type="arrow-right" />
            &nbsp;
            {channelOut.code}
            </Popover.Title>
            <Popover.Content>
                {gain !== null
                    ? <React.Fragment>
                        <FormControl
                            type="number"
                            value={gain}
                            onChange={e => onChange("gain", parseFloat(e.target.value))}
                            step="0.1"
                        />
                        <Button variant="danger" size="sm" style={{ marginTop: 5 }} onClick={disableRoute}>
                            Disable
                        </Button>
                    </React.Fragment>
                    : <Button variant="primary" size="sm" onClick={enableRoute}>
                        Enable
                    </Button>
                }
            </Popover.Content>
        </Popover>
    );
});

AdvancedRoutingPopover.propTypes = {
    jsonContext: PropTypes.object.isRequired,
    json: PropTypes.object.isRequired,
    channelIn: PropTypes.object.isRequired,
    channelOut: PropTypes.object.isRequired,
    title: PropTypes.string.isRequired,
    gain: PropTypes.number
};

AdvancedRoutingPopover.displayName = "AdvancedRoutingPopover";
export default AdvancedRoutingPopover;