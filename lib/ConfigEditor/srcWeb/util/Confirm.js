import React, { useState, useRef } from "react";
import PropTypes from "prop-types";
import Popover from "react-bootstrap/Popover";
import Overlay from "react-bootstrap/Overlay";
import Button from "react-bootstrap/Button";

const confirm = ({ children, text, callback, variant = "danger", placement = "bottom" }) => {
    const [show, setShow] = useState(false);
    const target = useRef(null);
    return (
        <React.Fragment>
            <span style={{ display: "inline-block" }} ref={target} onClick={() => setShow(!show)}>
                {children}
            </span>
            <Overlay
                show={show}
                target={target.current}
                placement={placement}
                rootClose
                onHide={() => setShow(false)}
            >
                <Popover>
                    <Popover.Content>
                        <Button variant={variant} size="sm" onClick={() => {
                            setShow(false);
                            callback();
                        }}>
                            {text}
                        </Button>
                    </Popover.Content>
                </Popover>
            </Overlay>
        </React.Fragment>
    );
};

confirm.propTypes = {
    children: PropTypes.node.isRequired,
    callback: PropTypes.func.isRequired,
    text: PropTypes.string.isRequired,
    variant: PropTypes.string,
    placement: PropTypes.string
};

export default confirm;