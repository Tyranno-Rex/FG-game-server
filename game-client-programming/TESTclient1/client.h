#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <iostream>
#include <string>

#define MAXBUF 80

using namespace boost;
using std::string;
using std::cin;
using std::cout;
using std::endl;


class Client
{
	asio::io_service m_ios;
	shared_ptr<asio::io_service::work> m_work;
	boost::shared_ptr<boost::asio::ip::tcp::socket> m_sock;
	thread_group m_threadGroup;
	asio::ip::tcp::endpoint m_endpoint;

public:
	Client(string, unsigned short);
	void Start(string);
	void Run();
};

class Service
{
	asio::io_service& m_ios;
	boost::shared_ptr<boost::asio::ip::tcp::socket> m_sock;
	char m_readBuf[MAXBUF];
	string m_writeBuf;

public:
	Service(asio::io_service& ios, shared_ptr<asio::ip::tcp::socket> sock) : m_ios(ios), m_sock(sock) {}
	void Start(string);
	void ReceivedFunc();
	void ReceivedError(const boost::system::error_code&, size_t);
	void SendFunc();
	void SendError(const boost::system::error_code&, size_t);
};

void PrintTid(std::string);