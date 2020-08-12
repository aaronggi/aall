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
#include <fmt/core.h>
#include <fmt/format-inl.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#undef FMT_HEADER_ONLY

#include <unistd.h>

/** MACROS **/

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
static inline zmq::context_t zmqContext{1};
#else
extern zmq::context_t zmqContext;
#endif

struct Server {
   enum Type { LOCAL, EXTERNAL, TYPE_MAX };
   std::thread proxythread{};
   zmq::socket_t zmqSubSocket{aall::zmqContext, ZMQ_XSUB};
   zmq::socket_t zmqPubSocket{aall::zmqContext, ZMQ_XPUB};
   zmq::socket_ref captureSock{};

   bool relay = false;

   Server(Type servertype = AALL_SERVER_TYPE,
          const char* svraddrlocal = AALL_SERVERADDRLOCAL,
          const char* svraddrPublish = AALL_SERVERADDR_PUBLISH,
          zmq::socket_ref capture = zmq::socket_ref())
       : captureSock(capture), relay(servertype == EXTERNAL ? true : false) {
      std::cout << "server1\n";

      std::cout << "server2\n";
      zmqSubSocket.bind(svraddrlocal);
      // zmqSubSocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);

      std::cout << "server3\n";

      std::cout << "server4\n";
      if (relay == false) {
         zmqPubSocket.bind(svraddrPublish);
         std::cout << "relay";
      }

      else {
         zmqPubSocket.connect(svraddrPublish);
         std::cout << "NOT RELAY";
      }

      std::cout << "server5\n";

      proxythread = std::thread(
          [this] { zmq::proxy(zmqPubSocket, zmqSubSocket, captureSock); });

      std::cout << "server6\n";
   }

   Server(Server&&) = default;
   Server& operator=(Server&&) = default;

   ~Server() {
      zmqPubSocket.close();
      zmqSubSocket.close();
      zmqContext.close();
      proxythread.join();
   }
};

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

#if AALL_CPPSTD >= 17
static inline Server server{};
#endif
extern std::string_view progname;
extern thread_local aall::Sender threadlogger;

// extern LoggerClass server;
// extern thread_local aall::Sender threadlogger;

// NOTE: this is highly recommended (for C++17 or greater)

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

   if (numTags > 0) {
      auto iter = t.begin();

      fmt::format_to(
          std::back_insert_iterator{tagstr}, FMT_STRING("#{}"), *iter);
      iter++;

      for (; iter < t.end(); iter++) {
         fmt::format_to(
             std::back_insert_iterator{tagstr}, FMT_STRING(", #{}"), *iter);
      }
   }

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

   fmt::print(fmtmsg);

   aall::threadlogger.zmqSocket.send(zmq::buffer(progname),
                                     zmq::send_flags::sndmore);
   aall::threadlogger.zmqSocket.send(zmq::buffer(fmtmsg),
                                     zmq::send_flags::dontwait);
}

}// namespace logging
}// namespace aall

#endif