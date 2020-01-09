const path = require("path");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");
const HtmlWebpackPlugin = require("html-webpack-plugin");

module.exports = {
  mode: "none", // this trigger webpack out-of-box prod optimizations
  optimization: {
    minimize: false
  },
  entry: "./index.js",
  output: {
    filename: `[name].js`, // [hash] is useful for cache busting!
    path: path.resolve(__dirname, "dist")
  },
  module: {
    rules: [
      // {
      //   test: /\.css$/,
      //   use: [
      //     {
      //       loader: MiniCssExtractPlugin.loader
      //     },
      //     "css-loader"
      //   ]
      // }
      // {
      //   test: /\.js$/,
      //   use: 'babel-loader',
      //   exclude: /node_modules/
      // }
    ]
  },
  plugins: [
    new HtmlWebpackPlugin(),
    // always deletes the dist folder first in each build run.
    new CleanWebpackPlugin()
    // the plugin to extract our css into separate .css files
    // new MiniCssExtractPlugin({
      // filename: "[name].[contenthash].css"
    // })
  ],
  devtool: "source-map" // supposedly the ideal type without bloating bundle size
};
