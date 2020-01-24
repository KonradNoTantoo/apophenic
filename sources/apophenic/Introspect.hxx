#ifndef AP_INTROSPECT_HXX
#define AP_INTROSPECT_HXX

#include <type_traits>


namespace ap
{
namespace insp
{



struct Error {};

struct BadRank : Error {};
struct BadName : Error {};
struct BadType : Error {};



template <class Implementor, unsigned rank, typename MemberType>
struct NamedMember
{
	MemberType _value;
	static const char * const kNAME;
};



template <typename StorageType>
using member_arg = std::conditional<
		std::is_scalar<StorageType>::value
	,	typename std::decay<StorageType>::type
	,	const StorageType &
	>;

template <typename StorageType>
using member_access = std::conditional<
		std::is_array<StorageType>::value
	,	typename std::decay<StorageType>::type
	,	StorageType &
	>;

template <typename StorageType>
using member_read = std::conditional<
		std::is_scalar<StorageType>::value
	,	typename std::decay<StorageType>::type
	,	typename member_access< typename std::add_const<StorageType>::type >::type
	>;

template <typename This>
using is_this_const = std::is_const< typename std::remove_pointer<This>::type >;

template <typename This, typename StorageType>
using get_result = std::conditional<
        is_this_const<This>::value
    ,   typename member_read<StorageType>::type
    ,   typename member_access<StorageType>::type
    >;



template <class Implementor, typename LastMemberType, typename... PreviousTypes>
class Introspected : public Introspected<Implementor, PreviousTypes...>
{
public:
	typedef Introspected<Implementor, PreviousTypes...> Parent;
	static constexpr size_t RANK = sizeof...(PreviousTypes);
	typedef NamedMember<Implementor, RANK, LastMemberType> MemberWrapper;

	Introspected() {}

	Introspected(typename member_arg<PreviousTypes>::type... next_values)
		: Parent(next_values...)
		, _wrapper{0} {}

	Introspected(
			typename member_arg<LastMemberType>::type value
		,	typename member_arg<PreviousTypes>::type... next_values
		)
		: Parent(next_values...)
		, _wrapper{value} {}

	Introspected(typename std::add_rvalue_reference<PreviousTypes>::type... next_values)
		: Parent(next_values...) {}

	Introspected(
			typename std::add_rvalue_reference<LastMemberType>::type value
		,	typename std::add_rvalue_reference<PreviousTypes>::type... next_values
		)
		: Parent(next_values...)
		, _wrapper{value} {}

	template <typename OtherType>
	typename member_read<OtherType>::type get(const std::string & name) const { return _get_implementation<OtherType>( static_cast<const Introspected*>(this), name ); }
	template <typename OtherType>
	typename member_access<OtherType>::type get(const std::string & name) { return _get_implementation<OtherType>( static_cast<Introspected*>(this), name ); }

	template <typename OtherType>
	typename member_read<OtherType>::type get(unsigned rank) const { return _get_implementation<OtherType>( static_cast<const Introspected*>(this), rank ); }
	template <typename OtherType>
	typename member_access<OtherType>::type get(unsigned rank) { return _get_implementation<OtherType>( static_cast<Introspected*>(this), rank ); }

protected:
	static bool _check_key(const std::string & name) { return MemberWrapper::kNAME == name; }
	static bool _check_key(unsigned rank) { return RANK == rank; }

	template <typename OtherType, typename This, typename Key, typename std::enable_if< std::is_same<OtherType, LastMemberType>::value, int >::type = 0>
	static
	typename get_result<This,OtherType>::type _get_implementation(This * _this, Key key)
	{
		if ( _check_key(key) )
		{
			return _this->_wrapper._value;
		}

		return static_cast< typename std::conditional<is_this_const<This>::value, const Parent*, Parent*>::type >(_this)->template get<OtherType>(key);
	}

	template <typename OtherType, typename This, typename Key, typename std::enable_if< ! std::is_same<OtherType, LastMemberType>::value, int >::type = 0>
	static
	typename get_result<This,OtherType>::type _get_implementation(This * _this, Key key)
	{
		if ( _check_key(key) )
		{
			throw BadType{};
		}

		return static_cast< typename std::conditional<is_this_const<This>::value, const Parent*, Parent*>::type >(_this)->template get<OtherType>(key);
	}

private:
	MemberWrapper _wrapper;
};



template <class Implementor, typename MemberType>
class Introspected<Implementor, MemberType>
{
public:
	typedef NamedMember<Implementor, 0, MemberType> MemberWrapper; 

	Introspected() : _wrapper{0} {}

	Introspected(typename member_arg<MemberType>::type value) : _wrapper{value} {}

	Introspected(typename std::add_rvalue_reference<MemberType>::type value) : _wrapper{value} {}

	template <typename OtherType>
	typename member_read<OtherType>::type get(const std::string & name) const { return _get_implementation<OtherType>( static_cast<const Introspected*>(this), name ); }
	template <typename OtherType>
	typename member_access<OtherType>::type get(const std::string & name) { return _get_implementation<OtherType>( static_cast<Introspected*>(this), name ); }

	template <typename OtherType>
	typename member_read<OtherType>::type get(unsigned rank) const { return _get_implementation<OtherType>( static_cast<const Introspected*>(this), rank ); }
	template <typename OtherType>
	typename member_access<OtherType>::type get(unsigned rank) { return _get_implementation<OtherType>( static_cast<Introspected*>(this), rank ); }

protected:
	template <typename OtherType, typename This, typename std::enable_if< std::is_same<OtherType, MemberType>::value, int >::type = 0>
	static
	typename get_result<This,OtherType>::type _get_implementation(This * _this, const std::string & name)
	{
		if ( MemberWrapper::kNAME == name )
		{
			return _this->_wrapper._value;
		}

		throw BadName{};
	}

	template <typename OtherType, typename This, typename std::enable_if< ! std::is_same<OtherType, MemberType>::value, int >::type = 0>
	static
	typename get_result<This,OtherType>::type _get_implementation(This * _this, const std::string & name)
	{
		if ( MemberWrapper::kNAME == name )
		{
			throw BadType{};
		}

		throw BadName{};
	}

	template <typename OtherType, typename This, typename std::enable_if< std::is_same<OtherType, MemberType>::value, int >::type = 0>
	static
	typename get_result<This,OtherType>::type _get_implementation(This * _this, unsigned rank)
	{
		if ( 0 == rank  )
		{
			return _this->_wrapper._value;
		}

		throw BadRank{};
	}

	template <typename OtherType, typename This, typename std::enable_if< ! std::is_same<OtherType, MemberType>::value, int >::type = 0>
	static
	typename get_result<This,OtherType>::type _get_implementation(This * _this, unsigned rank)
	{
		if ( 0 == rank  )
		{
			throw BadType{};
		}

		throw BadRank{};
	}

private:
	MemberWrapper _wrapper;
};



} // namespace insp
} // namespace ap

#endif // AP_INTROSPECT_HXX
