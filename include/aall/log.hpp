#ifndef AALL_LOG_HPP
#define AALL_LOG_HPP

#include <date/date.h>
#include <stdlib.h>

#include <array>
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>
#include <zmq.hpp>

#define FMT_HEADER_ONLY 1
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#undef FMT_HEADER_ONLY

#include <unistd.h>

/** MACROS **/
#define ___l //fmt::print("{}\n", __LINE__);

#define AALL_VERBOSE \
   ::aall::util::Context { __LINE__, __func__, __FILE__ }
#ifndef VRBS
#define VRBS AALL_VERBOSE
#endif

namespace aall {
#if defined(__cplusplus)
#if __cplusplus >= 202003L
#define AALL_CPPSTD 20
using StringLIteral = std::string_view;
#elif __cplusplus >= 201703L
#define AALL_CPPSTD 17
#define AALL_IF_CONSTEXPR if constexpr
#define AALL_INLINE_VAR inline
using StringLiteral = std::string_view;
#elif __cplusplus >= 201402L
#define AALL_CPPSTD 14
#define AALL_IF_CONSTEXPR if
#define AALL_INLINE_VAR
using StringLiteral = const char*;
#elif __cplusplus >= 201103L
#define AALL_CPPSTD 11
#define AALL_IF_CONSTEXPR if
#define AALL_INLINE_VAR
using StringLiteral = const char*;
#else
#error "C++11 or greater is required"
#endif
#endif
}// namespace aall

// USER DEFINITIONS BEGIN
// Users SHOULD define at least progname, and AALL_DEFAULTS_SET if they want
// to forego explicit initialization of the MsgProxy (C++>17 only)

#ifndef AALL_BROADCAST_SKT
#define AALL_BROADCAST_SKT "tcp://127.0.0.1:51227"
#endif

#ifndef AALL_PROGNAME
#define AALL_PROGNAME "unnamed_proc"
#endif

#ifndef AALL_SERVERADDRLOCAL
#define AALL_SERVERADDRLOCAL "tcp://127.0.0.1:51228"
//"inproc://aalllocalserver"
#endif

#ifndef AALL_SERVERADDR_PUBLISH
#define AALL_SERVERADDR_PUBLISH "tcp://127.0.0.1:51229"
#endif

#ifndef AALL_SERVER_TYPE
#define AALL_SERVER_TYPE ::aall::Server::LOCAL
#endif

/** END MACROS**/

namespace aall {

using Tags = std::initializer_list<StringLiteral>;
// using StringLiteral = const char * until C++17.

static constexpr bool MESSAGES_DISABLED = false;
static constexpr bool MESSAGES_ENABLED = true;
static constexpr bool TIME_ENABLED = true;
static constexpr bool TIME_DISABLED = false;

/**
 * NAME: LogLevels
 * DESCRIPTION: logging LogLevels, from lowest to highest priority
 */
enum LogLevels { dbg, msg, wrn, err, ftl, max };

namespace util {

template <typename T>
constexpr inline aall::StringLiteral file_name(T const& path) {
#if AALL_CPPSTD >= 17
   T file = path;
   auto i = 0L;

   while (path.size() && i + 1 < path.size()) {
      i++;
      if (i > 1 && (path[i - 1] == '/' || path[i - 1] == '\\')) {
         file = path.substr(i, path.size() - i);
      }
   }

   return file;
#else
   return path;
#endif
}

// returns a string associated with the log level
constexpr StringLiteral getLogLevelstring(LogLevels l) {
   switch (l) {
      case LogLevels::dbg:
         return "debug";
      case LogLevels::msg:
         return "message";
      case LogLevels::wrn:
         return "warning";
      case LogLevels::err:
         return "ERROR";
      case LogLevels::ftl:
         return "FATAL";
      default:
         break;
   }
   return "";
}

// mainly used for the VRBS macro. will help with printing line/function/file
// info
struct Context {
   int line = 0;
   StringLiteral func = "";
   StringLiteral file = "";

   constexpr Context(int l, StringLiteral const& f1, StringLiteral const& f2)
       : line(l), func(f1), file(util::file_name(f2)) {}
   constexpr Context() : Context(0, "", "") {}

   constexpr Context(Context&& other) = default;
   constexpr Context(Context const& other) = default;
};

}// namespace util

#if AALL_CPPSTD >= 17
inline zmq::context_t zmqContext{1};
#else
extern zmq::context_t zmqContext;
#endif

extern aall::StringLiteral progname;

//Server
//      basically forwards all messages
//      to subscribers, and broadcasts the service
//      (service name/ tcp listening port) to
//      the CLI/whoever is listening
//The big boy
class Server {

   enum Type { LOCAL, EXTERNAL, TYPE_MAX };

   std::thread proxythread{};
   std::thread broadcastThread{};
   zmq::socket_t zmqSubSocket{aall::zmqContext, zmq::socket_type::xsub};
   zmq::socket_t zmqPubSocket{aall::zmqContext, zmq::socket_type::xpub};
   zmq::socket_t serviceBroadcaster{aall::zmqContext, zmq::socket_type::pub};
   zmq::socket_ref captureSock{};
   bool broadcasting = false; //TODO: make this thread safe, std::atomic is not 
                              //      movable by default
   std::string publishAddr;

public:

