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
    std::string output;
    std::string personal_name;
    std::string local_host = "127.0.0.1";
    int port = 3333;
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket;
    boost::asio::ip::tcp::endpoint endpoint;
public:
    explicit Client(): endpoint(boost::asio::ip::address::from_string(local_host), port), socket(io_service, endpoint.protocol()) {
        try {
            socket.connect(endpoint);
            std::cout << "Please, enter your name to join the chat: ";
            getline(std::cin, personal_name);
            boost::asio::write(socket, boost::asio::buffer(personal_name + '\r'));
        }
        catch (boost::system::system_error& client_create)
        {
            std::cout << "Error occured! Error code = " << client_create.code() << ". Message: " << client_create.what() << std::endl;
            std::cout << "AUF" << std::endl;
            client_create.code().value();
            system("pause");
        }
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
        do {
            try
            {
                getline(std::cin, output);
                boost::asio::write(socket,boost::asio::buffer(output+"\n"));

            }
            catch (boost::system::system_error& send_client)
            {
                std::cout << "Error occured! Error code = " << send_client.code() << ". Message: " << send_client.what() << std::endl;
                system("pause");
                send_client.code().value();
            }
        } while (output != "Bye");
        end = true;
    }
    void get() {
        
        while (!end.load()) {
                try
                {
                    boost::asio::streambuf str_buf;
                    boost::asio::read_until(socket, str_buf, '\n');
                    std::istream input_stream(&str_buf);
                    std::string message;
                    std::getline(input_stream, message);
                    std::cout << message << std::endl;
                   
                }
                catch (boost::system::system_error& get_client)
                {
                    std::cout << "Error occured! Error code = " << get_client.code() << ". Message: " << get_client.what() << std::endl;
                    system("pause");
                    get_client.code().value();
                }
        }
    }
   
        
};
int main() {
   // system("chcp 1251");
    Client().start();
    system("pause");
    return EXIT_SUCCESS;

}
