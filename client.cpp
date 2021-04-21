#define BOOST_DATE_TIME_NO_LIB
#include<iostream>
#include<string>
#include <future>
#include<chrono>
#include<map>
#include<atomic>
#include <thread>
#include <boost/asio.hpp>

class Client {
private:
    std::atomic<bool> flag = false, end = false;
    std::string personal_name;
    std::string local_host = "127.0.0.1";
    int port = 3333;
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket;
    boost::asio::ip::tcp::endpoint endpoint;
public:
    explicit Client(): endpoint(boost::asio::ip::address::from_string(local_host), port), socket(io_service, endpoint.protocol()) {
        socket.connect(endpoint);
        std::cout << "Please, enter your name to join the chat: " << std::endl;
        std::getline(std::cin, personal_name);
        boost::asio::write(socket, boost::asio::buffer(personal_name + '\n'));
    }
    void start() {
        std::future<void> th1 = std::async(&Client::send, this);
        std::future<void> th2 = std::async(&Client::get, this);
        th1.get();
        th2.get();
        system("pause");
    }
    void send()
    {
        while (true) {
            std::string output;
            while (std::getline(std::cin, output)) {
                boost::asio::write(socket, boost::asio::buffer("["+personal_name+"]" + " " + output + '\n'));
            }
            output = "EOF";
            boost::asio::write(socket, boost::asio::buffer(output + '\n'));
        }
    }
    void get() {
        
        while (!end.load()) {
                boost::asio::streambuf str_buf;
                boost::asio::read_until(socket, str_buf, '\n');
                std::istream input_stream(&str_buf);
                std::string message;
                std::getline(input_stream, message);
                if (message == "EOF") {
                    break;
                }
                std::cout << message << std::endl;
            }

        }
   
        
};
int main() {
   // system("chcp 1251");
    Client().start();
    system("pause");
    return EXIT_SUCCESS;

}
