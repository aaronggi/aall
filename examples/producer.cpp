#include <aall/log.hpp>

int main(int argc, const char *argv[]) {
   using aall::LogLevels;
   using aall::Tags;
   using aall::logging::log;

   aall::logging::initialize(argv[0], "tcp://127.0.0.1:37837");

   while (1) {
      log("hello_world", Tags{"test1", "test2"}, VRBS);
      log<LogLevels::err>("hello_world", Tags{"test3", "test4"}, VRBS);

      usleep(100000);
   }
}