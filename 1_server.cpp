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

using namespace boost::interprocess;
using char_allocator = allocator<char, managed_shared_memory::segment_manager>;
using shared_string = boost::interprocess::basic_string<char, std::char_traits<char>, char_allocator>;
using pair_allocator = allocator < std::pair<int, std::pair< std::string, std::string>>, managed_shared_memory::segment_manager >;
using shared_map = boost::unordered_map< int, std::pair< std::string, std::string>, boost::hash<int>, std::equal_to<int>, pair_allocator>;

class Server {
private:
    std::vector< std::future<void>> thr;
    interprocess_mutex* mutex_;
    interprocess_condition* condition_;
    std::atomic<bool> flag = false, end = false;
    shared_map* map_;
    int* ID_;
    shared_string* line;
   /* std::string personal_name;
    shared_string* current_name;*/
    const std::string shared_memory_name = "managed_shared_memory";
    managed_shared_memory shared_memory;
public:
    void add(boost::asio::ip::tcp::socket* socket)
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
           map_->emplace(*ID_, std::make_pair(name_str, message));
           (*ID_)++;
            flag = true;
            condition_->notify_all();
        } while (message != "Bye");
        end = true;
    }
    void out(boost::asio::ip::tcp::endpoint endpoint) {
        while (!end.load()) {
            std::unique_lock lock(*mutex_);
            condition_->wait(lock);
            if (!flag.load()) {
                try
                {
                    boost::asio::io_service io_service;
                    boost::asio::ip::tcp::socket socket(io_service, endpoint.protocol());
                    socket.connect(endpoint);
                    boost::asio::write(socket,
                        boost::asio::buffer("[" + map_->find(((*ID_) - 1))->second.first + "]: " + (map_->find((*ID_) - 1))->second.second));

                }
                catch (boost::system::system_error& e)
                {
                    std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;
                    system("pause");
                    e.code().value();
                }  
            }
            flag = false;
        }
    }
    void processing(boost::asio::ip::tcp::socket* socket, boost::asio::ip::tcp::endpoint endpoint){
       /* for (auto i : *map_) {
            std::cout << "[" << i.second.first << "]: " << i.second.second << std::endl;
        }*/
        std::future<void> th1 = std::async(&Server::add, this, socket);
        std::future<void> th2 = std::async(&Server::out, this, endpoint);
        using namespace std::chrono_literals;
        while (!end.load()) {
            std::this_thread::sleep_for(2ms);
        }
            th1.get();
            th2.get();
        system("pause");
    }
    void start() {
        shared_memory = managed_shared_memory(open_or_create, shared_memory_name.c_str(), 4096);
        mutex_ = shared_memory.find_or_construct<interprocess_mutex>("m")();
        condition_ = shared_memory.find_or_construct<interprocess_condition>("c")();
        map_ = shared_memory.find_or_construct<shared_map>("Users")(shared_memory.get_segment_manager());
        ID_ = shared_memory.find_or_construct<int>("ID")(0);
        line = shared_memory.find_or_construct<shared_string>("line")(shared_memory.get_segment_manager());
        const std::size_t size = 30;
        auto port = 3333;
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(), port);
        boost::asio::io_service io_service;
        do {
            try
            {
                boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint.protocol());
                acceptor.bind(endpoint);
                acceptor.listen(size);
                boost::asio::ip::tcp::socket socket(io_service);
                acceptor.accept(socket);
                thr.emplace_back(std::async(&Server::processing,this, &socket, endpoint));
            }
            catch (boost::system::system_error& e)
            {
                std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;
                system("pause");
                e.code().value();
            }
        }while(*line!= "Close server");
    }
};
int main() {
    system("chcp 1251");
    Server s;
    s.start();
    system("pause");
    return EXIT_SUCCESS;

}
