//#include <iostream>
//#include <winsock2.h>
//
//
//// ws2_32.lib is required for WinSock2
//#pragma comment(lib, "ws2_32.lib")
//
//int main() {
//	// WSADATA is a structure that contains information about the Windows Sockets implementation.
//    WSADATA wsaData;
//	// WSAStartup initializes the Windows Sockets library. 
//	// (MAKEWORD(2, 2) is version 2.2), wsaData is a pointer to the WSADATA structure.
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::cout << "WSAStartup failed." << std::endl;
//        return 1;
//    }
//
//	// Create a socket with the specified address family, socket type, and protocol.
//	// AF_INET is the address family for IPv4, SOCK_STREAM is the socket type for TCP, and 0 is the protocol.
//    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//    if (serverSocket == INVALID_SOCKET) {
//        std::cout << "Socket creation failed." << std::endl;
//        WSACleanup();
//        return 1;
//    }
//
//    sockaddr_in serverAddr;
//	serverAddr.sin_family = AF_INET; // IPv4
//	serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address.
//	serverAddr.sin_port = htons(8080); // Port 8080
//
//	// Bind the socket to the specified address.
//	// *Bind is used to associate a local address with a socket.
//    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
//        std::cout << "Bind failed." << std::endl;
//        closesocket(serverSocket);
//        WSACleanup();
//        return 1;
//    }
//
//	// Listen for incoming connections on the socket.
//    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
//        std::cout << "Listen failed." << std::endl;
//        closesocket(serverSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    std::cout << "Server listening on port 8080..." << std::endl;
//
//	// Accept a connection on the socket.
//    SOCKET clientSocket = accept(serverSocket, NULL, NULL);
//    if (clientSocket == INVALID_SOCKET) {
//        std::cout << "Accept failed." << std::endl;
//        closesocket(serverSocket);
//        WSACleanup();
//        return 1;
//    }
//
//	// Receive data from the client.
//    char buffer[1024];
//
//	while (true) {
//		// Receive data from the client
//        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
//		// If bytesReceived is greater than 0, the client has sent data. => Echo the message back to the client.
//        if (bytesReceived > 0) {
//            buffer[bytesReceived] = '\0';
//            std::cout << "Received: " << buffer << std::endl;
//
//            // Echo the message back to the client
//            send(clientSocket, buffer, bytesReceived, 0);
//        }
//        else if (bytesReceived == 0) {
//            std::cout << "Client disconnected." << std::endl;
//            break;
//        }
//        else {
//            std::cout << "recv failed." << std::endl;
//            break;
//        }
//	}
//
//    closesocket(clientSocket);
//    closesocket(serverSocket);
//    WSACleanup();
//
//    return 0;
//}


//#include <iostream>
//#include <memory>
//#include <string>
//#include <boost/asio.hpp>
//
//using boost::asio::ip::tcp;
//
//class Session : public std::enable_shared_from_this<Session> {
//public:
//    Session(tcp::socket socket)
//        : socket_(std::move(socket)) {}
//
//    void start() {
//        do_read();
//    }
//
//private:
//    void do_read() {
//        auto self(shared_from_this());
//        socket_.async_read_some(boost::asio::buffer(data_, max_length),
//            [this, self](boost::system::error_code ec, std::size_t length) {
//                if (!ec) {
//                    std::cout << "Received: " << std::string(data_, length) << std::endl;
//                    do_write(length);
//                }
//            });
//    }
//
//    void do_write(std::size_t length) {
//        auto self(shared_from_this());
//        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
//            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
//                if (!ec) {
//                    do_read();
//                }
//            });
//    }
//
//    tcp::socket socket_;
//    enum { max_length = 1024 };
//    char data_[max_length];
//};
//
//class Server {
//public:
//    Server(boost::asio::io_context& io_context, short port)
//        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
//        do_accept();
//    }
//
//private:
//    void do_accept() {
//        acceptor_.async_accept(
//            [this](boost::system::error_code ec, tcp::socket socket) {
//                if (!ec) {
//                    std::cout << "New connection accepted" << std::endl;
//                    std::make_shared<Session>(std::move(socket))->start();
//                }
//
//                do_accept();
//            });
//    }
//
//    tcp::acceptor acceptor_;
//};
//
//int main(int argc, char* argv[]) {
//    try {
//        if (argc != 2) {
//            std::cerr << "Usage: async_tcp_echo_server <port>\n";
//            return 1;
//        }
//
//        boost::asio::io_context io_context;
//
//        Server s(io_context, std::atoi(argv[1]));
//
//        std::cout << "Server running on port " << argv[1] << std::endl;
//        io_context.run();
//    }
//    catch (std::exception& e) {
//        std::cerr << "Exception: " << e.what() << "\n";
//    }
//
//    return 0;
//}


