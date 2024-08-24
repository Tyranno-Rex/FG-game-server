#include "server.h"

vector<Client*> sockVector;
vector<Room*> roomVec;
mutex d_lock;

void Server::Start(unsigned short port_num, unsigned int thread_pool_size)
{
	assert(thread_pool_size > 0);
	Room* room = new Room("AnterRoom");
	roomVec.push_back(room);
	acc.reset(new Acceptor(m_ios, port_num));
	acc->Start();

	for (int i = 0; i < thread_pool_size; i++)
	{
		m_thread_pool.create_thread(bind(&Server::Run, this));
	}
}

void Server::Run()
{
	PrintTid("ThreadStart!");
	m_ios.run();
	PrintTid("ThreadFinish!");
}

void Server::Stop()
{
	acc->Stop();
	m_ios.stop();
	m_thread_pool.join_all();
}

Acceptor::Acceptor(asio::io_service& ios, unsigned short port_num) :
	m_ios(ios), m_acc_strand(m_ios),
	m_acceptor(m_ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port_num)),
	m_isStopped(false)
{}

void Acceptor::Start()
{
	m_acceptor.listen();
	InitAccept();
}

void Acceptor::InitAccept()
{
	sock.reset(new asio::ip::tcp::socket(m_ios));

	d_lock.lock();

	m_acceptor.async_accept(*sock, m_acc_strand.wrap(bind(&Acceptor::onAccept, this, _1, sock)));
	d_lock.unlock();
}

void Acceptor::onAccept(const boost::system::error_code& ec, shared_ptr<asio::ip::tcp::socket> sock)
{
	if (!ec)
	{
		Service* serv = new Service(sock, m_ios);
		m_acc_strand.post(bind(&Service::StartHandling, serv));
	}
	else
	{
		std::cout << "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
	}

	if (!m_isStopped.load())
	{
		InitAccept();
	}
	else
	{
		m_acceptor.close();
	}
}

void Acceptor::Stop()
{
	m_isStopped = true;
}

void Service::StartHandling()
{
	PrintTid("StartHandling");

	d_lock.lock();
	m_sock->read_some(asio::buffer(m_request));

	m_client = new Client(m_sock, m_request);
	roomVec[0]->m_sockVector.push_back(m_client);
	sockVector.push_back(m_client);
	cout << "현재 클라이언트의 수 : " << sockVector.size() << endl;
	d_lock.unlock();

	m_nickName = m_request;
	PrintTid("WaitReading");
	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), m_io_strand.wrap(bind(&Service::onRequestReceived, this, _1, _2)));
	d_lock.unlock();
}

void Service::onRequestReceived(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (exitClient || m_whisperbool) return;

	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	if (bytes_transferred == 0)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Good Bye Client!" << endl;
		onFinish(m_sock);
		return;
	}

	PrintTid("StartWriting");

	if (strcmp(m_request, "/w") == 0)
	{
		m_whisperbool = true;
		m_response = "Input NickName!";
		d_lock.lock();
		m_sock->async_write_some(asio::buffer(m_response), bind(&Service::Whisper, this, _1, _2));
		d_lock.unlock();
		return;
	}

	else if ((strcmp(m_request, "/create") == 0) && (m_client->m_roomNum == 0))
	{
		m_response = "Input RoomName!";
		d_lock.lock();
		m_sock->async_write_some(asio::buffer(m_response), bind(&Service::createRoom, this, _1, _2));
		d_lock.unlock();
		return;
	}

	else if ((strcmp(m_request, "/enter") == 0) && (m_client->m_roomNum == 0))
	{
		m_response = "Input RoomName!";
		d_lock.lock();
		m_sock->async_write_some(asio::buffer(m_response), bind(&Service::EnterRoom, this, _1, _2));
		d_lock.unlock();
		return;
	}

	else if ((strcmp(m_request, "/exit") == 0) && (m_client->m_roomNum != 0))
	{
		roomVec[0]->m_sockVector.push_back(m_client);

		for (int i = 0; i < roomVec[m_client->m_roomNum]->m_sockVector.size(); i++)
		{
			if (roomVec[m_client->m_roomNum]->m_sockVector[i] == m_client)
			{
				roomVec[m_client->m_roomNum]->m_sockVector.erase((roomVec[m_client->m_roomNum]->m_sockVector.begin() + i));

				if (roomVec[m_client->m_roomNum]->m_sockVector.size() == 0)
				{
					roomVec.erase(roomVec.begin() + m_client->m_roomNum);
				}
				break;
			}
		}

		m_client->m_roomNum = 0;

		d_lock.lock();
		m_sock->async_read_some(asio::buffer(m_request, MAXBUF), m_io_strand.wrap(bind(&Service::onRequestReceived, this, _1, _2)));
		d_lock.unlock();
		return;
	}

	else if (strcmp(m_request, "/room") == 0)
	{
		system::error_code ec;
		m_response = "";

		for (auto oneOfRoom : roomVec)
		{
			m_response = m_response + "[" + oneOfRoom->m_roomName + "]" + "방장 : " + oneOfRoom->m_sockVector[0]->m_nickName + "\n";
		}

		d_lock.lock();
		m_client->m_sock->write_some(asio::buffer(m_response), ec);
		d_lock.unlock();

		if (ec)
		{
			std::cout << "Error occured! Error code = "
				<< ec.value()
				<< ". Message: " << ec.message();
		}

		memset(m_request, '\0', MAXBUF);

		d_lock.lock();
		m_sock->async_read_some(asio::buffer(m_request, MAXBUF), m_io_strand.wrap(bind(&Service::onRequestReceived, this, _1, _2)));
		d_lock.unlock();
		return;
	}

	m_request[bytes_transferred] = '\0';
	m_response = m_nickName + " : " + m_request;
	m_io_strand.post(bind(&Service::EchoSend, this, m_response));
}

