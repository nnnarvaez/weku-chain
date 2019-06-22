#pragma once

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/chrono.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/throw_exception.hpp>

#include <array>
#include <atomic>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>

#include <chainbase/object.hpp>
#include <chainbase/undo_state.hpp>
#include <chainbase/generic_index.hpp>
#include <chainbase/mutex_manager.hpp>

#ifdef CHAINBASE_CHECK_LOCKING
   #define CHAINBASE_REQUIRE_READ_LOCK(m, t) require_read_lock(m, typeid(t).name())
   #define CHAINBASE_REQUIRE_WRITE_LOCK(m, t) require_write_lock(m, typeid(t).name())
#else
   #define CHAINBASE_REQUIRE_READ_LOCK(m, t)
   #define CHAINBASE_REQUIRE_WRITE_LOCK(m, t)
#endif

namespace chainbase {

   class abstract_session {
      public:
         virtual ~abstract_session(){};
         virtual void push()             = 0;
         virtual void squash()           = 0;
         virtual void undo()             = 0;
         virtual int64_t revision()const  = 0;
   };

   template<typename SessionType>
   class session_impl : public abstract_session
   {
      public:
         session_impl( SessionType&& s ):_session( std::move( s ) ){}

         virtual void push() override  { _session.push();  }
         virtual void squash() override{ _session.squash(); }
         virtual void undo() override  { _session.undo();  }
         virtual int64_t revision()const override  { return _session.revision();  }
      private:
         SessionType _session;
   };

   class index_extension
   {
      public:
         index_extension() {}
         virtual ~index_extension() {}
   };

   typedef std::vector< std::shared_ptr< index_extension > > index_extensions;

   class abstract_index
   {
      public:
         abstract_index( void* i ):_idx_ptr(i){}
         virtual ~abstract_index(){}
         virtual void     set_revision( int64_t revision ) = 0;
         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) = 0;

         virtual int64_t revision()const = 0;
         virtual void    undo()const = 0;
         virtual void    squash()const = 0;
         virtual void    commit( int64_t revision )const = 0;
         virtual void    undo_all()const = 0;
         virtual uint32_t type_id()const  = 0;

         virtual void remove_object( int64_t id ) = 0;

