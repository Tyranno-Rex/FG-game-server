
#include "client.h"

mutex d_lock;

Client::Client(string ip, unsigned short port) : m_endpoint(asio::ip::address::from_string(ip), port)
{
	m_work.reset(new asio::io_service::work(m_ios));
	m_sock.reset(new boost::asio::ip::tcp::socket(m_ios, m_endpoint.protocol()));
}

void Client::Start(string nickName)
{
	for (int i = 0; i < 4; i++)
	{
		m_threadGroup.create_thread(bind(&Client::Run, this));
	}

	Service* service = new Service(m_ios, m_sock);
	m_sock->async_connect(m_endpoint, bind(&Service::Start, service, nickName));
}

void Service::Start(string nickName)
{
	m_sock->write_some(asio::buffer(nickName));
	m_ios.post(bind(&Service::ReceivedFunc, this));
	m_ios.post(bind(&Service::SendFunc, this));
}

void Client::Run()
{
	PrintTid("Thread Start!");
	m_ios.run();
	PrintTid("Thread FInish!");
}

void Service::ReceivedFunc()
{
	m_sock->async_read_some(asio::buffer(m_readBuf, MAXBUF), bind(&Service::ReceivedError, this, _1, _2));
}

void Service::ReceivedError(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (ec) {
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		m_sock->close();
		return;
	}

	if (bytes_transferred == 0) {
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Good Bye Client!" << endl;
		m_sock->close();
		return;
	}

	m_readBuf[bytes_transferred] = '\0';
	cout << m_readBuf << endl;
	m_ios.post(bind(&Service::ReceivedFunc, this));
}


void Service::SendFunc()
{
	fflush(stdin);
	std::getline(std::cin, m_writeBuf);
	m_sock->async_write_some(asio::buffer(m_writeBuf), bind(&Service::SendError, this, _1, _2));
}

void Service::SendError(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (ec) {
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		m_sock->close();
		return;
	}

	m_ios.post(bind(&Service::SendFunc, this));
}

void PrintTid(std::string mes)
{
	d_lock.lock();
	std::cout << "[" << this_thread::get_id() << "]  " << mes << std::endl;
	d_lock.unlock();
}