void Service::Whisper(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), bind(&Service::Whisper2, this, _1, _2));
	d_lock.unlock();
}

void Service::Whisper2(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (exitClient) return;

	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	if (bytes_transferred == 0)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Good Bye Client!" << endl;
		onFinish(m_sock);
		return;
	}

	m_whisperNickName = m_request;
	cout << "[" << m_nickName << "->" << m_whisperNickName << " -- Whisper.]" << endl;
	m_response = "Input Message!";

	d_lock.lock();
	m_sock->async_write_some(asio::buffer(m_response), bind(&Service::Whisper3, this, _1, _2));
	d_lock.unlock();
}

void Service::Whisper3(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), bind(&Service::WhisperLast, this, _1, _2));
	d_lock.unlock();
}

void Service::WhisperLast(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (exitClient) return;

	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	if (bytes_transferred == 0)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Good Bye Client!" << endl;
		onFinish(m_sock);
		return;
	}

	m_request[bytes_transferred] = '\0';
	m_response = "[Whisper]" + m_nickName + " : " + m_request;

	m_ios.post(bind(&Service::WhisperSend, this, m_whisperNickName, m_response));
	m_whisperbool = false;
}

void Service::WhisperSend(string nickName, string response)
{
	system::error_code ec;

	for (int i = 0; i < sockVector.size(); i++)
	{
		if (sockVector[i]->m_nickName == nickName)
		{
			d_lock.lock();
			sockVector[i]->m_sock->write_some(asio::buffer(response), ec);
			d_lock.unlock();

			if (ec)
			{
				std::cout << "[" << (*m_sock).local_endpoint() << "]"
					<< "Error occured! Error code = "
					<< ec.value()
					<< ". Message: " << ec.message()
					<< endl;
				onFinish(m_sock);
				return;
			}
			break;
		}
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), m_io_strand.wrap(bind(&Service::onRequestReceived, this, _1, _2)));
	d_lock.unlock();
}

void Service::createRoom(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), bind(&Service::createRoom2, this, _1, _2));
	d_lock.unlock();
}

void Service::createRoom2(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (exitClient) return;

	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	if (bytes_transferred == 0)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Good Bye Client!" << endl;
		onFinish(m_sock);
		return;
	}

	string roomName = m_request + '\0';
	Room* room = new Room(roomName);
	roomVec.push_back(room);
	room->m_sockVector.push_back(m_client);

	for (int i = 0; i < roomVec[0]->m_sockVector.size(); i++)
	{
		if (roomVec[0]->m_sockVector[i] == m_client)
		{
			roomVec[0]->m_sockVector.erase(roomVec[0]->m_sockVector.begin() + i);
			break;
		}
	}

	cout << "방생성 완료!" << room->m_roomName << endl;

	for (int i = 0; i < roomVec.size(); i++)
	{
		if (roomName == roomVec[i]->m_roomName)
		{
			m_client->m_roomNum = i;
			break;
		}
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), m_io_strand.wrap(bind(&Service::onRequestReceived, this, _1, _2)));
	d_lock.unlock();
}

