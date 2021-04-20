#define BOOST_DATE_TIME_NO_LIB
#include<iostream>
#include<string>
#include <future>
#include<chrono>
#include<map>
#include<atomic>
#include <thread>
#include <boost/asio.hpp>
class ClientOnServer {
private:
    std::string personal_name;
    boost::asio::ip::tcp::socket m_socket;
    std::atomic<bool> flag;
public:
    ClientOnServer(boost::asio::ip::tcp::socket socket,
         std::atomic<bool>* flag) : m_socket(std::move(socket)) {
        boost::asio::streambuf buffer;
        boost::asio::read_until(m_socket, buffer, '\n');
        std::istream input_stream(&buffer);
        std::getline(input_stream, personal_name);
        *flag = true;
    }
    std::string get_name() {
        return this->personal_name;
    }
};

class Server {
private:
    std::mutex* mutex_;
    std::condition_variable* con_;
    boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::io_service io_service;
    std::vector<std::future<void>> thr;
    std::atomic<bool> flag = false;
    std::atomic<bool> close_flag = false;
    int ID = 0;
    std::map<int, std::pair<std::string, std::string>> map_;
    const std::size_t size = 30;
    int port = 3333;
    std::string* message_;
    const std::string close_str = "Close server";
public:
    Server():endpoint(boost::asio::ip::address_v4::any(), port), acceptor(io_service, endpoint.protocol()) {
        acceptor.bind(endpoint);
        acceptor.listen(size);
    }
    void start() {
        do {
            try
            {
                boost::asio::ip::tcp::socket socket(io_service);
                 if (!close_flag.load()) {
                    acceptor.accept(socket);
                    thr.push_back(std::async(&Server::processing, this, std::move(socket), std::move(endpoint)));
                    thr.back().get();
                }
                
            }
            catch (boost::system::system_error& server_start)
            {
                std::cout << "Error occured! Error code = " << server_start.code() << ". Message: " << server_start.what() << std::endl;
                system("pause");
                server_start.code().value();
            }
        } while (!close_flag.load());
    }
    
    void processing(boost::asio::ip::tcp::socket socket, boost::asio::ip::tcp::endpoint endpoint){
        try {
            ClientOnServer cl(std::move(socket), &flag);
            std::atomic<bool> end = false;
            std::future<void> th1 = std::async(&Server::get, this, std::move(socket), &end, &cl);
            std::future<void> th2 = std::async(&Server::send, this, std::move(socket), std::move(endpoint), &end);
            th1.get();
            th2.get();
        }
        catch (boost::system::system_error& server_processing)
        {
            std::cout << "Error occured! Error code = " << server_processing.code() << ". Message: " << server_processing.what() << std::endl;
            system("pause");
            server_processing.code().value();
        }
        system("pause");
    }
    void get(boost::asio::ip::tcp::socket socket, std::atomic<bool> end, ClientOnServer* cl)
    {

        do {
            try {
                std::scoped_lock lock(*mutex_);
                boost::asio::streambuf str_buf;
                boost::asio::read_until(socket, str_buf, '\n');
                std::istream input_str(&str_buf);
                std::getline(input_str, *message_);
                map_.emplace(ID, std::make_pair(cl->get_name(), *message_));
                ID++;
                flag = true;
                con_->notify_all();
                if (*message_ == close_str) close_flag = true;
            }
            catch (boost::system::system_error& server_get)
            {
                std::cout << "Error occured! Error code = " << server_get.code() << ". Message: " << server_get.what() << std::endl;
                system("pause");
                server_get.code().value();
            }
        } while (*message_ != "Bye");
        end = true;
    }
    void send(boost::asio::ip::tcp::socket socket, boost::asio::ip::tcp::endpoint endpoint, std::atomic<bool> end) {
        while (!end.load()) {
            std::unique_lock lock(*mutex_);
            con_->wait(lock);
            if (!close_flag.load()) {
                try
                {
                    boost::asio::write(socket,
                        boost::asio::buffer("[" + map_.find(ID - 1)->second.first + "]: " + (map_.find(ID - 1)->second.second)));

                }
                catch (boost::system::system_error& server_send)
                {
                    std::cout << "Error occured! Error code = " << server_send.code() << ". Message: " << server_send.what() << std::endl;
                    system("pause");
                    server_send.code().value();
                }
            }
            flag = false;
        }
    }
};
int main() {
    Server().start();
    system("pause");
    return EXIT_SUCCESS;

}
