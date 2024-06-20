#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

class websocket_client {
public:
    websocket_client(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx);

    void connect(const std::string& uri);
    void send(const boost::asio::const_buffer& buffer);
    void close();

private:
    boost::asio::io_context& ioc_;
    boost::asio::ssl::context& ctx_;
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_;
};

#endif // WEBSOCKET_CLIENT_H
