#include <aall/log.hpp>
#include <iostream>
#include <zmq.hpp>

int main(int argc, char* argv[]) {
   zmq::context_t zcontext{};
   zmq::socket_t zsock{zcontext, zmq::socket_type::pull};
   zsock.connect("tcp://127.0.0.1:37837");
   while (true) {
      zmq::message_t msg{};
      auto rx = zsock.recv(msg);
      aall::logging::log(msg.to_string());
   }
   return 0;
}