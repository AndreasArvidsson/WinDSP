import HTTP from "owp.http";
import Feedback from "owp.feedback";
import React, { useState, useEffect } from "react";
import PropTypes from "prop-types";
import clonedeep from "lodash.clonedeep";

const Context = React.createContext();

const JsonProvider = (props) => {
    const [json, setJson] = useState();
    const [currentBasic, setCurrentBasic] = useState(clonedeep(defaultBasic));
    const [currentAdvanced, setCurrentAdvanced] = useState(clonedeep(defaultAdvanced));

    useEffect(() => {
        HTTP.get("rest/json")
            .then(json => {
                if (json.basic) {
                    setCurrentBasic(json.basic);
                }
                if (json.advanced) {
                    setCurrentAdvanced(json.advanced);
                }
                setJson(json);
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
            .then(() => {
                setJson(newJson);
            })
            .catch(error => {
                Feedback.notify("Failed to save config file", {
                    type: "error",
                    sticky: true
                });
                console.error(error);
            });
    }

    const resetDefault = () => {
        setCurrentBasic(clonedeep(defaultBasic));
        setCurrentAdvanced(clonedeep(defaultAdvanced));
        const newJson = clonedeep(defaultJson);
        if (json.devices) {
            newJson.devices = json.devices;
        }
        save(newJson);
    }

    const useBasic = () => {
        const newJson = { ...json };
        newJson.basic = currentBasic;
        delete newJson.advanced;
        save(newJson);
    }

    const useAdvanced = () => {
        const newJson = { ...json };
        newJson.advanced = currentAdvanced;
        delete newJson.basic;
        save(newJson);
    }

    const reRender = () => {
        save({ ...json });
    }

    if (!json) {
        return <div>Loading...</div>
    }

    return (
        <Context.Provider value={{
            json,
            reRender,
            resetDefault,
            useBasic,
            useAdvanced,
            getDefaultOutput
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
    "front": "Large",
    "center": "Large",
    "subwoofer": "Sub",
    "surround": "Large",
    "surroundBack": "Large",
    "stereoBass": false,
    "expandSurround": false,
    "lfeGain": 0,
    "lowPass": {
        "crossoverType": "BUTTERWORTH",
        "order": 5,
        "freq": 80
    },
    "highPass": {
        "crossoverType": "BUTTERWORTH",
        "order": 3,
        "freq": 80
    }
};

const defaultAdvanced = {};

const defaultJson = {
    "startWithOS": false,
    "minimize": false,
    "hide": false,
    "description": "Default config",
    "devices": {},
    "basic": defaultBasic,
    "outputs": [
        getDefaultOutput(["L", "R"])
    ]
};

function getDefaultOutput(channels) {
    return {
        "channels": channels || [],
        "gain": 0,
        "delay": 0,
        "invert": false,
        "mute": false,
        "filters": []
    }
}

    //Keep if needed in the future

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