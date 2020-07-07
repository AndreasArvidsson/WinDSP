import Glyph from "owp.glyphicons";
import React, { useState, useEffect } from "react";
import FormControl from "react-bootstrap/FormControl";
import Button from "react-bootstrap/Button";
import Badge from "react-bootstrap/Badge";
import PropTypes from "prop-types";
import { v4 as uuid } from "uuid";
import Dropdown from "../inputs/Dropdown";
import Panel from "../util/Panel";
import OutputFilters from "../outputs/OutputFilters";
import Filter from "./Filter";

let timeoutId;

const Filters = ({ jsonContext }) => {
    const [filterNames, setFilterNames] = useState([]);
    const [openLast, setOpenLast] = useState(false);

    if (!jsonContext.json.filters) {
        jsonContext.json.filters = {};
    }
    const json = jsonContext.json.filters;

    useEffect(() => {
        setFilterNames(Object.keys(json).sort());
    }, [json]);

    const usedFilterNames = OutputFilters.getUsedFilterNames(jsonContext.json);

    const onChange = () => {
        jsonContext.reRender();
    }

    const addFilter = () => {
        let name;
        let i = Object.keys(json).length;
        do {
            name = "Filter_" + i++;
        } while (json[name]);
        json[name] = {
            type: Filter.types[0].value
        };
        filterNames.push(name);
        setFilterNames([...filterNames]);
        setOpenLast(true);
        onChange();
    }

    const removeFilter = (name) => {
        delete json[name];
        filterNames.splice(filterNames.indexOf(name), 1);
        setFilterNames([...filterNames]);
        onChange();
    }

    const changeType = (name) => {
        json[name] = {
            type: json[name].type
        };
        Filter.init(json[name]);
        onChange();
    }

    const renameFilter = (oldName, newName) => {
        if (newName && !json[newName]) {
            json[newName] = json[oldName];
            delete json[oldName];
            OutputFilters.renameFilter(jsonContext.json, oldName, newName);
            const i = filterNames.indexOf(oldName);
            filterNames[i] = newName;
            setFilterNames([...filterNames]);

            //If true then wait 1sec before sending change up to parent. Resets with every new change.
            clearTimeout(timeoutId);
            timeoutId = setTimeout(() => {
                onChange();
            }, 1000);
        }
    }

    const renderFilter = (name, index) => {
        const isLast = index === filterNames.length - 1;
        const filterJson = json[name];
        const disabled = usedFilterNames.includes(name);
        const id = uuid();
        return (
            <Panel key={index} header={name} defaultOpen={isLast && openLast}>
                <table><tbody>
                    <tr>
                        <td className="paddingRight">
                            <label htmlFor={id}>
                                Name
                            </label>
                        </td>
                        <td>
                            <FormControl
                                id={id}
                                type="text"
                                value={name}
                                onChange={e => renameFilter(name, e.target.value)}
                            />
                        </td>
                    </tr>
                    <Dropdown
                        json={filterJson}
                        field="type"
                        label="Type"
                        values={Filter.types}
                        onChange={() => changeType(name)}
                    />
                    <Filter json={filterJson} onChange={onChange} />
                    <tr>
                        <td></td>
                        <td>
                            <Button
                                size="sm"
                                variant="danger"
                                style={{ marginTop: 5 }}
                                disabled={disabled}
                                title={disabled ? "Can't remove used filter" : null}
                                onClick={() => removeFilter(name)}
                            >
                                <Glyph type="remove" /> Remove filter
                            </Button>
                        </td>
                    </tr>
                </tbody></table>
            </Panel>
        );
    }

    return (
        <div>
            <h5>
                Filters
                <Badge style={{ marginLeft: 5 }} variant="primary">{Object.keys(json).length}</Badge>
            </h5>
            {filterNames.map((name, i) => {
                return renderFilter(name, i);
            })}
            <Button size="sm" style={{ marginTop: 10 }} onClick={addFilter}>
                <Glyph type="plus" /> Add filter
            </Button>
        </div>
    )
}

Filters.propTypes = {
    jsonContext: PropTypes.object.isRequired
};

export default Filters;