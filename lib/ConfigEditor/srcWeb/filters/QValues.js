import React, { useEffect } from "react";
import FormControl from "react-bootstrap/FormControl";
import PropTypes from "prop-types";

const QValues = ({ json, onChange }) => {
    if (!json.q) {
        json.q = [];
    }

    useEffect(() => {
        const count = Math.floor(json.order / 2);
        if (json.q.length > count) {
            json.q.length = count;
        }
        while (json.q.length < count) {
            json.q.push(0.707);
        }
        onChange();
    }, [json.order]);

    const myOnChange = (index, e) => {
        json.q[index] = parseFloat(e.target.value);
        onChange();
    }

    return (
        <tr>
            <td style={{ paddingRight: 10 }}>
                <label>Q-values</label>
            </td>
            <td>
                {json.q.map((q, index) =>
                    <FormControl
                        key={index}
                        type="number"
                        step="0.001"
                        style={{ display: "inline-block", width: 85 }}
                        value={q}
                        min={0.1}
                        max={2}
                        onChange={e => myOnChange(index, e)}
                    />
                )}
            </td>
        </tr>
    );
}

QValues.propTypes = {
    json: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

export default QValues;