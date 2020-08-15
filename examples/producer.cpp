#define AALL_USE_DEFAULT_SETTINGS
#include <aall/log.hpp>

std::string_view aall::progname{"PRODUCER"};
thread_local aall::Sender aall::threadlogger{};
aall::Server aall::server{};

int main(int argc, const char *argv[]) {
   aall::setThreadID("main");
   using aall::LogLevels;
   using aall::Tags;
   using aall::logging::log_;

   
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
      log_<LogLevels::err>("hello_world1", Tags{"test1", "test2"}, VRBS);
      log_<LogLevels::err>("hello_world2", Tags{"test3", "test4"}, VRBS);

      usleep(100000);
   }
}