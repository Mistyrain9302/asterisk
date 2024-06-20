#include "websocket_client.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

static boost::asio::io_context ioc;
static boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23_client};
static boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_rx(ioc, ctx);
static boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_tx(ioc, ctx);

extern "C" void websocket_client_connect() {
    try {
        auto const results_rx = boost::asio::ip::tcp::resolver(ioc).resolve("rx_websocket_server", "8765");
        auto const results_tx = boost::asio::ip::tcp::resolver(ioc).resolve("tx_websocket_server", "8765");

        boost::asio::connect(ws_rx.next_layer().next_layer(), results_rx);
        ws_rx.next_layer().handshake(boost::asio::ssl::stream_base::client);
        ws_rx.handshake("rx_websocket_server", "/");

        boost::asio::connect(ws_tx.next_layer().next_layer(), results_tx);
        ws_tx.next_layer().handshake(boost::asio::ssl::stream_base::client);
        ws_tx.handshake("tx_websocket_server", "/");
    } catch (const std::exception& e) {
        // Handle error
    }
}

extern "C" void websocket_client_send(const char* data, size_t len, bool is_rx) {
    try {
        if (is_rx) {
            ws_rx.write(boost::asio::buffer(data, len));
        } else {
            ws_tx.write(boost::asio::buffer(data, len));
        }
    } catch (const std::exception& e) {
        // Handle error
    }
}

extern "C" void websocket_client_close() {
    ws_rx.close(boost::beast::websocket::close_code::normal);
    ws_tx.close(boost::beast::websocket::close_code::normal);
}
