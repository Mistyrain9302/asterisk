// websocket_client.cpp
#include "websocket_client.h"

websocket_client::websocket_client(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx)
    : ioc_(ioc), ctx_(ctx), ws_(ioc, ctx) {}

void websocket_client::connect(const std::string& uri) {
    auto const results = boost::asio::ip::tcp::resolver(ioc_).resolve(uri, "https");
    auto ep = boost::asio::connect(ws_.next_layer().next_layer(), results);
    ws_.next_layer().handshake(boost::asio::ssl::stream_base::client);
    ws_.handshake(uri, "/");
}

void websocket_client::send(const boost::asio::const_buffer& buffer) {
    ws_.write(buffer);
}

void websocket_client::close() {
    ws_.close(boost::beast::websocket::close_code::normal);
}
