#define BOOST_DATE_TIME_NO_LIB
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
class ClientOnServer {
private:

	boost::asio::ip::tcp::socket m_socket;
	std::string personal_name;
	std::mutex m_mutex;
	std::string m_message;
	std::atomic<bool> m_flag;
public:

	ClientOnServer(boost::asio::ip::tcp::socket socket, std::condition_variable* con_, std::string* message_, std::mutex* mutex_,
		std::atomic<bool>* flag) : m_socket(std::move(socket)) {
		boost::asio::streambuf str_buf;
		boost::asio::read_until(m_socket, str_buf, '\n');
		std::istream input_stream(&str_buf);
		std::getline(input_stream, personal_name);
		std::future<void> thr_out = std::async(&ClientOnServer::send, this, con_, message_);
		std::future<void> thr_get = std::async(&ClientOnServer::get, this, con_, message_, mutex_);
		thr_out.get();
		thr_get.get();
		*flag = true;
	}
	/*std::string get_name() {
		return this->personal_name;
	}
	void add_message(std::string* message_) {
		m_message = *message_;
	}*/
	void get(std::condition_variable* con_, std::string* message_, std::mutex* mutex_) {
		while (true) {
			boost::asio::streambuf str_buf;
			m_message = *message_;
			boost::asio::read_until(m_socket, str_buf, '\n');
			std::istream input_stream(&str_buf);
			std::getline(input_stream, m_message);
			if (m_message == "EOF") {
				std::lock_guard<std::mutex> lock(*mutex_);
				*message_ = personal_name + "\r";
				con_->notify_all();
				break;
			}
			m_flag = true;
			std::scoped_lock lock(*mutex_);
			*message_ = m_message;
			con_->notify_all();
		}
	}
	void send(std::condition_variable* con_, std::string* message_) {
		while (true) {
			std::unique_lock lock(m_mutex);
			con_->wait(lock);
			m_message = *message_;
			if (m_message == personal_name + '\r') {
				boost::asio::write(m_socket, boost::asio::buffer("EOF\n"));
				break;
			}
			if (m_message[m_message.size() - 1] == '\r') {
				m_message.erase(m_message.size() - 1);
				boost::asio::write(m_socket, boost::asio::buffer(m_message + " leave the chat\n"));
			}
			else {
				if (!m_flag) {
					boost::asio::write(m_socket, boost::asio::buffer(m_message + "\n"));
				}
				else {
					m_flag = false;
				}
			}
		}
	}

};
class Server {
private:
	int port = 3333;
	const std::size_t size = 30;
	boost::asio::ip::tcp::endpoint endpoint;
	boost::asio::io_service io_service;
	std::condition_variable con_;
	boost::asio::ip::tcp::acceptor acceptor;
	std::string message_ = " ";
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
		ClientOnServer client(std::move(socket), &con_, &message_, &mutex_, &flag);
		tr_start.get();
	}
};

int main()
{	
	Server().start();
	system("pause");
	return EXIT_SUCCESS;
}
