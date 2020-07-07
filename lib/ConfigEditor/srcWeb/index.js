import "bootstrap/dist/css/bootstrap.css";
import "./styles.css";
import "./images/favicon.ico";
import React from "react";
import ReactDOM from "react-dom";
import App from "./App";
import { JsonProvider } from "./util/JsonContext";

// Polyfills and shims.
// import "core-js/es/promise";
// import "core-js/es/symbol/iterator";
// import "./Object";
// import "./String";
// import "./Array";

ReactDOM.render(
    <JsonProvider>
        <App />
    </JsonProvider>,
    document.getElementById("root")
);