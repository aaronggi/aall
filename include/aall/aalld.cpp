

#include "../../external/cppzmq/zmq.hpp"
#include "log.hpp"
#include <string>

#if __cplusplus >= 202002L
#define AALL_CPPSTD 20
#elif __cplusplus >= 201703L
#define AALL_CPPSTD 17
#elif __cplusplus >= 201402L
#define AALL_CPPSTD 14
#elif __cplusplus >= 201103L
#define AALL_CPPSTD 11
else
#error "C++11 or greater required"
#endif

//USER DEFINITIONS BEGIN
//Users SHOULD define at least progname, and AALL_DEFAULTS_SET if they want 
//to forego explicit initialization of the MsgProxy (C++>17 only)
#ifndef AALL_PROGNAME
#define AALL_PROGNAME "unnamed_proc"
#endif

#ifndef AALL_DAEMONADDRLOCAL
#define AALL_DAEMONADDRLOCAL "inproc://aalldaemonaddr"
#endif

#ifndef AALL_DAEMONADDR_PUBLISH
#define AALL_DAEMONADDR_PUBLISH = "tcp://localhost:51229"
#endif

#ifndef AALL_NODAEMON 
#define AALL_NODAEMON true
#endif

namespace aall{

struct MsgPublisher {
    zmq::socket_t zmqSocket{LoggerClass::zmqContext, ZMQ_PUB};
    std::string threadID{};
    public:
    MsgPublisher(const char * threadid = "", 
                 const char *dmn = AALL_DAEMONADDRLOCAL)
    : threadID(threadid) {
        zmqSocket.connect(dmn);
    }

    MsgPublisher(MsgPublisher&&) = default;
    MsgPublisher& operator=(MsgPublisher&&) = default;

    template<typename T>
    friend void setThreadID(T&& id);
};

//Recommend Call this at the beginning of the thread
template<typename T>
inline void setThreadID(T&& id){
    extern thread_local MsgPublisher aall::threadlogger;
    threadlogger.threadID = std::forward<T>(id);
}

struct Daemon {
    zmq::socket_t zmqSubSocket{LoggerClass::zmqContext, ZMQ_XSUB};
    zmq::socket_t zmqPubSocket{LoggerClass::zmqContext, ZMQ_XPUB};
    bool localdaemon = AALL_NODAEMON;

    Daemon(const char* dmnaddrlocal = AALL_DAEMONADDRLOCAL, 
           const char* dmnaddrPublish = AALL_DAEMONADDR_PUBLISH, 
           zmq::socket_ref capture = zmq::socket_ref()){

        zmqSubSocket.bind(AALL_DAEMONADDRLOCAL);

        if(localdaemon) zmqPubSocket.bind(dmnaddrPublish);
        else            zmqPubSocket.connect(dmnaddrPublish);

        zmq::proxy(zmqPubSocket,zmqSubSocket,capture);
    }
    Daemon(Daemon&&) = default;
    Daemon& operator=(Daemon&&) = default;
};



struct LoggerClass {
    static AALL_INLINE_VAR std::string program = AALL_PROGNAME;
    static AALL_INLINE_VAR std::string daemonaddr = AALL_DAEMONADDRLOCAL;
    
#ifdef AALL_CPPSTD >= 17
    static inline zmq::context_t zmqContext{1};
    Daemon daemon;
#else
    static zmq::context_t zmqContext;
    static void initialize(){ zmqContext = zmq::context_t{}; }
#endif

static void setProgname(std::string progname){
    std::swap(program, progname);
}
  
};

//extern LoggerClass server;
//xtern thread_local aall::MsgPublisher threadlogger;

}
#define AALL_USE_DEFAULT_SETTINGS
//NOTE: this is highly recommended (for C++17 or greater)
#ifdef AALL_USE_DEFAULT_SETTINGS 
#if AALL_CPPSTD >= 17
extern ::aall::LoggerClass server;
extern thread_local aall::MsgPublisher threadlogger;
#endif
#endif

namespace aall{
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
             typename string_type = std::string,
             bool timeEnabled = TIME_ENABLED,
             bool sendMsgs = MESSAGES_ENABLED
            >
   void log_(string_type const& msg,
            Tags t = Tags{},
            util::Context && c = util::Context{}){
            
      using namespace date;

        auto progname = aall::threadlogger.
      auto& threadid = aall::threadlogger.threadID;
      auto numTags = t.size();
      
      string_type datestr;
      string_type tagstr;

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
      using x = std::string::value_type;
      auto fmtmsg = fmt::format<string_type>("[{}] (({})) {}:{}:{} <Tags: {}> [{}]: {} \n",
                                datestr,
                                progname,
                                c.file,
                                c.func,
                                c.line ? c.line : 0,
                                tagstr,
                                util::getLogLevelstring(lvl),
                                msg);

    std::cout << fmtmsg;

    aall::threadlogger.send(zmq::buffer(fmtmsg), zmq::send_flags::dontwait);
    
   }
}
}

int main(){
    aall::logging::log_("hello");
     return 0;
}