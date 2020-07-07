import Glyph from "owp.glyphicons";
import React from "react";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import Container from "react-bootstrap/Container";
import { JsonConsumer } from "./util/JsonContext";
import Filters from "./filters/Filters";
import Routing from "./routing/Routing";
import Outputs from "./outputs/Outputs";
import General from "./General";
import Devices from "./Devices";

const App = () => {
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
        </JsonConsumer>
    );
}

export default App;