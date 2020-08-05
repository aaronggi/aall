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
#if __cplusplus >= 201703L
#define AALL_CPP17
#define AALL_IF_CONSTEXPR if constexpr
#define AALL_CONSTEXPR constexpr
using StringLiteral = std::string_view;
#elif __cplusplus >= 201402L
#define AALL_CPP14
#define AALL_IF_CONSTEXPR if
#define AALL_CONSTEXPR constexpr
using StringLiteral = const char*;
#elif __cplusplus >= 201103L
#define AALL_CPP11
#define AALL_IF_CONSTEXPR if
#define AALL_CONSTEXPR
using StringLiteral = const char*;
#else
#error "C++11 or greater is required"
#endif
#endif
}// namespace aall
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

struct Messaging {
   zmq::context_t zcontext{};
   zmq::socket_t zsock{zcontext, zmq::socket_type::push};
};
// mainly used for the VRBS macro. will help with printing line/function/file
// info
struct Context {
   int line = 0;
   StringLiteral func = "";
   StringLiteral file = "";

#ifdef AALL_CPP11
   Context(int l, StringLiteral f1, StringLiteral f2)
       : line(l), func(f1), file(f2) {}
   Context(Context&& other) = default;
   Context(Context const& other) = default;
#endif
};

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

//[aaronggi]: these are currently just a bandaid to allow us to still
// use these variables globally, even though this is a header-only file.
inline Messaging& getMsgr() {
   static Messaging msg{};
   return msg;
}

inline std::mutex& getMtx() {
   static std::mutex mutex;
   return mutex;
}

inline std::string& getProgname() {
   static std::string prgname;
   return prgname;
}
}// namespace util

template <typename Serializer, typename Sender, typename StringType>
struct Logger {
   using string_type = StringType;
   using serializer_type = Serializer;
   using sender_type = Sender;

   static Logger defaultLogger{};

   string_type threadID{};
   serializer_type serializer{};
   sender_type sender{};

   Logger(string_type thread_id)
       : Logger{std::move(thread_id), serializer_type{}, sender_type{}} {}

   Logger(string_type&& thread_id,
          serializer_type&& srlizer,
          sender_type&& sndr)
       : threadID{threadID}
       , serializer{std::move(srlizer)}
       , sender{std::move(sndr)} {}

   Logger(Logger&& logger) = default;

   Logger& operator=(Logger&& rhs) = default;


};

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
             bool sendMsgs = MESSAGES_ENABLED>
   void log(std::string const& msg,
            Tags t = Tags{},
            util::Context && c = util::Context{},
            util::Messaging * sender = &util::getMsgr()) {
      using namespace date;

      auto& progname = util::getProgname();
      auto numTags = t.size();
      auto& mtx = util::getMtx();

      std::string datestr;
      std::string tagstr;

      AALL_IF_CONSTEXPR(timeEnabled) {
         std::ostringstream sstream;
         sstream << '[' << std::chrono::system_clock::now() << ']';
         datestr = sstream.str();
      }

      if (numTags > 0) {
         for (auto const& str : t) {
            tagstr += str;
            tagstr += ",";
         }
      }

      auto fmtmsg = fmt::format("[{}] (({})) {}:{}:{} <Tags: {}> [{}]: {} \n",
                                datestr,
                                progname,
                                c.file,
                                c.func,
                                c.line ? c.line : 0,
                                tagstr,
                                util::getLogLevelstring(lvl),
                                msg);

      std::cout << fmtmsg;

      {
         std::lock_guard<std::mutex> lg{mtx};
         sender->zsock.send(zmq::buffer(fmtmsg), zmq::send_flags::dontwait);
      }
   }

   /**
    * NAME: Initialize
    * DESCRIPTION: bootstrapping function, mainly sets the program name for
    * logging PARAMS:
    *  - argv: the program arguments
    * RETURNS:
    *  nothing
    */
   void initialize(
       const char* prgname, const char* addr, const bool sendMsgs = true) {
      auto& progname = util::getProgname();
      auto& messaging = util::getMsgr();

      progname = std::string(prgname);

      auto slash = progname.find_last_of("/\\");

      if (slash != std::string::npos) {
         progname = progname.substr(slash + 1);
      }

      if (sendMsgs) {
         messaging.zsock.bind(addr);
      }
   }

}// namespace logging
}// namespace aall

#endif