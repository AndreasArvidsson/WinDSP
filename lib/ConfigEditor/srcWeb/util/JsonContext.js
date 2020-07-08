import HTTP from "owp.http";
import Feedback from "owp.feedback";
import React, { useState, useEffect } from "react";
import PropTypes from "prop-types";
import clonedeep from "lodash.clonedeep";

const Context = React.createContext();
let timeoutId;

const JsonProvider = (props) => {
    const [json, setJson] = useState();
    const [jsonOrg, setJsonOrg] = useState();
    const [currentBasic, setCurrentBasic] = useState(clonedeep(defaultBasic));
    const [currentAdvanced, setCurrentAdvanced] = useState(clonedeep(defaultAdvanced));

    const setNewJson = (newJson) => {
        if (newJson.basic) {
            setCurrentBasic(newJson.basic);
        }
        if (newJson.advanced) {
            setCurrentAdvanced(newJson.advanced);
        }
        setJson(newJson);
    }

    useEffect(() => {
        HTTP.get("rest/json")
            .then(json => {
                setNewJson(clonedeep(json));
                setJsonOrg(json);
            })
            .catch(error => {
                Feedback.notify("Failed to load config file", {
                    type: "error",
                    sticky: true
                });
                console.error(error);
            });
    }, []);

    const save = (newJson) => {
        const data = JSON.stringify(newJson, null, 4);
        HTTP.put("rest/json", { data, contentType: "application/json" })
            .catch(error => {
                Feedback.notify("Failed to save config file", {
                    type: "error",
                    sticky: true
                });
                console.error(error);
            });
    }

    const undo = () => {
        const newJson = clonedeep(jsonOrg);
        setNewJson(newJson);
        save(newJson);
    }

    const useBasic = () => {
        const newJson = { ...json };
        newJson.basic = currentBasic;
        delete newJson.advanced;
        setJson(newJson);
        save(newJson);
    }

    const useAdvanced = () => {
        const newJson = { ...json };
        newJson.advanced = currentAdvanced;
        delete newJson.basic;
        setJson(newJson);
        save(newJson);
    }

    const reRender = () => {
        const newJson = { ...json };
        setJson(newJson);
        clearTimeout(timeoutId);
        timeoutId = setTimeout(() => {
            save(newJson);
        }, 500);
    }

    if (!json) {
        return <div>Loading...</div>
    }

    return (
        <Context.Provider value={{
            json,
            reRender,
            useBasic,
            useAdvanced,
            getDefaultOutput,
            undo
        }}>
            {props.children}
        </Context.Provider>
    );
}

JsonProvider.propTypes = {
    children: PropTypes.node
};

const JsonConsumer = Context.Consumer;

export default Context;
export { JsonProvider, JsonConsumer };


/* **********************************
* ************* PRIVATE *************
*********************************** */

const defaultBasic = {
    front: "Large",
    center: "Large",
    subwoofer: "Sub",
    surround: "Large",
    surroundBack: "Large",
    stereoBass: false,
    expandSurround: false,
    lfeGain: 0,
    lowPass: {
        crossoverType: "BUTTERWORTH",
        freq: 80,
        order: 5,
        qOffset: 0
    },
    highPass: {
        crossoverType: "BUTTERWORTH",
        freq: 80,
        order: 3,
        qOffset: 0
    }
};

const defaultAdvanced = {};

function getDefaultOutput(channels) {
    return {
        channels: channels || [],
        gain: 0,
        mute: false,
        invert: false,
        delay: {
            value: 0,
            unitInMeter: false
        },
        filters: []
    }
}

    //Keep if needed in the future

    // const defaultJson = {
    //     startWithOS: false,
    //     minimize: false,
    //     hide: false,
    //     debug: false,
    //     description: "Default config",
    //     devices: {},
    //     basic: defaultBasic,
    //     outputs: [
    //         getDefaultOutput(["L", "R"])
    //     ]
    // };

    // const resetDefault = () => {
    //     setCurrentBasic(clonedeep(defaultBasic));
    //     setCurrentAdvanced(clonedeep(defaultAdvanced));
    //     const newJson = clonedeep(defaultJson);
    //     if (json.devices) {
    //         newJson.devices = json.devices;
    //     }
    //     save(newJson);
    // }

    // const resetDefaultBasic = () => {
    //     const newBasic = clonedeep(defaultBasic);
    //     setCurrentBasic(newBasic);
    //     save({ ...json, basic: newBasic });
    // }

    // const resetDefaultAdvanced = () => {
    //     const newAdvanced = clonedeep(defaultAdvanced);
    //     setCurrentAdvanced(newAdvanced);
    //     save({ ...json, advanced: newAdvanced });
    // }