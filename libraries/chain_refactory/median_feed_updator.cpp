#include <wk/chain_refactory/median_feed_updator.hpp>

using namespace steemit::chain;

namespace wk{namespace chain{

// only update median feed once every hour.
void median_feed_updator::update_median_feed() 
{
    try {
        if( (_db.head_block_num() % STEEMIT_FEED_INTERVAL_BLOCKS) != 0 ) return;

        auto now = _db.head_block_time();
        const witness_schedule_object& wso = _db.get_witness_schedule_object();
        std::vector<price> feeds; 
        feeds.reserve( wso.num_scheduled_witnesses );
        for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
        {
            const auto& wit = _db.get_witness( wso.current_shuffled_witnesses[i] );
            if( _db.has_hardfork( STEEMIT_HARDFORK_0_19 ) ) // TODO: hardfork can be cleaned
            {
                if( now < wit.last_sbd_exchange_update + STEEMIT_MAX_FEED_AGE_SECONDS
                    && !wit.sbd_exchange_rate.is_null() )
                {
                    feeds.push_back( wit.sbd_exchange_rate );
                }
            }
            else if( wit.last_sbd_exchange_update < now + STEEMIT_MAX_FEED_AGE_SECONDS &&
                !wit.sbd_exchange_rate.is_null() )
            {
                feeds.push_back( wit.sbd_exchange_rate );
            }
        }

        if( feeds.size() >= STEEMIT_MIN_FEEDS )
        {
            std::sort( feeds.begin(), feeds.end() );
            auto median_feed = feeds[feeds.size()/2];

            _db.modify( _db.get_feed_history(), [&]( feed_history_object& fho )
            {
                fho.price_history.push_back( median_feed );
                size_t steemit_feed_history_window = STEEMIT_FEED_HISTORY_WINDOW_PRE_HF_16;
                if( _db.has_hardfork( STEEMIT_HARDFORK_0_16) )
                    steemit_feed_history_window = STEEMIT_FEED_HISTORY_WINDOW;

                if( fho.price_history.size() > steemit_feed_history_window )
                    fho.price_history.pop_front();

                if( fho.price_history.size() )
                {
                    std::deque< price > copy;
                    for( auto i : fho.price_history )
                    {
                    copy.push_back( i );
                    }

                    std::sort( copy.begin(), copy.end() ); /// TODO: use nth_item
                    fho.current_median_history = copy[copy.size()/2];

                    #ifdef IS_TEST_NET
                    if( skip_price_feed_limit_check )
                    return;
                    #endif
                    if( _db.has_hardfork( STEEMIT_HARDFORK_0_14 ) )
                    {
                    const auto& gpo = _db.get_dynamic_global_properties();
                    price min_price( asset( 9 * gpo.current_sbd_supply.amount, SBD_SYMBOL ), gpo.current_supply ); // This price limits SBD to 10% market cap

                    if( min_price > fho.current_median_history )
                        fho.current_median_history = min_price;
                    }
                }
            });
        }
    } FC_CAPTURE_AND_RETHROW() }

}}