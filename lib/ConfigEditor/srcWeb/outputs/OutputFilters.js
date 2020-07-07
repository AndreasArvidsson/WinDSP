import Glyph from "owp.glyphicons";
import React, { useState } from "react";
import Badge from "react-bootstrap/Badge";
import Button from "react-bootstrap/Button";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import PropTypes from "prop-types";
import Filter from "../filters/Filter";
import StringUtil from "../util/StringUtil";
import "./Outputs.css";

const OutputFilters = ({ json, filtersJson, onChange }) => {
    const filterNames = getUnusedFilterNames(filtersJson, json);
    const [selectedFilter, setSelectedFilter] = useState(filterNames[0]);

    const filterToString = (filterJson, i) => {
        let name, error;
        if (hasRef(filterJson)) {
            name = getRefName(filterJson);
            const refNode = filtersJson[name];
            if (!refNode) {
                error = "Can`t find reference: " + filterJson["#ref"];
            }
            filterJson = refNode;
        }
        const title = (name ? `[ ${name} ]\n` : "") + (error ? error : Filter.toString(filterJson));
        return (
            <div key={i}>
                <Glyph
                    type="remove clickable"
                    title={`Remove filter\n\n${title}`}
                    style={{ color: "red" }}
                    onClick={() => removeFilter(i)}
                />
                &nbsp;
                <span title={title}>
                    {!!name &&
                        <React.Fragment>
                            <b>{name}</b> |&nbsp;
                    </React.Fragment>
                    }
                    {error
                        ? <b style={{ color: "orange" }}>{error}</b>
                        : StringUtil.clean(filterJson.type)
                    }
                </span>
            </div>
        );
    }

    const importFilter = () => {
        const filter = {}
        setRefName(filter, selectedFilter);
        json.push(filter);
        onChange();
    }

    const removeFilter = (filterIndex) => {
        json.splice(filterIndex, 1);
        onChange();
    }

    return (
        <React.Fragment>
            <h5>
                Filters
                <small>
                    <Badge variant="primary" style={{ marginLeft: 5 }}>{json.length}</Badge>
                </small>
            </h5>
            {json.map(filterToString)}

            <hr />

            <Row>
                <Col>
                    <select
                        className="custom-select"
                        value={selectedFilter}
                        disabled={!filterNames.length}
                        onChange={e => setSelectedFilter(e.target.value)}
                    >
                        {filterNames.map(name => {
                            return <option key={name} value={name}>{name}</option>;
                        })}
                    </select>
                </Col>
                <Col xs="4">
                    <Button size="sm" onClick={importFilter} disabled={!selectedFilter}>
                        <Glyph type="import" /> Import
                    </Button>
                </Col>
            </Row>

        </React.Fragment>
    );
}

OutputFilters.propTypes = {
    json: PropTypes.array.isRequired,
    filtersJson: PropTypes.object.isRequired,
    onChange: PropTypes.func.isRequired
};

OutputFilters.getUsedFilterNames = (json) => {
    let res = [];
    if (json.outputs) {
        json.outputs.forEach(output => {
            if (output.filters) {
                res = res.concat(getUsedFilterNames(output.filters));
            }
        });
    }
    return res;
}

OutputFilters.renameFilter = (json, oldName, newName) => {
    if (json.outputs) {
        json.outputs.forEach(output => {
            if (output.filters) {
                output.filters.forEach(filter => {
                    if (hasRef(filter)) {
                        const refName = getRefName(filter);
                        if (oldName === refName) {
                            setRefName(filter, newName);
                        }
                    }
                });
            }
        });
    }
}

export default OutputFilters;

function hasRef(json) {
    return !!json["#ref"];
}

function getRefName(json) {
    const ref = json["#ref"];
    return ref.substr(ref.lastIndexOf("/") + 1);
}

function setRefName(filterJson, name) {
    filterJson["#ref"] = "filters/" + name;
}

function getUnusedFilterNames(filtersJson, json) {
    const usedNames = getUsedFilterNames(json);
    const res = [];
    Object.keys(filtersJson).forEach(name => {
        if (!usedNames.includes(name)) {
            res.push(name);
        }
    });
    return res;
}

function getUsedFilterNames(json) {
    const res = [];
    json.forEach(f => {
        if (hasRef(f)) {
            res.push(getRefName(f));
        }
    });
    return res;
}