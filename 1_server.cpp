#define BOOST_DATE_TIME_NO_LIB
#include<iostream>
#include<string>
#include <future>
#include<chrono>
#include<map>
#include<atomic>
#include <thread>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/asio.hpp>
std::string read_data(boost::asio::ip::tcp::socket& socket)
{
    const std::size_t length = 10;
    char buffer[length];
    boost::asio::read(socket, boost::asio::buffer(buffer, length));
    return std::string(buffer, length);
}


using namespace boost::interprocess;
using char_allocator = allocator<char, managed_shared_memory::segment_manager>;
using shared_string = boost::interprocess::basic_string<char, std::char_traits<char>, char_allocator>;
using pair_str_allocator = allocator < std::pair< std::string, shared_string>, managed_shared_memory::segment_manager >;
using pair_allocator = allocator < std::pair<int, std::pair< std::string, shared_string>>, managed_shared_memory::segment_manager >;
using shared_map = boost::unordered_map< int, std::pair< std::string, shared_string>, boost::hash<int>, std::equal_to<int>, pair_allocator>;

class Server {
private:
    std::vector< std::future<void>> thr;
    interprocess_mutex* mutex_;
    interprocess_condition* condition_;
    std::vector<std::pair<std::string, std::string>> data;
    std::atomic<bool> flag = false, end = false;
    shared_map* map_;
    int* ID_;
    shared_string* line;
    std::string personal_name;
    shared_string* current_name;
public:
    void add(boost::asio::ip::tcp::socket& socket)
    {
        std::string message;
        do {
            boost::asio::streambuf str_buf;
            boost::asio::streambuf name_buf;
            
            std::string name_str;
            size_t name_size;
            name_size = boost::asio::read_until(socket, name_buf, ' ');
            boost::asio::read_until(socket, str_buf, '\n');
            std::istream input_stream(&str_buf);
            std::getline(input_stream, message);
            std::istream input_stream(&name_buf);
            std::getline(input_stream, name_str);
            std::scoped_lock lock(mutex_);
            data.push_back(std::make_pair(name_str, message));
            flag = true;
            condition_->notify_all();
        } while (message != "Bye");
        end = true;
    }
    void out() {
        while (!end.load()) {
            std::unique_lock lock(*mutex_);
            condition_->wait(lock);
            if (!flag.load()) {
                std::cout << "[" <<map_->find((*ID_) - 1)->second.first << "]: " << (map_->find((*ID_) - 1))->second.second << std::endl;
            }
            flag = false;
        }
    }
    void processing(boost::asio::ip::tcp::socket& socket) {
        const std::string shared_memory_name = "managed_shared_memory";
        managed_shared_memory shared_memory;
        interprocess_mutex* mutex_;
        interprocess_condition* condition_;
    }
    void start() {
        const std::size_t size = 30;
        auto port = 3333;
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(), port);
        boost::asio::io_service io_service;
        try
        {
            boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint.protocol());
            acceptor.bind(endpoint);
            acceptor.listen(size);
            boost::asio::ip::tcp::socket socket(io_service);
            acceptor.accept(socket);
            thr.emplace_back(std::async(&Server::processing, socket));
        }
        catch (boost::system::system_error& e)
        {
            std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;
            system("pause");
            return e.code().value();
        }
    }
};
int main() {
    system("chcp 1251");
    Server s;
    s.start();
    system("pause");
    return EXIT_SUCCESS;

}