         void add_index_extension( std::shared_ptr< index_extension > ext )  { _extensions.push_back( ext ); }
         const index_extensions& get_index_extensions()const  { return _extensions; }
         void* get()const { return _idx_ptr; }
      private:
         void*              _idx_ptr;
         index_extensions   _extensions;
   };

   // index_impl is a simple wrapper for generic_index
   template<typename BaseIndex>
   class index_impl : public abstract_index {
      public:
         index_impl( BaseIndex& base ):abstract_index( &base ),_base(base){}

         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) override {
            return unique_ptr<abstract_session>(new session_impl<typename BaseIndex::session>( _base.start_undo_session( enabled ) ) );
         }

         virtual void     set_revision( int64_t revision ) override { _base.set_revision( revision ); }
         virtual int64_t  revision()const  override { return _base.revision(); }
         virtual void     undo()const  override { _base.undo(); }
         virtual void     squash()const  override { _base.squash(); }
         virtual void     commit( int64_t revision )const  override { _base.commit(revision); }
         virtual void     undo_all() const override {_base.undo_all(); }
         virtual uint32_t type_id()const override { return BaseIndex::value_type::type_id; }

         virtual void     remove_object( int64_t id ) override { return _base.remove_object( id ); }
      private:
         BaseIndex& _base;
   };

   template<typename IndexType>
   class index : public index_impl<IndexType> {
      public:
         index( IndexType& i ):index_impl<IndexType>( i ){}
   };
   
   /**
    *  This class
    */
   class database
   {
      public:
         enum open_flags {
            read_only     = 0,
            read_write    = 1
         };

         void open( const bfs::path& dir, uint32_t write = read_only, uint64_t shared_file_size = 0 );
         void close();
         void flush();
         void wipe( const bfs::path& dir );
         void set_require_locking( bool enable_require_locking );

         #ifdef CHAINBASE_CHECK_LOCKING
         void require_lock_fail( const char* method, const char* lock_type, const char* tname )const;

         // some functions must already have a lock as pre-execute condition.
         // so this is to make sure that pre-conditon is satisfiled.
         // every time a lock is acquired, the lock will incremented.
         // every time a lock is released, the lock will decremented.
         void require_read_lock( const char* method, const char* tname )const
         {
            if( BOOST_UNLIKELY( _enable_require_locking & _read_only & (_read_lock_count <= 0) ) )
               require_lock_fail(method, "read", tname);
         }

         void require_write_lock( const char* method, const char* tname )
         {
            if( BOOST_UNLIKELY( _enable_require_locking & (_write_lock_count <= 0) ) )
               require_lock_fail(method, "write", tname);
         }
         #endif

        // embeded session in database, it's different with session embedded in generic_index.
        // this session is "database session", it's a wrapper/container of all generic_index(table) sessions.
         struct session {
            public:
               session( session&& s ):_index_sessions( std::move(s._index_sessions) ),_revision( s._revision ){}
               session( vector<std::unique_ptr<abstract_session>>&& s ):_index_sessions( std::move(s) )
               {
                  if( _index_sessions.size() )
                     _revision = _index_sessions[0]->revision();
               }

               ~session() {
                  undo();
               }

               void push()
               {
                  for( auto& i : _index_sessions ) i->push();
                  _index_sessions.clear();
               }

               void squash()
               {
                  for( auto& i : _index_sessions ) i->squash();
                  _index_sessions.clear();
               }

               void undo()
               {
                  for( auto& i : _index_sessions ) i->undo();
                  _index_sessions.clear();
               }

               int64_t revision()const { return _revision; }

            private:
               friend class database;
               session(){}

               vector< std::unique_ptr<abstract_session> > _index_sessions;
               int64_t _revision = -1;
         };

         session start_undo_session( bool enabled );

         int64_t revision()const { // all indices/tables in the database have same reivsion number.
             if( _index_list.size() == 0 ) return -1;
             return _index_list[0]->revision();
         }

         void undo();
         void squash();
         void commit( int64_t revision );
         void undo_all();
         
         void set_revision( int64_t revision )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK( "set_revision", int64_t );
             for( auto i : _index_list ) i->set_revision( revision );
         }

        // add generic_index<T> via abstract_index
         template<typename MultiIndexType>
         void add_index() {
             const uint16_t type_id = generic_index<MultiIndexType>::value_type::type_id;
             typedef generic_index<MultiIndexType>          index_type;
             typedef typename index_type::allocator_type    index_alloc;

             std::string type_name = boost::core::demangle( typeid( typename index_type::value_type ).name() );

             if( !( _index_map.size() <= type_id || _index_map[ type_id ] == nullptr ) ) {
                BOOST_THROW_EXCEPTION( std::logic_error( type_name + "::type_id is already in use" ) );
             }

             index_type* idx_ptr =  nullptr;
             if( !_read_only ) {
                idx_ptr = _segment->find_or_construct< index_type >( type_name.c_str() )( index_alloc( _segment->get_segment_manager() ) );
             } else {
                idx_ptr = _segment->find< index_type >( type_name.c_str() ).first;
                if( !idx_ptr ) BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find index for " + type_name + " in read only database" ) );
             }

             idx_ptr->validate();

             if( type_id >= _index_map.size() )
                _index_map.resize( type_id + 1 );

             auto new_index = new index<index_type>( *idx_ptr );
             _index_map[ type_id ].reset( new_index );
             _index_list.push_back( new_index );
         }

         auto get_segment_manager() -> decltype( ((bip::managed_mapped_file*)nullptr)->get_segment_manager()) {
            return _segment->get_segment_manager();
         }

         size_t get_free_memory()const
         {
            return _segment->get_segment_manager()->get_free_memory();
         }

         template<typename MultiIndexType>
         bool has_index()const
         {
            CHAINBASE_REQUIRE_READ_LOCK("get_index", typename MultiIndexType::value_type);
            typedef generic_index<MultiIndexType> index_type;
            return _index_map.size() > index_type::value_type::type_id && _index_map[index_type::value_type::type_id];
         }

         template<typename MultiIndexType>
         const generic_index<MultiIndexType>& get_index()const
         {
            CHAINBASE_REQUIRE_READ_LOCK("get_index", typename MultiIndexType::value_type);
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;

            if( !has_index< MultiIndexType >() )
            {
               std::string type_name = boost::core::demangle( typeid( typename index_type::value_type ).name() );
               BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find index for " + type_name + " in database" ) );
            }

            return *index_type_ptr( _index_map[index_type::value_type::type_id]->get() );
         }

         template<typename MultiIndexType>
         void add_index_extension( std::shared_ptr< index_extension > ext )
         {
            typedef generic_index<MultiIndexType> index_type;

            if( !has_index< MultiIndexType >() )
            {
               std::string type_name = boost::core::demangle( typeid( typename index_type::value_type ).name() );
               BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find index for " + type_name + " in database" ) );
            }

            _index_map[index_type::value_type::type_id]->add_index_extension( ext );
         }

         template<typename MultiIndexType, typename ByIndex>
         auto get_index()const -> decltype( ((generic_index<MultiIndexType>*)( nullptr ))->indicies().template get<ByIndex>() )
         {
            CHAINBASE_REQUIRE_READ_LOCK("get_index", typename MultiIndexType::value_type);
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;

            if( !has_index< MultiIndexType >() )
            {
               std::string type_name = boost::core::demangle( typeid( typename index_type::value_type ).name() );
               BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find index for " + type_name + " in database" ) );
            }

            return index_type_ptr( _index_map[index_type::value_type::type_id]->get() )->indicies().template get<ByIndex>();
         }

         // the difference with get_index() is that, this one has a write lock, and the other one has a read lock.
         template<typename MultiIndexType>
         generic_index<MultiIndexType>& get_mutable_index()
         {
            CHAINBASE_REQUIRE_WRITE_LOCK("get_mutable_index", typename MultiIndexType::value_type);
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;

            if( !has_index< MultiIndexType >() )
            {
               std::string type_name = boost::core::demangle( typeid( typename index_type::value_type ).name() );
               BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find index for " + type_name + " in database" ) );
            }

            return *index_type_ptr( _index_map[index_type::value_type::type_id]->get() );
         }

         template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
         const ObjectType* find( CompatibleKey&& key )const
         {
             CHAINBASE_REQUIRE_READ_LOCK("find", ObjectType);
             typedef typename get_index_type< ObjectType >::type index_type;
             const auto& idx = get_index< index_type >().indicies().template get< IndexedByType >();
             auto itr = idx.find( std::forward< CompatibleKey >( key ) );
             if( itr == idx.end() ) return nullptr;
             return &*itr;
         }

         template< typename ObjectType >
         const ObjectType* find( oid< ObjectType > key = oid< ObjectType >() ) const
         {
             CHAINBASE_REQUIRE_READ_LOCK("find", ObjectType);
             typedef typename get_index_type< ObjectType >::type index_type;
             const auto& idx = get_index< index_type >().indices();
             auto itr = idx.find( key );
             if( itr == idx.end() ) return nullptr;
             return &*itr;
         }

         template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
         const ObjectType& get( CompatibleKey&& key )const
         {
             CHAINBASE_REQUIRE_READ_LOCK("get", ObjectType);
             auto obj = find< ObjectType, IndexedByType >( std::forward< CompatibleKey >( key ) );
             if( !obj ) BOOST_THROW_EXCEPTION( std::out_of_range( "unknown key" ) );
             return *obj;
         }

         template< typename ObjectType >
         const ObjectType& get( const oid< ObjectType >& key = oid< ObjectType >() )const
         {
             CHAINBASE_REQUIRE_READ_LOCK("get", ObjectType);
             auto obj = find< ObjectType >( key );
             if( !obj ) BOOST_THROW_EXCEPTION( std::out_of_range( "unknown key") );
             return *obj;
         }

         template<typename ObjectType, typename Modifier>
         void modify( const ObjectType& obj, Modifier&& m )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK("modify", ObjectType);
             typedef typename get_index_type<ObjectType>::type index_type;
             get_mutable_index<index_type>().modify( obj, m );
         }

         template<typename ObjectType>
         void remove( const ObjectType& obj )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK("remove", ObjectType);
             typedef typename get_index_type<ObjectType>::type index_type;
             return get_mutable_index<index_type>().remove( obj );
         }

         template<typename ObjectType, typename Constructor>
         const ObjectType& create( Constructor&& con )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK("create", ObjectType);
             typedef typename get_index_type<ObjectType>::type index_type;
             // call generic_index<T>::emplace, then call underline multiindex container emplace.
             return get_mutable_index<index_type>().emplace( std::forward<Constructor>(con) );
         }

         template< typename Lambda >
         auto with_read_lock( Lambda&& callback, uint64_t wait_micro = 1000000 ) -> decltype( (*(Lambda*)nullptr)() )
         {
            read_lock lock( _rw_manager->current_lock(), bip::defer_lock_type() );
            #ifdef CHAINBASE_CHECK_LOCKING
            BOOST_ATTRIBUTE_UNUSED
            int_incrementer ii( _read_lock_count );
            #endif

            if( !wait_micro )
            {
               lock.lock();
            }
            else
            {
               if( !lock.timed_lock( boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds( wait_micro ) ) )
                  BOOST_THROW_EXCEPTION( std::runtime_error( "unable to acquire lock" ) );
            }

            return callback();
         }

         template< typename Lambda >
         auto with_write_lock( Lambda&& callback, uint64_t wait_micro = 1000000 ) -> decltype( (*(Lambda*)nullptr)() )
         {
            if( _read_only )
               BOOST_THROW_EXCEPTION( std::logic_error( "cannot acquire write lock on read-only process" ) );

            write_lock lock( _rw_manager->current_lock(), boost::defer_lock_t() );
            #ifdef CHAINBASE_CHECK_LOCKING
            BOOST_ATTRIBUTE_UNUSED
            int_incrementer ii( _write_lock_count );
            #endif

            if( !wait_micro )
            {
               lock.lock();
            }
            else
            {
               while( !lock.timed_lock( boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds( wait_micro ) ) )
               {
                  _rw_manager->next_lock();
                  std::cerr << "Lock timeout, moving to lock " << _rw_manager->current_lock_num() << std::endl;
                  lock = write_lock( _rw_manager->current_lock(), boost::defer_lock_t() );
               }
            }

            return callback();
         }

         template< typename IndexExtensionType, typename Lambda >
         void for_each_index_extension( Lambda&& callback )const
         {
            for( const abstract_index* idx : _index_list )
            {
               const index_extensions& exts = idx->get_index_extensions();
               for( const std::shared_ptr< index_extension >& e : exts )
               {
                  std::shared_ptr< IndexExtensionType > e2 = std::dynamic_pointer_cast< IndexExtensionType >( e );
                  if( e2 )
                     callback( e2 );
               }
            }
         }

      private:
         // represents file: shared_memory.bin in memory, which contains environment at first, then all other memory objects.
         unique_ptr<bip::managed_mapped_file>                        _segment;
         // represents file: shared_memory.meta in memory, which only contains one object: read_write_mutex_manager
         unique_ptr<bip::managed_mapped_file>                        _meta;
         read_write_mutex_manager*                                   _rw_manager = nullptr;
         bool                                                        _read_only = false;
         // use to try lock file: shared_memory.meta, to prevent acess from all other processes (not threads).
         // its a exclusive lock, to prevent all other processes (not threads) to access.
         bip::file_lock                                              _flock;

         /**
          * This is a sparse list of known indicies kept to accelerate creation of undo sessions
          */
         vector<abstract_index*>                                     _index_list;

         /**
          * This is a full map (size 2^16) of all possible index designed for constant time lookup
          */
         vector<unique_ptr<abstract_index>>                          _index_map;

         bfs::path                                                   _data_dir;

         int32_t                                                     _read_lock_count = 0;
         int32_t                                                     _write_lock_count = 0;
         bool                                                        _enable_require_locking = false;
   };

   template<typename Object, typename... Args>
   using shared_multi_index_container = boost::multi_index_container<Object,Args..., chainbase::allocator<Object> >;
}  // namepsace chainbase

