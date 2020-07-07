import Glyph from "owp.glyphicons";
import React, { useState, useEffect } from "react";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import Card from "react-bootstrap/Card";
import FormControl from "react-bootstrap/FormControl";
import Button from "react-bootstrap/Button";
import Badge from "react-bootstrap/Badge";
import PropTypes from "prop-types";
import { v4 as uuid } from "uuid";
import Dropdown from "../inputs/Dropdown";
import Panel from "../util/Panel";
import Confirm from "../util/Confirm";
import OutputFilters from "../outputs/OutputFilters";
import Filter from "./Filter";

let timeoutId;

const Filters = ({ jsonContext }) => {
    const [filterNames, setFilterNames] = useState([]);

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
        let i = Object.keys(json).length + 1;
        do {
            name = "Filter_" + i++;
        } while (json[name]);
        json[name] = {
            type: Filter.types[0].value
        };
        Filter.init(json[name]);
        filterNames.push(name);
        setFilterNames([...filterNames]);
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
        const filterJson = json[name];
        if (!filterJson) {
            return null;
        }
        const disabled = usedFilterNames.includes(name);
        const id = uuid();
        return (
            <Col key={index} xs="4">
                <Card style={{ margin: 5 }}>
                    <Card.Body>
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
                                    <Confirm callback={() => removeFilter(name)} text="Remove filter">
                                        <Button
                                            size="sm"
                                            variant="danger"
                                            style={{ marginTop: 5 }}
                                            disabled={disabled}
                                            title={disabled ? "Can't remove used filter" : null}
                                        >
                                            <Glyph type="remove" /> Remove filter
                                        </Button>
                                    </Confirm>
                                </td>
                            </tr>
                        </tbody></table>
                    </Card.Body>
                </Card>
            </Col>
        );
    }

    const header = (
        <React.Fragment>
            Filters
            <Badge style={{ marginLeft: 5 }} variant="primary">{Object.keys(json).length}</Badge>
        </React.Fragment>
    );
    return (
        <Panel header={header} defaultOpen={true}>
            <Row noGutters="true">
                {filterNames.map((name, i) => {
                    return renderFilter(name, i);
                })}
            </Row>
            <Button size="sm" style={{ marginTop: 10 }} onClick={addFilter}>
                <Glyph type="plus" /> Add filter
            </Button>
        </Panel>
    )
}

Filters.propTypes = {
    jsonContext: PropTypes.object.isRequired
};

export default Filters;