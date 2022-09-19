#pragma once

#include <string>
#include <type_traits>


namespace ap
{
namespace insp
{



struct Error {};

struct EBadRank : Error {};
struct EBadName : Error {};
struct EBadType : Error {};



template< typename StorageType >
using member_arg = ::std::conditional<
		::std::is_scalar<StorageType>::value
	,	typename ::std::decay<StorageType>::type
	,	StorageType const &
	>;

template< typename StorageType >
using member_access = ::std::conditional<
		::std::is_array<StorageType>::value
	,	typename ::std::decay<StorageType>::type
	,	StorageType &
	>;

template< typename StorageType >
using member_read = ::std::conditional<
		::std::is_scalar<StorageType>::value
	,	typename ::std::decay<StorageType>::type
	,	typename member_access< typename ::std::add_const<StorageType>::type >::type
	>;

template< typename This >
using is_this_const = ::std::is_const< typename ::std::remove_pointer<This>::type >;

template< typename This, typename StorageType >
using get_result = ::std::conditional<
        is_this_const<This>::value
    ,   typename member_read<StorageType>::type
    ,   typename member_access<StorageType>::type
    >;



template< auto ptr_to_member >
struct Member {};



template< typename MemberType, class Base, MemberType Base::*ptr_to_member >
struct Member< ptr_to_member >
{
	using type = MemberType;
	static char const * const kNAME;

	template< class T, class Implementor >
	static typename member_read< MemberType >::type get( T const * t ) { return static_cast< Implementor const * >(t)->*ptr_to_member; }

	template< class T, class Implementor >
	static typename member_access< MemberType >::type get( T * t ) { return static_cast< Implementor * >(t)->*ptr_to_member; }
};



template< unsigned RANK, class Implementor, typename Member, typename... NextMembers >
class RankedIntrospector : public RankedIntrospector< RANK+1, Implementor, NextMembers... >
{
public:
	using MemberType = typename Member::type;
	using Parent = RankedIntrospector< RANK+1, Implementor, NextMembers... >;
	using Introspector = RankedIntrospector< RANK, Implementor, Member, NextMembers... >;


	Introspector & introspector() { return *this; }
	Introspector const & introspector() const { return *this; }
	Parent & parent_introspector() { return *this; }
	Parent const & parent_introspector() const { return *this; }


	typename member_access<MemberType>::type front_member() { return Member::template get< RankedIntrospector, Implementor >( this ); }
	typename member_read<MemberType>::type front_member() const { return Member::template get< RankedIntrospector, Implementor >( this ); }


	static char const * front_member_name() { return Member::kNAME; }


	template< typename OtherType >
	typename member_read<OtherType>::type get( ::std::string const & name ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector const * >(this), name ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( ::std::string const & name )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector * >(this), name ); }

	template< typename OtherType >
	typename member_read<OtherType>::type get( unsigned rank ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector const * >(this), rank ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( unsigned rank )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector * >(this), rank ); }


	template< unsigned GET_RANK, typename OtherType >
	typename member_read<OtherType>::type get() const
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospector const * >(this) ); }

	template< unsigned GET_RANK, typename OtherType >
	typename member_access<OtherType>::type get()
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospector * >(this) ); }


	static ::std::size_t nb_members() { return sizeof...( NextMembers ) + 1; }


	static bool has_member( ::std::string const & name )
	{
		return _check_key( name )
			? true
			: Parent::has_member( name );
	}


	static char const * member_name( unsigned rank )
	{
		return _check_key( rank )
			? Member::kNAME
			: Parent::member_name( rank );
	}


	::std::type_info const & member_type( ::std::string const & name ) const
	{
		return _check_key( name )
			? typeid( MemberType )
			: _down_cast<>(this)->member_type( name );
	}


	::std::type_info const & member_type( unsigned rank ) const
	{
		return _check_key( rank )
			? typeid( MemberType )
			: _down_cast<>(this)->member_type( rank );
	}


