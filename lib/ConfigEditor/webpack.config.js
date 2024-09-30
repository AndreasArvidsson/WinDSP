const HtmlWebPackPlugin = require("html-webpack-plugin");
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const OptimizeCSSAssetsPlugin = require("optimize-css-assets-webpack-plugin");
const path = require("path");

function getArg(name) {
    const index = process.argv.indexOf("--" + name);
    if (index < 0) {
        console.error("Missing argument: " + name);
        process.exit(1);
    }
    return process.argv[index + 1];
}

const mode = getArg("mode");

module.exports = {
    entry: "./srcWeb/index.js",
    output: {
        publicPath: "/",
        chunkFilename: "[name].js",
    },
    resolve: {
        modules: ["node_modules", "srcWeb/js"],
    },
    module: {
        rules: [
            {
                test: /\.js$/,
                exclude: /node_modules/,
                use: [
                    {
                        loader: "babel-loader",
                        options: {
                            presets: [
                                "@babel/preset-env",
                                "@babel/preset-react",
                            ],
                            plugins: [
                                "@babel/plugin-proposal-object-rest-spread",
                                "@babel/plugin-proposal-class-properties",
                            ],
                        },
                    },
                    {
                        loader: "eslint-loader",
                        options: {
                            configFile: "eslintrc.js",
                        },
                    },
                ],
            },
            {
                test: /\.html$/,
                loader: "html-loader",
            },
            {
                test: /\.css$/,
                use: [MiniCssExtractPlugin.loader, "css-loader"],
            },
            {
                test: /\.(ico|eot|woff|woff2|ttf|png|jpg|gif|svg)$/,
                loader: "file-loader",
                options: {
                    name: "files/[name].[ext]",
                },
            },
        ],
    },
    plugins: [
        // Use template html file.
        new HtmlWebPackPlugin({
            template: "srcWeb/index.html",
        }),
        // Extract css styles as external file.
        new MiniCssExtractPlugin({
            filename: "styles.css",
        }),
    ],
};

if (mode === "development") {
    module.exports.devServer = {
        publicPath: "/",
        historyApiFallback: {
            index: "/srcWeb/index.html",
        },
        proxy: {
            "/rest": {
                target: "http://localhost:8080",
                changeOrigin: true,
            },
        },
    };
} else if (mode === "production") {
    module.exports.output.path = path.resolve(
        __dirname,
        "target/windsp-config-editor"
    );

    // Minimize css and remove comments
    module.exports.plugins.push(
        new OptimizeCSSAssetsPlugin({
            cssProcessorPluginOptions: {
                preset: ["default", { discardComments: { removeAll: true } }],
            },
        })
    );
} else {
    console.warn("Unknown mode: " + mode);
    process.exit(1);
}