   Server(const char* svraddrlocal = AALL_SERVERADDRLOCAL,
          const char* svraddrPublish = AALL_SERVERADDR_PUBLISH,
          zmq::socket_ref captureSkt = zmq::socket_ref(),
          bool broadcast = true)
       : captureSock(captureSkt)
       , broadcasting(broadcast)
       , publishAddr(svraddrPublish){
      
      using namespace std::chrono_literals;
      
      zmqSubSocket.bind(svraddrlocal);
      zmqPubSocket.bind(svraddrPublish);
      
      //forwards messages+subscriptions thru xpub/xsub
      proxythread = std::thread(
          [this] { zmq::proxy(zmqPubSocket, zmqSubSocket, captureSock); });

      //we can, like, not tell potential listeners we exist
      if(broadcasting){
         serviceBroadcaster.set(zmq::sockopt::sndhwm, 1);
         //TODO: make this configurable? maybe?
         serviceBroadcaster.connect(AALL_BROADCAST_SKT);

         //broadcast progname/tcp addr to the listener (CLI prob)
         broadcastThread = std::thread(
            [this] {
               while(broadcasting){
                  fmt::print("broadcasting");
                  serviceBroadcaster.send(zmq::buffer("AALL_CHANNEL"), 
                                          zmq::send_flags::sndmore);
                  serviceBroadcaster.send(zmq::buffer(aall::progname),
                                         zmq::send_flags::sndmore);
                  serviceBroadcaster.send(zmq::buffer(publishAddr));
                  std::this_thread::sleep_for(500ms);
               }
               return;
            });
      }
   }

   Server(Server&&) = default;
   Server& operator=(Server&&) = default;

   ~Server() {
      broadcasting = true;
      broadcastThread.join();
      serviceBroadcaster.close();
      zmqPubSocket.close();
      zmqSubSocket.close();
      zmqContext.close();
      proxythread.join(); //proxy automatically closes with context
      
   }
};

//TODO: Add description
struct Sender {
   zmq::socket_t zmqSocket{zmqContext, zmq::socket_type::pub};
   std::string threadID;

   public:

   Sender(const char* threadid = "main", const char* dmn = AALL_SERVERADDRLOCAL)
       : threadID(threadid) {
      zmqSocket.connect(dmn);
   }

   Sender(Sender&&) = default;
   Sender& operator=(Sender&&) = default;

   template <typename T>
   friend void setThreadID(T&& id);
};

//VARIABLES
//do I want to keep these global-ish?
extern Server server;
extern thread_local aall::Sender threadlogger;

// Recommend Call this at the beginning of the thread
template <typename T>
inline void setThreadID(T&& id) {
   aall::threadlogger.threadID = std::forward<T>(id);
}

static void setProgname(std::string_view progName) {
   std::swap(aall::progname, progName);
}

namespace logging {
/**
 * NAME: Log
 * DESCRIPTION: Main logging function
 * PARAMS:
 * msg -- the message to send
 * Tags -- the Tags to categorize your message
 * Context -- data regarding where/when the function is called
 * RETURNS:
 * nothing.
 */
template <LogLevels lvl = LogLevels::msg,
          bool timeEnabled = TIME_ENABLED,
          bool sendMsgs = MESSAGES_ENABLED,
          typename T = std::string>
void log_(T&& msg, Tags t = Tags{}, util::Context&& c = util::Context{}) {
   using namespace date;

   auto& progname = aall::progname;
   auto& threadID = aall::threadlogger.threadID;
   auto numTags = t.size();

   std::string datestr;
   std::string tagstr;

   AALL_IF_CONSTEXPR(timeEnabled) {
      datestr = date::format("[%F %T]", std::chrono::system_clock::now());
   }

   //Create a string like "#tag1, #tag2, #tag3..."
   if (numTags > 0) {
      auto iter = t.begin();

      //add first tag to our tag string
      fmt::format_to(
          std::back_insert_iterator{tagstr}, FMT_STRING("#{}"), *iter);
      iter++;

      //add the rest
      for (; iter < t.end(); iter++) {
         fmt::format_to(
             std::back_insert_iterator{tagstr}, FMT_STRING(", #{}"), *iter);
      }
   }

   //create our format string
   auto fmtmsg =
       fmt::format(FMT_STRING("{} (({})) <<{}>> {}:{}:{} <{}> [{}]: {} \n"),
                   datestr,
                   progname,
                   threadID,
                   c.file,
                   c.func,
                   c.line ? c.line : 0,
                   tagstr,
                   util::getLogLevelstring(lvl),
                   std::forward<T>(msg));

   //print our log string ((TODO: make this optional))
   fmt::print(fmtmsg);

   //Send our program name
   aall::threadlogger.zmqSocket.send(zmq::buffer(progname),
                                     zmq::send_flags::sndmore);

   //send our tags
   for(auto& tag : t){
      aall::threadlogger.zmqSocket.send(zmq::buffer(tag), zmq::send_flags::sndmore);
   }
   //send our tag-delimeter
   aall::threadlogger.zmqSocket.send(zmq::message_t{}, zmq::send_flags::sndmore);
   //send the rest of our messge ((WORK IN PROGRESS))
   aall::threadlogger.zmqSocket.send(zmq::buffer(fmtmsg),
                                     zmq::send_flags::dontwait);
}

}// namespace logging
}// namespace aall

#endif