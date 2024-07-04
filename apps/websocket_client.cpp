#include "websocket_client.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <fstream>

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace websocket = boost::beast::websocket;
namespace beast = boost::beast;
namespace net = boost::asio;

static net::io_context ioc;
static ssl::context ctx{ssl::context::sslv23_client};
static websocket::stream<beast::ssl_stream<tcp::socket>> ws_rx(ioc, ctx);
static websocket::stream<beast::ssl_stream<tcp::socket>> ws_tx(ioc, ctx);

std::string auth_host;
std::string auth_port;
std::string auth_path;
std::string auth_key;
std::string data_format;
int resolution;
int sample_rate;
std::string data_type;
std::string endian;
std::string reg_uri;

void read_config(const std::string& file) {
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(file, pt);

    auth_host = pt.get<std::string>("websocket.auth_host");
    auth_port = pt.get<std::string>("websocket.auth_port");
    auth_path = pt.get<std::string>("websocket.auth_path");
    auth_key = pt.get<std::string>("websocket.auth_key");
    data_format = pt.get<std::string>("websocket.data_format");
    resolution = pt.get<int>("websocket.resolution");
    sample_rate = pt.get<int>("websocket.sample_rate");
    data_type = pt.get<std::string>("websocket.data_type");
    endian = pt.get<std::string>("websocket.endian");
}

void on_read(beast::error_code ec, std::size_t bytes_transferred, beast::flat_buffer& buffer) {
    if (ec) {
        std::cerr << "Read error: " << ec.message() << std::endl;
    } else {
        std::string response = beast::buffers_to_string(buffer.data());
        boost::json::value json_response = boost::json::parse(response);
        if (json_response.as_object().contains("endpoint") && json_response.as_object().at("endpoint").as_bool()) {
            std::cout << "EndPoint Detected: " << response << std::endl;
        } else {
            std::cout << "Result: " << response << std::endl;
        }
        buffer.consume(buffer.size());  // Clear the buffer
    }
}

void do_read_rx(beast::flat_buffer& buffer) {
    ws_rx.async_read(buffer, [&](beast::error_code ec, std::size_t bytes_transferred) {
        on_read(ec, bytes_transferred, buffer);
        do_read_rx(buffer); // Continue reading for more data
    });
}

void do_read_tx(beast::flat_buffer& buffer) {
    ws_tx.async_read(buffer, [&](beast::error_code ec, std::size_t bytes_transferred) {
        on_read(ec, bytes_transferred, buffer);
        do_read_tx(buffer); // Continue reading for more data
    });
}

void register_and_recognize_stt(int file_mode) {
    try {
        read_config("/etc/asterisk/websocket.conf"); // 설정 파일 읽기

        auto const results = tcp::resolver(ioc).resolve(auth_host, auth_port);

        beast::ssl_stream<tcp::socket> ssl_stream(ioc, ctx);
        if(! SSL_set_tlsext_host_name(ssl_stream.native_handle(), auth_host.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        net::connect(ssl_stream.next_layer(), results.begin(), results.end());
        ssl_stream.handshake(ssl::stream_base::client);

        websocket::stream<beast::ssl_stream<tcp::socket>> ws(std::move(ssl_stream));
        ws.handshake(auth_host + ":" + auth_port, auth_path); // 웹소켓 핸드셰이크

        boost::json::object auth_message;
        auth_message["AUTH_KEY"] = auth_key;
        auth_message["DATA_FORMAT"] = data_format;
        auth_message["RESOLUTION"] = resolution;
        auth_message["SAMPLE_RATE"] = sample_rate;
        auth_message["DATA_TYPE"] = data_type;
        auth_message["ENDIAN"] = endian;

        ws.write(net::buffer(boost::json::serialize(auth_message))); // 인증 메시지 전송

        beast::flat_buffer buffer;
        ws.read(buffer); // 응답 읽기

        std::string response = beast::buffers_to_string(buffer.data());
        std::cout << "Result1: " << response << std::endl;

        boost::json::value json_response = boost::json::parse(response);

        if(json_response.as_object().at("status").as_string() != "AUTHENTICATED") {
            std::cerr << "AUTHENTICATION failed" << std::endl;
            return;
        }

        reg_uri = json_response.as_object().at("url").as_string().c_str();
        std::cout << "Receive Token = " << reg_uri << std::endl;

        ws.close(websocket::close_code::normal); // 인증 후 연결 닫기

        std::string full_uri = "wss://" + auth_host + ":" + auth_port + reg_uri;

        boost::asio::connect(ws_rx.next_layer().next_layer(), tcp::resolver(ioc).resolve(auth_host, auth_port));
        ws_rx.next_layer().handshake(ssl::stream_base::client);
        ws_rx.handshake(auth_host, reg_uri); // RX 웹소켓 연결 및 핸드셰이크

        boost::asio::connect(ws_tx.next_layer().next_layer(), tcp::resolver(ioc).resolve(auth_host, auth_port));
        ws_tx.next_layer().handshake(ssl::stream_base::client);
        ws_tx.handshake(auth_host, reg_uri); // TX 웹소켓 연결 및 핸드셰이크

        beast::flat_buffer rx_buffer;
        beast::flat_buffer tx_buffer;

        if (file_mode) {
            std::ifstream file("01_2007_0102214XXXX_2019_05_01_13_08_51_PCM_left.wav", std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot open file");
            }

            std::vector<char> chunk(2048);
            while (file.read(chunk.data(), chunk.size()) || file.gcount() > 0) {
                ws_rx.write(net::buffer(chunk.data(), file.gcount())); // 파일 데이터를 웹소켓으로 전송
                ws_rx.read(rx_buffer); // 응답 읽기
                std::string result = beast::buffers_to_string(rx_buffer.data());
                boost::json::value response = boost::json::parse(result);

                if (response.as_object().at("endpoint").as_bool()) {
                    std::cout << "EndPoint Detected: " << result << std::endl;
                } else {
                    std::cout << "Result: " << result << std::endl;
                }
                rx_buffer.consume(rx_buffer.size());  // 버퍼 비우기
            }
        } else {
            do_read_rx(rx_buffer); // RX 웹소켓 비동기 읽기 시작
            do_read_tx(tx_buffer); // TX 웹소켓 비동기 읽기 시작
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

extern "C" void websocket_client_connect() {
    register_and_recognize_stt(0);
}

extern "C" void websocket_client_send(const char *data, size_t len, int is_rx) {
    try {
        auto buffer = net::buffer(data, len);
        if (is_rx) {
            ws_rx.async_write(buffer, [](beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
            });
        } else {
            ws_tx.async_write(buffer, [](beast::error_code ec, std::size_t) {
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
    ws_rx.async_close(websocket::close_code::normal, [](beast::error_code ec) {
        if (ec) {
            std::cerr << "Close error: " << ec.message() << std::endl;
        }
    });
    ws_tx.async_close(websocket::close_code::normal, [](beast::error_code ec) {
        if (ec) {
            std::cerr << "Close error: " << ec.message() << std::endl;
        }
    });
}