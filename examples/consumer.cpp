//#define AALL_PROGNAME "Consumer"

//#include <aall/log.hpp>
#include <iostream>
#include <zmq.hpp>

int main(int argc, char* argv[]) {
   zmq::context_t zcontext{1};
   zmq::socket_t zsock{zcontext, zmq::socket_type::sub};
   std::cout<<"print1\n";
   zsock.connect("tcp://127.0.0.1:51229");
   zsock.set(zmq::sockopt::subscribe, "");
   while (true) {

   std::cout<<"print1\n";
      zmq::message_t msg{};
      auto rx = zsock.recv(msg);
      auto rxPayload = zsock.recv(msg);
      std::cout << (const char *)msg.data();
   }
   return 0;
}