protected:
	static bool _check_key( ::std::string const & name ) { return Member::kNAME == name; }
	static bool _check_key( unsigned rank ) { return RANK == rank; }


	template< typename This >
	using  DownCastType =
		typename ::std::conditional<is_this_const<This>::value, Parent const *, Parent *>::type;


	template< typename This >
	static auto _down_cast( This * _this ) -> DownCastType<This>
	{
		return static_cast< DownCastType<This> >( _this );
	}


	template<
			typename OtherType
		,	typename This
		,	typename Key
		,	typename ::std::enable_if< ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, Key key )
	{
		return _check_key(key)
			? Member::template get< This, Implementor >( _this )
			: _down_cast<This>(_this)->template get<OtherType>(key);
	}


	template<
			typename OtherType
		,	typename This
		,	typename Key
		,	typename ::std::enable_if< ! ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, Key key )
	{
		if ( _check_key(key) ) throw EBadType{};
		return _down_cast<This>(_this)->template get<OtherType>(key);
	}


	template<
			unsigned GET_RANK
		,	typename OtherType
		,	typename This
		,	typename ::std::enable_if<GET_RANK == RANK, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _sta_get( This * _this )
	{
		static_assert( ::std::is_same<OtherType, MemberType>::value, "Bad type" );
		return Member::template get< This, Implementor >( _this );
	}


	template<
			unsigned GET_RANK
		,	typename OtherType
		,	typename This
		,	typename ::std::enable_if<GET_RANK != RANK, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _sta_get( This * _this )
	{
		return _down_cast<This>(_this)->template get<GET_RANK, OtherType>();
	}
};



template< unsigned RANK, class Implementor, typename Member >
class RankedIntrospector< RANK, Implementor, Member >
{
public:
	using MemberType = typename Member::type;
	using Introspector = RankedIntrospector< RANK, Implementor, Member >;


	Introspector & introspector() { return *this; }
	Introspector const & introspector() const { return *this; }


	typename member_access<MemberType>::type front_member() { return Member::template get< RankedIntrospector, Implementor >( this ); }
	typename member_read<MemberType>::type front_member() const { return Member::template get< RankedIntrospector, Implementor >( this ); }


	static char const * front_member_name() { return Member::kNAME; }


	template< typename OtherType >
	typename member_read<OtherType>::type get( ::std::string const & name ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector const * >(this), name ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( ::std::string const & name )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector * >(this), name ); }

	template< typename OtherType >
	typename member_read<OtherType>::type get( unsigned rank ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector const * >(this), rank ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( unsigned rank )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospector * >(this), rank ); }


	template< unsigned GET_RANK, typename OtherType >
	typename member_read<OtherType>::type get() const
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospector const * >(this) ); }

	template< unsigned GET_RANK, typename OtherType >
	typename member_access<OtherType>::type get()
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospector * >(this) ); }


	static ::std::size_t nb_keys() { return 1; }


	static bool has_member( ::std::string const & name )
	{
		return Member::kNAME == name;
	}


	static char const * member_name( unsigned rank )
	{
		if ( RANK == rank ) return Member::kNAME;
		throw EBadRank{};
	}


	::std::type_info const & member_type( ::std::string const & name ) const
	{
		if ( Member::kNAME == name ) return typeid( MemberType );
		throw EBadName{};
	}


	::std::type_info const & member_type( unsigned rank ) const
	{
		if ( RANK == rank ) return typeid( MemberType );
		throw EBadRank{};
	}


protected:
	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, const ::std::string & name )
	{
		if ( Member::kNAME == name ) return Member::template get< This, Implementor >( _this );
		throw EBadName{};
	}


	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ! ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, const ::std::string & name )
	{
		if ( Member::kNAME == name ) throw EBadType{};
		throw EBadName{};
	}


	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, unsigned rank )
	{
		if ( RANK == rank ) return Member::template get< This, Implementor >( _this );
		throw EBadRank{};
	}


	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ! ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, unsigned rank )
	{
		if ( RANK == rank ) throw EBadType{};
		throw EBadRank{};
	}


	template<
			unsigned GET_RANK
		,	typename OtherType
		,	typename This
		>
	static
	typename get_result<This,OtherType>::type _sta_get( This * _this )
	{
		static_assert( GET_RANK == RANK, "Bad rank" );
		static_assert( ::std::is_same<OtherType, MemberType>::value, "Bad type" );
		return Member::template get< This, Implementor >( _this );
	}
};



template< class Implementor, typename... Members >
using Introspector = RankedIntrospector< 0, Implementor, Members... >;



} // namespace insp
} // namespace ap
