#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <future>
#include <thread>
class ClientOnServer {
private:
	
	boost::asio::ip::tcp::socket m_socket;
	std::string personal_name;
	
public:
	std::string m_message;
	ClientOnServer(boost::asio::ip::tcp::socket socket, std::atomic<bool>* flag) : m_socket(std::move(socket)) {
		boost::asio::streambuf str_buf;
		boost::asio::read_until(m_socket, str_buf, '\n');
		std::istream input_stream(&str_buf);
		std::getline(input_stream, personal_name);
		*flag = true;
	}
	std::string get_name() {
		return this->personal_name;
	}
	void add_message(std::string* message_) {
		m_message = *message_;
	}
	
	
};
class Server {
private:
	int port = 3333;
	std::string localhost = "127.0.0.1";
	const std::size_t size = 30;
	boost::asio::ip::tcp::endpoint endpoint;
	boost::asio::io_service io_service;
	std::condition_variable* con_;
	boost::asio::ip::tcp::acceptor acceptor;
	std::string message_= " ";
	std::mutex mutex_;
	std::atomic<bool> close_flag = false;
	std::atomic<bool> flag = false;
public:
	Server() : endpoint(boost::asio::ip::address_v4::any(), port), acceptor(io_service, endpoint.protocol()) {
		acceptor.bind(endpoint);
		acceptor.listen(size);
	}
	void start() {
		boost::asio::ip::tcp::socket socket(io_service);
		acceptor.accept(socket);
		std::atomic<bool> flag = false;
		if (close_flag.load()) {
			return;
		}
		std::future<void> tr_start = std::async(&Server::start, this);
		ClientOnServer client(std::move(socket), &flag);
		std::future<void> tr_get = std::async(&Server::get, this, std::move(socket), &client);
		std::future<void> tr_send = std::async(&Server::send, this, std::move(socket), &client);
		tr_start.get();
	}
	void get(boost::asio::ip::tcp::socket socket,ClientOnServer* client) {
		while (true) {
			boost::asio::streambuf str_buf;
			client->add_message(&message_);
			boost::asio::read_until(socket, str_buf, '\n');
			std::istream input_stream(&str_buf);
			std::getline(input_stream, client->m_message);
			if (client->m_message == "EOF") {
				std::lock_guard<std::mutex> lock(mutex_);
				message_ = client->get_name() + "\r";
				con_->notify_all();
				break;
			}
			flag = true;
			std::scoped_lock lock(mutex_);
			message_ = client->m_message;
			con_->notify_all();
		}
	}
	void send(boost::asio::ip::tcp::socket socket, ClientOnServer* client) {
		while (true) {
			std::unique_lock lock(mutex_);
			con_->wait(lock);
			 client->add_message(&message_);
			if (client->m_message == client->get_name() + '\r') {
				boost::asio::write(socket, boost::asio::buffer("EOF\n"));
				break;
			}
			if (client->m_message[client->m_message.size() - 1] == '\r'){
				client->m_message.erase(client->m_message.size() - 1);
				boost::asio::write(socket, boost::asio::buffer(client->m_message + " leave the chat\n"));
			}
			else {
				if (!flag) boost::asio::write(socket, boost::asio::buffer(client->m_message + "\n"));
				else flag = false;
			}
		}
	}
};

int main()
{
	Server().start();
	system("pause");
	return EXIT_SUCCESS;
}
