import React from "react";
import PropTypes from "prop-types";
import Channels from "../util/Channels";

const SelectedChannels = ({ json, usedChannels, onChange }) => {

    const onClick = (code) => {
        const i = json.indexOf(code);
        if (i > -1) {
            json.splice(i, 1);
        }
        else if (usedChannels.includes(code)) {
            return;
        }
        else {
            json.push(code);
            Channels.sort(json);
        }
        onChange();
    }

    const getClassName = (code) => {
        if (json.includes(code)) {
            return "channel-selected";
        }
        if (usedChannels.includes(code)) {
            return "channel-disabled";
        }
        return "channel-available";
    }

    const getTitle = (channel) => {
        if (!json.includes(channel.code) && usedChannels.includes(channel.code)) {
            return `${channel.name}\nChannel is already used by another output`;
        }
        return channel.name;
    }

    return (
        <tr>
            <td style={{ paddingRight: 10 }}>
                <label
                    style={{ color: !json.length ? "orange" : null }}
                    title={!json.length ? "No selected channels" : null}
                >
                    Channels
                </label>
            </td>
            <td>
                {Channels.getList().map(channel =>
                    <label
                        key={channel.code}
                        className={getClassName(channel.code)}
                        onClick={() => onClick(channel.code)}
                        title={getTitle(channel)}
                    >
                        {channel.code}
                        &nbsp;
                    </label>
                )}
            </td>
        </tr>
    );
}

SelectedChannels.propTypes = {
    json: PropTypes.array.isRequired,
    usedChannels: PropTypes.array.isRequired,
    onChange: PropTypes.func.isRequired
};

export default SelectedChannels;