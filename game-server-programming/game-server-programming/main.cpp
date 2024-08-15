#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <set>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class ChatRoom;

class ChatParticipant {
public:
    virtual ~ChatParticipant() {}
    virtual void deliver(const std::string& msg) = 0;
};

class ChatRoom {
public:
    void join(std::shared_ptr<ChatParticipant> participant) {
        participants_.insert(participant);
    }

    void leave(std::shared_ptr<ChatParticipant> participant) {
        participants_.erase(participant);
    }

    void broadcast(const std::string& msg, std::shared_ptr<ChatParticipant> sender) {
        for (auto participant : participants_) {
            if (participant != sender) {
                participant->deliver(msg);
            }
        }
    }

private:
    std::set<std::shared_ptr<ChatParticipant>> participants_;
};

class ChatSession : public ChatParticipant, public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket, std::unordered_map<std::string, ChatRoom>& rooms)
        : socket_(std::move(socket)), rooms_(rooms) {}

    void start() {
        read_room_name();
    }

private:
    void read_room_name() {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, buffer_, "\n",
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::string room_name;
                    std::istream is(&buffer_);
                    std::getline(is, room_name);
                    room_name_ = room_name;

                    if (rooms_.find(room_name) == rooms_.end()) {
                        rooms_[room_name] = ChatRoom();
                    }

                    rooms_[room_name].join(shared_from_this());
                    do_read();
                }
            });
    }

    void do_read() {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, buffer_, "\n",
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::string msg;
                    std::istream is(&buffer_);
                    std::getline(is, msg);
                    rooms_[room_name_].broadcast(msg + "\n", shared_from_this());
                    do_read();
                }
                else {
                    rooms_[room_name_].leave(shared_from_this());
                }
            });
    }

    void deliver(const std::string& msg) override {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(msg),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    rooms_[room_name_].leave(shared_from_this());
                }
            });
    }

    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    std::unordered_map<std::string, ChatRoom>& rooms_;
    std::string room_name_;
};

class ChatServer {
public:
    ChatServer(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
        : acceptor_(io_context, endpoint) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<ChatSession>(std::move(socket), rooms_)->start();
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
    std::unordered_map<std::string, ChatRoom> rooms_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: chat_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
        ChatServer server(io_context, endpoint);

        std::cout << "Chat server running on port " << argv[1] << std::endl;
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}