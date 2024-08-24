
#include "client.h"

int main()
{
	string serverIP;
	unsigned short serverPort;
	string nickName;

	cout << "Input Server IP : ";
	cin >> serverIP;

	cout << "Input Server PortNumber : ";
	cin >> serverPort;

	cout << "Input NickName : ";
	cin >> nickName;

	try {
		Client client(serverIP, serverPort);
		client.Start(nickName);

		while (1) {

		}
	}

	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = "
			<< e.code() << ". Message: "
			<< e.what();
	}
}