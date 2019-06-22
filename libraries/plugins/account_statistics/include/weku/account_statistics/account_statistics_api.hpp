#pragma once

#include <weku/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace weku{ namespace app {
   struct api_context;
} }

namespace weku { namespace account_statistics {

namespace detail
{
   class account_statistics_api_impl;
}

class account_statistics_api
{
   public:
      account_statistics_api( const weku::app::api_context& ctx );

      void on_api_startup();

   private:
      std::shared_ptr< detail::account_statistics_api_impl > _my;
};

} } // weku::account_statistics

FC_API( weku::account_statistics::account_statistics_api, )