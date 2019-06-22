#pragma once
#include <chainbase/object.hpp>

namespace chainbase{

template< typename value_type >
class undo_state
{
    public:
        typedef typename value_type::id_type                      id_type;
        typedef allocator< std::pair<const id_type, value_type> > id_value_allocator_type;
        typedef allocator< id_type >                              id_allocator_type;

        template<typename T>
        undo_state( allocator<T> al )
        :old_values( id_value_allocator_type( al.get_segment_manager() ) ),
        removed_values( id_value_allocator_type( al.get_segment_manager() ) ),
        new_ids( id_allocator_type( al.get_segment_manager() ) ){}

        typedef boost::interprocess::map< id_type, value_type, std::less<id_type>, id_value_allocator_type >  id_value_type_map;
        typedef boost::interprocess::set< id_type, std::less<id_type>, id_allocator_type >                    id_type_set;

        id_value_type_map            old_values;
        id_value_type_map            removed_values;
        id_type_set                  new_ids;
        id_type                      old_next_id = 0;
        int64_t                      revision = 0;
};

}