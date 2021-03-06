#ifndef LLARP_DHT_MESSAGES_PUB_INTRO_HPP
#define LLARP_DHT_MESSAGES_PUB_INTRO_HPP
#include <llarp/dht/message.hpp>
#include <llarp/service/IntroSet.hpp>
#include <vector>

namespace llarp
{
  namespace dht
  {
    struct PublishIntroMessage : public IMessage
    {
      llarp::service::IntroSet I;
      std::vector< Key_t > E;
      uint64_t R    = 0;
      uint64_t S    = 0;
      uint64_t txID = 0;
      bool hasS     = false;
      PublishIntroMessage() : IMessage({})
      {
      }

      PublishIntroMessage(const llarp::service::IntroSet& i, uint64_t tx,
                          uint64_t s, const std::vector< Key_t >& exclude = {})
          : IMessage({}), E(exclude), txID(tx)
      {
        I = i;
        S = s;
      }

      ~PublishIntroMessage();

      bool
      BEncode(llarp_buffer_t* buf) const;

      bool
      DecodeKey(llarp_buffer_t key, llarp_buffer_t* val);

      virtual bool
      HandleMessage(llarp_dht_context* ctx,
                    std::vector< std::unique_ptr< IMessage > >& replies) const;
    };
  }  // namespace dht
}  // namespace llarp
#endif
