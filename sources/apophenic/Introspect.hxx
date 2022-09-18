#pragma once

#include <type_traits>


namespace ap
{
namespace insp
{



struct Error {};

struct BadRank : Error {};
struct BadName : Error {};
struct BadType : Error {};



template< class Implementor, unsigned rank, typename MemberType >
struct NamedMember
{
	MemberType _value;
	static const char * const kNAME;
};



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



template< unsigned RANK, class Implementor, typename CurrentMemberType, typename... NextTypes >
class RankedIntrospected : public RankedIntrospected<RANK+1, Implementor, NextTypes...>
{
public:
	typedef RankedIntrospected<RANK+1, Implementor, NextTypes...> Parent;
	typedef NamedMember<Implementor, RANK, CurrentMemberType> MemberWrapper;


	RankedIntrospected() {}

	RankedIntrospected(typename member_arg<NextTypes>::type... next_values)
		: Parent(next_values...)
		, _wrapper{0} {}

	RankedIntrospected(
			typename member_arg<CurrentMemberType>::type value
		,	typename member_arg<NextTypes>::type... next_values
		)
		: Parent(next_values...)
		, _wrapper{value} {}

	RankedIntrospected(typename ::std::add_rvalue_reference<NextTypes>::type... next_values)
		: Parent(next_values...) {}

	RankedIntrospected(
			typename ::std::add_rvalue_reference<CurrentMemberType>::type value
		,	typename ::std::add_rvalue_reference<NextTypes>::type... next_values
		)
		: Parent(next_values...)
		, _wrapper{value} {}


	template< typename OtherType >
	typename member_read<OtherType>::type get( ::std::string const & name ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected const * >(this), name ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( ::std::string const & name )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected * >(this), name ); }

	template< typename OtherType >
	typename member_read<OtherType>::type get( unsigned rank ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected const * >(this), rank ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( unsigned rank )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected * >(this), rank ); }

	template< unsigned GET_RANK, typename OtherType >
	typename member_read<OtherType>::type get() const
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospected const * >(this) ); }

	template< unsigned GET_RANK, typename OtherType >
	typename member_access<OtherType>::type get()
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospected * >(this) ); }


protected:
	static bool _check_key( ::std::string const & name ) { return MemberWrapper::kNAME == name; }
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
		,	typename ::std::enable_if< ::std::is_same<OtherType, CurrentMemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, Key key )
	{
		if ( _check_key(key) )
		{
			return _this->_wrapper._value;
		}

		return _down_cast<This>(_this)->template get<OtherType>(key);
	}


	template<
			typename OtherType
		,	typename This
		,	typename Key
		,	typename ::std::enable_if< ! ::std::is_same<OtherType, CurrentMemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, Key key )
	{
		if ( _check_key(key) )
		{
			throw BadType{};
		}

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
		static_assert( ::std::is_same<OtherType, CurrentMemberType>::value, "Bad type" );
		return _this->_wrapper._value;
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


private:
	MemberWrapper _wrapper;
};



template< unsigned RANK, class Implementor, typename MemberType >
class RankedIntrospected< RANK, Implementor, MemberType >
{
public:
	typedef NamedMember<Implementor, RANK, MemberType> MemberWrapper; 


	RankedIntrospected() : _wrapper{0} {}

	RankedIntrospected(typename member_arg<MemberType>::type value) : _wrapper{value} {}

	RankedIntrospected(typename ::std::add_rvalue_reference<MemberType>::type value) : _wrapper{value} {}


	template< typename OtherType >
	typename member_read<OtherType>::type get( ::std::string const & name ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected const * >(this), name ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( ::std::string const & name )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected * >(this), name ); }

	template< typename OtherType >
	typename member_read<OtherType>::type get( unsigned rank ) const
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected const * >(this), rank ); }

	template< typename OtherType >
	typename member_access<OtherType>::type get( unsigned rank )
		{ return _dyn_get<OtherType>( static_cast< RankedIntrospected * >(this), rank ); }

	template< unsigned GET_RANK, typename OtherType >
	typename member_read<OtherType>::type get() const
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospected const * >(this) ); }

	template< unsigned GET_RANK, typename OtherType >
	typename member_access<OtherType>::type get()
		{ return _sta_get<GET_RANK, OtherType>( static_cast< RankedIntrospected * >(this) ); }


protected:
	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, const ::std::string & name )
	{
		if ( MemberWrapper::kNAME == name )
		{
			return _this->_wrapper._value;
		}

		throw BadName{};
	}


	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ! ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, const ::std::string & name )
	{
		if ( MemberWrapper::kNAME == name )
		{
			throw BadType{};
		}

		throw BadName{};
	}


	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, unsigned rank )
	{
		if ( RANK == rank  )
		{
			return _this->_wrapper._value;
		}

		throw BadRank{};
	}


	template<
			typename OtherType
		,	typename This
		,	typename ::std::enable_if< ! ::std::is_same<OtherType, MemberType>::value, int >::type = 0
		>
	static
	typename get_result<This,OtherType>::type _dyn_get( This * _this, unsigned rank )
	{
		if ( RANK == rank  )
		{
			throw BadType{};
		}

		throw BadRank{};
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
		return _this->_wrapper._value;
	}


private:
	MemberWrapper _wrapper;
};



template< class Implementor, typename... MemberTypes >
using Introspected = RankedIntrospected< 0, Implementor, MemberTypes... >;



} // namespace insp
} // namespace ap
