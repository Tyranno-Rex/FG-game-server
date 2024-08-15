//#include <iostream>
//#include <string>
//#include <winsock2.h>
//#include <ws2tcpip.h>
//
//#pragma comment(lib, "ws2_32.lib")
//
//int main() {
//    WSADATA wsaData;
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::cout << "WSAStartup failed." << std::endl;
//        return 1;
//    }
//
//    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
//    if (clientSocket == INVALID_SOCKET) {
//        std::cout << "Socket creation failed." << std::endl;
//        WSACleanup();
//        return 1;
//    }
//
//    sockaddr_in serverAddr;
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_port = htons(8080);
//    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
//
//    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
//        std::cout << "Connection failed." << std::endl;
//        closesocket(clientSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    std::string message;
//    char buffer[1024];
//
//    while (true) {
//        std::cout << "Enter message (or 'quit' to exit): ";
//        std::getline(std::cin, message);
//
//        if (message == "quit") {
//            break;
//        }
//
//        send(clientSocket, message.c_str(), message.length(), 0);
//        std::cout << "Message sent." << std::endl;
//
//        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
//        if (bytesReceived > 0) {
//            buffer[bytesReceived] = '\0';
//            std::cout << "Received: " << buffer << std::endl;
//        }
//    }
//
//    closesocket(clientSocket);
//    WSACleanup();
//
//    return 0;
//}

#include <iostream>
#include <thread>
#include <deque>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class ChatClient {
public:
    ChatClient(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints)
        : socket_(io_context) {
        do_connect(endpoints);
    }

    void write(const std::string& msg) {
        boost::asio::post(socket_.get_executor(),
            [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                write_msgs_.push_back(msg + "\n");
                if (!write_in_progress) {
                    do_write();
                }
            });
    }

    void close() {
        boost::asio::post(socket_.get_executor(), [this]() { socket_.close(); });
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    do_read();
                }
            });
    }

    void do_read() {
        boost::asio::async_read_until(socket_, read_buffer_, "\n",
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::string msg;
                    std::istream is(&read_buffer_);
                    std::getline(is, msg);
                    std::cout << msg << std::endl;
                    do_read();
                }
                else {
                    close();
                }
            });
    }

    void do_write() {
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front()),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        do_write();
                    }
                }
                else {
                    close();
                }
            });
    }

    tcp::socket socket_;
    boost::asio::streambuf read_buffer_;
    std::deque<std::string> write_msgs_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cerr << "Usage: chat_client <host> <port> <room>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(argv[1], argv[2]);

        ChatClient client(io_context, endpoints);

        std::thread t([&io_context]() { io_context.run(); });

        // Send room name
        client.write(argv[3]);

        std::string line;
        while (std::getline(std::cin, line)) {
            client.write(line);
        }

        client.close();
        t.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}