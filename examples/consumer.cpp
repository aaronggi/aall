//#define AALL_PROGNAME "Consumer"

//#include <aall/log.hpp>
#include <zmq.hpp>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <unordered_set>
#include <unordered_map>
#include <thread>

//The best kind of debug
#if defined(__GNUC__) || defined(__clang__)
#define aall_likely(x)       __builtin_expect((x),1)
#define aall_unlikely(x)     __builtin_expect((x),0)
#else
#define aall_likely(x)       x
#define aall_unlikely(x)     x
#endif

//the best kind of debugging
#define ___l //fmt::print("{}\n", __LINE__);


//ignore these static vars for now, this will be more 
//idiomatic later
zmq::context_t zcontext{1};
std::thread serviceDiscovery{};
zmq::socket_t serviceSubSkt{zcontext, zmq::socket_type::sub};
std::unordered_set<std::string> subscribedServices{};
std::unordered_map<std::string, std::string> services{};

zmq::socket_t logRxMsgSkt{zcontext, zmq::socket_type::sub};

bool running = false;

//also ignore this; WIP
void staticInit(int argc, char* argv[]){
   services.reserve(32);

   serviceSubSkt.set(zmq::sockopt::subscribe, "AALL_CHANNEL");
   serviceSubSkt.bind("tcp://127.0.0.1:51227");

   //subscribe all if none specified or wildcard
   if(argc == 1 || std::string{argv[1]} == "*"){
      logRxMsgSkt.set(zmq::sockopt::subscribe, "");
   }
   else {
      logRxMsgSkt.set(zmq::sockopt::subscribe, argv[1]);
   }

   running = true;

   //service discovery thread
   serviceDiscovery = std::thread([]{
      zmq::message_t msg{};
      fmt::print("looking for services \n");
      while(running){

         //discard topic
         auto rx = serviceSubSkt.recv(msg);
         if(rx.has_value() == false) continue;
         
         //get service name
         rx = serviceSubSkt.recv(msg);
         if(rx.has_value() == false) throw std::logic_error("receive err");
         auto serviceName = msg.to_string();
         
         //get Addr
         rx = serviceSubSkt.recv(msg);
         if(rx.has_value() == false) throw std::logic_error("receive err");
         auto serviceAddr = msg.to_string();

                  
         //add service to our map
         auto it = services.find(serviceName);
         if(it == services.end()){
            fmt::print("Found Service {} @{}\n", serviceName, serviceAddr);

            services.insert(std::make_pair(std::move(serviceName),  
                                           serviceAddr));
            logRxMsgSkt.connect(serviceAddr);
         }
      }
   });
}



int main(int argc, char* argv[]) {

   using namespace std::chrono_literals;

   staticInit(argc, argv);

   std::unordered_set<std::string> tagsSet{32};
   std::vector<std::string> subscribedTags;
   
   std::string progname = argv[1];
   auto allProgs = false;
   
   //keep all subscribed tags
   for(int i = 2; i < argc; i++){
      if(argv[i] == nullptr){
         throw std::logic_error("ptr is null");
      }
      subscribedTags.push_back(argv[i]);
   }

//main loop
   while (true) {
   
      zmq::message_t msg{};
      auto rx = logRxMsgSkt.recv(msg);
      auto foundtag = false;

      //an empty msg acts as a delimiter for the end of tags
      //we loop until the message size is 0, while adding the 
      //message tags to the loop
      while(true){
         auto msgResult = logRxMsgSkt.recv(msg);
         if (aall_unlikely(msgResult.has_value() == false)){
            //TODO: obv better error reporting/handling
            throw std::logic_error("whoops"); 
         }
         if (msg.size() == 0) 
            break;
         auto rxTag = msg.to_string();
         tagsSet.insert(rxTag);
      }

      //Filter based on tags we want
      //no tags means ALL TAGS
      if(subscribedTags.size() == 0) {
         foundtag = true;
      }
      else {
         //check our subscribed tags to see if any of them
         //match up with the message's tags
         for(auto& sTag : subscribedTags){
            if(tagsSet.find(sTag) != tagsSet.end() || 
               sTag == "*") {
               foundtag = true;
            }  
         }
      }
      
      //The rest of the message (still WIP)
      auto rxPayload = logRxMsgSkt.recv(msg);
      
      if (foundtag ){
         fmt::print("{}", msg.to_string_view());
      }

      tagsSet.clear();
   }

   //cleanup
   running = false;
   serviceSubSkt.close();
   serviceDiscovery.join();
   zcontext.close();
   return 0;
}