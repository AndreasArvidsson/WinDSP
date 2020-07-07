import Glyph from "owp.glyphicons";
import React, { useState } from "react";
import Card from "react-bootstrap/Card";
import PropTypes from "prop-types";

const Panel = ({ header, children, defaultOpen = false, ...rest }) => {
    const [isOpen, setIsOpen] = useState(defaultOpen);
    return (
        <Card {...rest} style={{ marginTop: 10 }}>
            <Card.Header onClick={() => setIsOpen(!isOpen)} className="clickable">
                <Glyph type={"chevron-" + (isOpen ? "up" : "down")} />
                &nbsp;
                {header}
            </Card.Header>
            {isOpen &&
                <Card.Body>
                    {children}
                </Card.Body>
            }
        </Card>
    )
}

Panel.propTypes = {
    header: PropTypes.node.isRequired,
    children: PropTypes.node.isRequired,
    defaultOpen: PropTypes.bool
};

export default Panel;