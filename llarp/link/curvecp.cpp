#include <llarp/link/curvecp.hpp>
#include "router.hpp"
#include <llarp/messages/link_intro.hpp>

namespace llarp
{
  namespace curvecp
  {
    std::unique_ptr< ILinkLayer >
    NewServer(llarp_router* r)
    {
      return nullptr;
    }
  }  // namespace curvecp

}  // namespace llarp
