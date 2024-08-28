#include "server.h"

const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;

int main()
{
	std::string exist;
	unsigned short port_num = 8080;

	try
	{
		Server srv;

		unsigned int thread_pool_size =
			thread::hardware_concurrency() * 2;
		
		if (thread_pool_size == 0)
			thread_pool_size = DEFAULT_THREAD_POOL_SIZE;

		srv.Start(port_num, thread_pool_size);

		while (1)
		{
			std::cin >> exist;

			if (exist == "/Exit")
			{
				cout << "Close Server" << endl;
				srv.Stop();
			}

			else if (exist == "/Info")
			{
				PrintClientsInfo();
			}

			else if (exist == "/Room")
			{
				PrintRoomsInfo();
			}

			else
			{
				cout << "It does not exist." << endl
					<< "/Exit" << endl
					<< "/Info" << endl
					<< "/Room" << endl;
			}
		}
	}

	catch (system::system_error& e)
	{
		std::cout << "Error occured! Error code = "
			<< e.code() << ". Message: "
			<< e.what();
	}

	return 0;
}