//#include <iostream>
//#include <memory>
//#include <string>
//#include <unordered_map>
//#include <set>
//#include <boost/asio.hpp>
//
//using boost::asio::ip::tcp;
//
//class ChatRoom;
//
//class ChatParticipant {
//public:
//    virtual ~ChatParticipant() {}
//    virtual void deliver(const std::string& msg) = 0;
//};
//
//class ChatRoom {
//public:
//    void join(std::shared_ptr<ChatParticipant> participant) {
//        participants_.insert(participant);
//    }
//
//    void leave(std::shared_ptr<ChatParticipant> participant) {
//        participants_.erase(participant);
//    }
//
//    void broadcast(const std::string& msg, std::shared_ptr<ChatParticipant> sender) {
//        for (auto participant : participants_) {
//            if (participant != sender) {
//                participant->deliver(msg);
//            }
//        }
//    }
//
//private:
//    std::set<std::shared_ptr<ChatParticipant>> participants_;
//};
//
//class ChatSession : public ChatParticipant, public std::enable_shared_from_this<ChatSession> {
//public:
//    ChatSession(tcp::socket socket, std::unordered_map<std::string, ChatRoom>& rooms)
//        : socket_(std::move(socket)), rooms_(rooms) {}
//
//    void start() {
//        read_room_name();
//    }
//
//private:
//    // ChatSession 클래스의 read_room_name 함수를 수정합니다.
//    void read_room_name() {
//        auto self(shared_from_this());
//        boost::asio::async_read_until(socket_, buffer_, "\n",
//            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
//                if (!ec) {
//                    std::string room_name;
//                    std::istream is(&buffer_);
//                    std::getline(is, room_name);
//                    room_name_ = room_name;
//
//                    if (rooms_.find(room_name) == rooms_.end()) {
//                        rooms_[room_name] = ChatRoom();
//                    }
//
//                    rooms_[room_name].join(shared_from_this());
//                    do_read();
//                }
//            });
//    }
//
//    // ChatSession 클래스의 do_read 함수를 수정합니다.
//    void do_read() {
//        auto self(shared_from_this());
//        boost::asio::async_read_until(socket_, buffer_, "\n",
//            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
//                if (!ec) {
//                    std::string msg;
//                    std::istream is(&buffer_);
//                    std::getline(is, msg);
//                    rooms_[room_name_].broadcast(msg + "\n", shared_from_this());
//                    do_read();
//                }
//                else {
//                    rooms_[room_name_].leave(shared_from_this());
//                }
//            });
//    }
//
//    // ChatSession 클래스의 deliver 함수를 수정합니다.
//    void deliver(const std::string& msg) override {
//        auto self(shared_from_this());
//        boost::asio::async_write(socket_, boost::asio::buffer(msg),
//            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
//                if (ec) {
//                    rooms_[room_name_].leave(shared_from_this());
//                }
//            });
//    }
//    tcp::socket socket_;
//    boost::asio::streambuf buffer_;
//    std::unordered_map<std::string, ChatRoom>& rooms_;
//    std::string room_name_;
//};
//
//class ChatServer {
//public:
//    ChatServer(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
//        : acceptor_(io_context, endpoint) {
//        do_accept();
//    }
//
//private:
//    void do_accept() {
//        acceptor_.async_accept(
//            [this](boost::system::error_code ec, tcp::socket socket) {
//                if (!ec) {
//                    std::make_shared<ChatSession>(std::move(socket), rooms_)->start();
//                }
//                do_accept();
//            });
//    }
//
//    tcp::acceptor acceptor_;
//    std::unordered_map<std::string, ChatRoom> rooms_;
//};
//
//int main(int argc, char* argv[]) {
//    try {
//        if (argc != 2) {
//            std::cerr << "Usage: chat_server <port>\n";
//            return 1;
//        }
//
//        boost::asio::io_context io_context;
//        tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
//        ChatServer server(io_context, endpoint);
//
//        std::cout << "Chat server running on port " << argv[1] << std::endl;
//        io_context.run();
//    }
//    catch (std::exception& e) {
//        std::cerr << "Exception: " << e.what() << "\n";
//    }
//
//    return 0;
//}