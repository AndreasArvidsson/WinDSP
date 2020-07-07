import Glyph from "owp.glyphicons";
import React from "react";
import Container from "react-bootstrap/Container";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import Button from "react-bootstrap/Button";
import { JsonConsumer } from "./util/JsonContext";
import Confirm from "./util/Confirm";
import Filters from "./filters/Filters";
import Routing from "./routing/Routing";
import Outputs from "./outputs/Outputs";
import General from "./General";
import Devices from "./Devices";

const App = () => {

    const renderUndo = (context) => {
        return (
            <div style={{ paddingTop: 5 }}>
                <Confirm callback={context.undo} text="Undo all changes">
                    <Button
                        disabled={!context.canUndo()}
                        title="Undo all changes and restore the config as it was when you entered this page"
                        size="sm"
                        variant="danger"
                    >
                        Undo all changes
                    </Button>
                </Confirm>
            </div>
        );
    }

    return (
        <JsonConsumer>
            {context =>
                <Container style={{ marginBottom: 10 }}>
                    <Row>
                        <Col>
                            <h1>WinDSP config editor</h1>
                        </Col>
                        <Col xs="3" >
                            <a href="https://github.com/AndreasArvidsson/WinDSP#windsp" target="_blank" rel="noreferrer">
                                <Glyph type="home" /> Help / documentation
                            </a>
                            {renderUndo(context)}
                        </Col>
                    </Row>
                    <Row>
                        <Col>
                            <General jsonContext={context} />
                            <Devices jsonContext={context} />
                            <Routing jsonContext={context} />
                            <Filters jsonContext={context} />
                            <Outputs jsonContext={context} />
                        </Col>
                    </Row>
                </Container>
            }
        </JsonConsumer >
    );
}

export default App;