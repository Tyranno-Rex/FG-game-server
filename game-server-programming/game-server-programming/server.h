#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>

#define MAXBUF 80

using namespace boost;
using std::cout;
using std::endl;
using std::vector;
using std::string;

using namespace boost;

class Client
{
public:
	shared_ptr<asio::ip::tcp::socket> m_sock;
	string m_nickName;
	int m_roomNum = 0;

	Client(shared_ptr<asio::ip::tcp::socket> sock, string nickName)
		: m_nickName(nickName), m_sock(sock)
	{}
};

class Room
{
public:
	string m_roomName;
	vector<Client*> m_sockVector;

	Room(string roomName) : m_roomName(roomName) {}
};

class Service
{
	shared_ptr<asio::ip::tcp::socket> m_sock;
	std::string m_response;
	char m_request[MAXBUF];
	asio::io_service& m_ios;
	asio::io_service::strand m_io_strand;
	bool exitClient = false;
	string m_nickName;
	string m_whisperNickName;
	bool m_whisperbool = false;
	Client* m_client;

	void onRequestReceived(const boost::system::error_code&, size_t);
	void onFinish(shared_ptr<asio::ip::tcp::socket>);
	void EchoSend(string);
	void WhisperSend(string, string);

	void Whisper(const boost::system::error_code&, size_t);
	void Whisper2(const boost::system::error_code&, size_t);
	void Whisper3(const boost::system::error_code&, size_t);
	void WhisperLast(const boost::system::error_code&, size_t);

	void createRoom(const boost::system::error_code&, size_t);
	void createRoom2(const boost::system::error_code&, size_t);

	void EnterRoom(const boost::system::error_code&, size_t);
	void EnterRoom2(const boost::system::error_code&, size_t);

public:
	Service(shared_ptr<asio::ip::tcp::socket> sock, asio::io_service& ios) :
		m_sock(sock), m_ios(ios), m_io_strand(m_ios)
	{}

	void StartHandling();
};

class Acceptor
{
	asio::io_service& m_ios;
	asio::ip::tcp::acceptor m_acceptor;
	atomic<bool> m_isStopped;
	asio::io_service::strand m_acc_strand;
	shared_ptr<asio::ip::tcp::socket> sock;

	void InitAccept();
	void onAccept(const boost::system::error_code&,
		shared_ptr<asio::ip::tcp::socket>);

public:
	Acceptor(asio::io_service&, unsigned short);
	void Start();
	void Stop();
};

class Server
{
	asio::io_service m_ios;
	shared_ptr<asio::io_service::work> m_work;
	shared_ptr<Acceptor> acc;
	thread_group m_thread_pool;

public:
	Server()
	{
		m_work.reset(new asio::io_service::work(m_ios));
	}

	void Start(unsigned short, unsigned int);
	void Run();
	void Stop();
};

void PrintTid(std::string);
void PrintClientsInfo();
void PrintRoomsInfo();