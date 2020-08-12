#define AALL_USE_DEFAULT_SETTINGS
#include <aall/log.hpp>

std::string_view aall::progname{"PRODUCER"};
thread_local aall::Sender aall::threadlogger{};

int main(int argc, const char *argv[]) {
   aall::setThreadID("main");
   using aall::LogLevels;
   using aall::Tags;
   using aall::logging::log_;

   //aall::logging::initialize(argv[0], "tcp://127.0.0.1:37837");
   std::thread t([]{
      aall::setThreadID("thread2");
      while(1){
         log_("hello_thread",{}, VRBS);
         usleep(100000);
      }
      
   });

   const char * xx = argv[0];
   while (1) {

      auto yy = aall::StringLiteral(xx, argc + 1);
      //log("hello_world", Tags{"test1", "test2"}, VRBS);
      log_<LogLevels::err>("hello_world", Tags{"test3", "test4", xx}, VRBS);

      usleep(100000);
   }
}