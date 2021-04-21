#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <boost/asio.hpp>
class Client {
private:
	std::string local_host = "127.0.0.1";
	int port = 3333;
	boost::asio::ip::tcp::endpoint endpoint_;
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket socket_;
	std::string personal_name;
public:
	explicit Client() : endpoint_(boost::asio::ip::address::from_string(local_host), port), socket_(io_service, endpoint_.protocol()) {
		socket_.connect(endpoint_);
		std::cout << "Please, enter your name to join the chat: " << std::endl;
		std::getline(std::cin, personal_name);
		boost::asio::write(socket_, boost::asio::buffer(personal_name + '\n'));

	}
	void start() {
		std::future<void> tr_get= std::async(&Client::get, this);
		std::future<void> tr_send = std::async(&Client::send, this);
		
		tr_get.get();
		tr_send.get();
	}
	void get()
	{
		while (true) {
			boost::asio::streambuf str_buf;
			boost::asio::read_until(socket_, str_buf, '\n');
			std::istream input_stream(&str_buf);
			std::string message;
			std::getline(input_stream, message);
			if (message == "EOF") break;
			std::cout << message << std::endl;
		}

	}
	void send()
	{
		std::string output;
		while (std::getline(std::cin, output)) {
			boost::asio::write(socket_, boost::asio::buffer("[" + personal_name + "]" + " " + output + '\n'));
		}
		output = "EOF";
		boost::asio::write(socket_, boost::asio::buffer(output + '\n'));
	}


};



int main()
{
	Client().start();
	system("pause");

	return EXIT_SUCCESS;
}