void Service::EnterRoom(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), bind(&Service::EnterRoom2, this, _1, _2));
	d_lock.unlock();
}

void Service::EnterRoom2(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (exitClient) return;

	if (ec)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message()
			<< endl;
		onFinish(m_sock);
		return;
	}

	if (bytes_transferred == 0)
	{
		std::cout << "[" << (*m_sock).local_endpoint() << "]"
			<< "Good Bye Client!" << endl;
		onFinish(m_sock);
		return;
	}

	string roomName = m_request;

	for (int i = 0; i < roomVec.size(); i++)
	{
		if (roomName == roomVec[i]->m_roomName)
		{
			roomVec[i]->m_sockVector.push_back(m_client);
			m_client->m_roomNum = i;
			cout << "[" << m_client->m_roomNum << " : " << roomVec[i]->m_roomName
				<< "]" << "에 " << m_nickName << "님이 입장하셨습니다." << endl;

			for (int i = 0; i < roomVec[0]->m_sockVector.size(); i++)
			{
				if (roomVec[0]->m_sockVector[i] == m_client)
				{
					roomVec[0]->m_sockVector.erase(roomVec[0]->m_sockVector.begin() + i);
					break;
				}
			}
			break;
		}
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), m_io_strand.wrap(bind(&Service::onRequestReceived, this, _1, _2)));
	d_lock.unlock();
}

void Service::EchoSend(string response)
{
	system::error_code ec;

	for (auto oneOfSock : roomVec[m_client->m_roomNum]->m_sockVector)
	{
		d_lock.lock();
		oneOfSock->m_sock->write_some(asio::buffer(m_response), ec);
		d_lock.unlock();

		if (ec)
		{
			std::cout << "Error occured! Error code = "
				<< ec.value()
				<< ". Message: " << ec.message();
		}
	}

	memset(m_request, '\0', MAXBUF);

	d_lock.lock();
	m_sock->async_read_some(asio::buffer(m_request, MAXBUF), m_io_strand.wrap(bind(&Service::onRequestReceived, this, _1, _2)));
	d_lock.unlock();
}

void Service::onFinish(shared_ptr<asio::ip::tcp::socket> sock)
{
	for (int i = 0; i < sockVector.size(); i++)
	{
		if (sockVector[i]->m_sock == sock)
		{
			d_lock.lock();
			sockVector.erase(sockVector.begin() + i);
			cout << "현재 총 클라이언트의 수 : " << sockVector.size() << endl;
			d_lock.unlock();
		}
	}

	for (int i = 0; i < roomVec[m_client->m_roomNum]->m_sockVector.size(); i++)
	{
		if (roomVec[m_client->m_roomNum]->m_sockVector[i]->m_sock == sock)
		{
			d_lock.lock();
			roomVec[m_client->m_roomNum]->m_sockVector.erase(roomVec[m_client->m_roomNum]->m_sockVector.begin() + i);
			cout << "현재" << roomVec[m_client->m_roomNum]->m_roomName
				<< " 클라이언트의 수 : " << roomVec[m_client->m_roomNum]->m_sockVector.size() << endl;
			d_lock.unlock();
		}
	}

	sock->close();
	exitClient = true;
	delete this;
}

void PrintTid(std::string mes)
{
	d_lock.lock();
	std::cout << "[" << this_thread::get_id() << "]  " << mes << std::endl;
	d_lock.unlock();
}

void PrintClientsInfo()
{
	for (int i = 0; i < sockVector.size(); i++)
	{
		cout << "[" << i << "]"
			<< sockVector[i]->m_nickName << endl;
	}
}

void PrintRoomsInfo()
{
	for (int i = 0; i < roomVec.size(); i++)
	{
		cout << "[" << i << "]"
			<< roomVec[i]->m_roomName << endl;
	}
}