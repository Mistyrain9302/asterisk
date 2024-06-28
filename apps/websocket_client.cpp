#include "websocket_client.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>

static boost::asio::io_context ioc;
static boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23_client};
static boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_rx(ioc, ctx);
static boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_tx(ioc, ctx);

std::string rx_server_url;
std::string tx_server_url;

void read_config(const std::string& file) {
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(file, pt);

    rx_server_url = pt.get<std::string>("websocket.rx_server");
    tx_server_url = pt.get<std::string>("websocket.tx_server");
}

void parse_url(const std::string& url, std::string& host, std::string& port, std::string& path) {
    auto protocol_end = url.find("://");
    auto host_start = protocol_end + 3;
    auto port_start = url.find(":", host_start);
    auto path_start = url.find("/", port_start);

    host = url.substr(host_start, port_start - host_start);
    port = url.substr(port_start + 1, path_start - port_start - 1);
    path = (path_start == std::string::npos) ? "/" : url.substr(path_start); // 리소스 경로가 없는 경우 기본값을 "/"로 설정
}

void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

void do_read_rx() {
    ws_rx.async_read(
        boost::beast::flat_buffer(),
        [](boost::beast::error_code ec, std::size_t bytes_transferred) {
            on_read(ec, bytes_transferred);
        });
}

void do_read_tx() {
    ws_tx.async_read(
        boost::beast::flat_buffer(),
        [](boost::beast::error_code ec, std::size_t bytes_transferred) {
            on_read(ec, bytes_transferred);
        });
}

void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        std::cerr << "Read error: " << ec.message() << std::endl;
    } else {
        std::cout << "Received " << bytes_transferred << " bytes" << std::endl;
        // 수신된 데이터를 처리하는 코드를 여기에 추가하세요.
    }
}

extern "C" void websocket_client_connect() {
    try {
        read_config("/etc/asterisk/websocket.conf");

        std::string rx_host, rx_port, rx_path;
        std::string tx_host, tx_port, tx_path;

        parse_url(rx_server_url, rx_host, rx_port, rx_path);
        parse_url(tx_server_url, tx_host, tx_port, tx_path);

        auto const results_rx = boost::asio::ip::tcp::resolver(ioc).resolve(rx_host, rx_port);
        auto const results_tx = boost::asio::ip::tcp::resolver(ioc).resolve(tx_host, tx_port);

        boost::asio::async_connect(ws_rx.next_layer().next_layer(), results_rx,
            [&](boost::beast::error_code ec, const boost::asio::ip::tcp::resolver::results_type::endpoint_type&) {
                if (!ec) {
                    ws_rx.next_layer().async_handshake(boost::asio::ssl::stream_base::client, 
                        [&](boost::beast::error_code ec) {
                            if (!ec) {
                                ws_rx.async_handshake(rx_host, rx_path, 
                                    [&](boost::beast::error_code ec) {
                                        if (!ec) {
                                            do_read_rx(); // 수신 시작
                                        }
                                    });
                            }
                        });
                }
            });

        boost::asio::async_connect(ws_tx.next_layer().next_layer(), results_tx,
            [&](boost::beast::error_code ec, const boost::asio::ip::tcp::resolver::results_type::endpoint_type&) {
                if (!ec) {
                    ws_tx.next_layer().async_handshake(boost::asio::ssl::stream_base::client, 
                        [&](boost::beast::error_code ec) {
                            if (!ec) {
                                ws_tx.async_handshake(tx_host, tx_path, 
                                    [&](boost::beast::error_code ec) {
                                        if (!ec) {
                                            do_read_tx(); // 수신 시작
                                        }
                                    });
                            }
                        });
                }
            });

        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

extern "C" void websocket_client_send(const char *data, size_t len, int is_rx) {
    try {
        auto buffer = boost::asio::buffer(data, len);
        if (is_rx) {
            ws_rx.async_write(buffer, [](boost::beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
            });
        } else {
            ws_tx.async_write(buffer, [](boost::beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
            });
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

extern "C" void websocket_client_close() {
    ws_rx.async_close(boost::beast::websocket::close_code::normal, [](boost::beast::error_code ec) {
        if (ec) {
            std::cerr << "Close error: " << ec.message() << std::endl;
        }
    });
    ws_tx.async_close(boost::beast::websocket::close_code::normal, [](boost::beast::error_code ec) {
        if (ec) {
            std::cerr << "Close error: " << ec.message() << std::endl;
        }
    });
}
