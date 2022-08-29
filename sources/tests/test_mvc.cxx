#include <string>
#include <vector>
#include <list>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "apophenic/MVC.hxx"


using ::testing::_;
using ::testing::Ref;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::A;
using ::testing::An;


class Inputer;
class InputBridge;


constexpr unsigned int kCONNECTION_CONTEXT = 1325465319u;


struct Implementation1
{
	typedef Inputer inputer;
	typedef InputBridge input_bridge;
	typedef unsigned int connection_context;
	typedef std::string message;
	typedef std::string reply;
};


class Inputer : public ap::mvc::Inputer<Implementation1>
{
public:
	MOCK_METHOD0(accepted_cb, void());
	MOCK_METHOD0(rejected_cb, void());
	MOCK_METHOD1(mock_reply_cb, void(std::string));
	virtual void reply_cb(std::string && str) override { mock_reply_cb(str); }
};


class InputBridge : public ap::mvc::InputBridge<Implementation1>
{
	friend class ap::mvc::Hub<InputBridge>;
public:
	virtual ap::mvc::eInputStatus handle_input(Inputer & inputer, std::string && message) override
	{
		return mock_handle_input(inputer, message);
	}

	MOCK_METHOD2(is_connection_allowed, bool(const ap::mvc::Spoke<Inputer> &, Implementation1::connection_context));
	MOCK_METHOD2(enact_connection, void(ap::mvc::Spoke<Inputer> &, Implementation1::connection_context));
	MOCK_METHOD1(is_connected, bool(const ap::mvc::Spoke<Inputer> & ));
	MOCK_METHOD1(enact_disconnection, void(ap::mvc::Spoke<Inputer> & ));

	MOCK_METHOD2(mock_handle_input, ap::mvc::eInputStatus(Inputer&, std::string));
};


TEST(mvc, input_connect_and_disconnect)
{
	InputBridge ic;
	Inputer i;

	EXPECT_CALL(ic, is_connection_allowed(_, kCONNECTION_CONTEXT))
		.WillOnce(Return(true));
	EXPECT_CALL(ic, enact_connection(_, kCONNECTION_CONTEXT))
		.Times(1);

	i.connect(ic, kCONNECTION_CONTEXT);

	ASSERT_EQ(i.bridge(), &ic);

	EXPECT_CALL(ic, is_connected(_))
		.WillOnce(Return(true));
	EXPECT_CALL(ic, enact_disconnection(_))
		.Times(1);

	i.disconnect();

	ASSERT_TRUE(i.bridge() == nullptr);
}


class View;
class ViewBridge;


struct integer
{
	int _value;
};


struct Implementation2
{
	typedef View view;
	typedef ViewBridge view_bridge;
	typedef ViewBridge event_emiter;
	typedef unsigned int connection_context;

	template<typename T>
	struct type_data {};
};


template<>
struct Implementation2::type_data<std::string>
{
	typedef std::vector<std::string> init_data;
};


template<>
struct Implementation2::type_data<integer>
{
	typedef std::list<integer> init_data;
};


class View : public ap::mvc::View<Implementation2>
{
public:
	virtual ~View() {};
};


class ViewString
	: public View
	, public ap::mvc::Listener<Implementation2, std::string>
{
public:
	virtual void initialize(Implementation2::type_data<std::string>::init_data && data) override
	{
		mock_initialize(data);
	}

	MOCK_METHOD1(handle_event, void (const std::string & event));
	MOCK_METHOD1(mock_initialize, void (Implementation2::type_data<std::string>::init_data & data));
};


class ViewInt
	: public View
	, public ap::mvc::Listener<Implementation2, integer>
{
public:
	virtual void initialize(Implementation2::type_data<integer>::init_data && data) override
	{
		mock_initialize(data);
	}

	MOCK_METHOD1(handle_event, void (const integer & event));
	MOCK_METHOD1(mock_initialize, void (Implementation2::type_data<integer>::init_data & data));
};


class ViewAll
	: public View
	, public ap::mvc::Listener<Implementation2, integer, std::string>
{
public:
	virtual void initialize(Implementation2::type_data<integer>::init_data && data) override
	{
		mock_string_initialize(data);
	}

	virtual void initialize(Implementation2::type_data<std::string>::init_data && data) override
	{
		mock_int_initialize(data);
	}

	MOCK_METHOD1(handle_event, void (const integer & event));
	MOCK_METHOD1(mock_string_initialize, void (Implementation2::type_data<integer>::init_data & data));

	MOCK_METHOD1(handle_event, void (const std::string & event));
	MOCK_METHOD1(mock_int_initialize, void (Implementation2::type_data<std::string>::init_data & data));
};


class ViewBridge
	: public ap::mvc::ViewBridge<Implementation2>
	, public ap::mvc::Emiter<Implementation2, integer, std::string>
{
	friend class ap::mvc::Hub<ViewBridge>;
public:
	typedef ap::mvc::Listener<Implementation2, integer> IntListener;
	typedef Implementation2::type_data<integer>::init_data IntInitData;
	typedef ap::mvc::Listener<Implementation2, std::string> StringListener;
	typedef Implementation2::type_data<std::string>::init_data StringInitData;

	virtual StringInitData get_init_data(const StringListener & l) override
	{
		StringInitData tmp(mock_get_init_data(l));
		return std::move(tmp);
	}

	virtual IntInitData get_init_data(const IntListener & l) override
	{
		IntInitData tmp(mock_get_init_data(l));
		return std::move(tmp);
	}

	MOCK_METHOD2(is_connection_allowed, bool(const ap::mvc::Spoke<View> &, Implementation2::connection_context));
	MOCK_METHOD2(enact_connection, void(ap::mvc::Spoke<View> &, Implementation2::connection_context));
	MOCK_METHOD1(is_connected, bool(const ap::mvc::Spoke<View> & ));
	MOCK_METHOD1(enact_disconnection, void(ap::mvc::Spoke<View> & ));

	MOCK_METHOD1(on_registered_cb, void(StringListener & listener));
	MOCK_METHOD1(on_registered_cb, void(IntListener & listener));
	MOCK_METHOD1(mock_get_init_data, StringInitData & (const StringListener & ));
	MOCK_METHOD1(mock_get_init_data, IntInitData & (const IntListener & ));

	template<typename TTarget, typename TBase>
	const TTarget & cast(const TBase & base) const
	{
		throw std::bad_cast();
	}

	template<typename TEvent>
	std::list<ap::mvc::Listener<Implementation2,TEvent> *> & listeners() const;

	MOCK_CONST_METHOD0(mock_get_string_listeners, std::list<StringListener *> & ());
	MOCK_CONST_METHOD0(mock_get_int_listeners, std::list<IntListener *> & ());

	template<typename TEvent>
	bool filter(const TEvent &, ap::mvc::Listener<Implementation2,TEvent> *) const { return true; }
};


template<>
const integer & ViewBridge::cast<integer,integer>(const integer & base) const
{
	return base;
}


template<>
const std::string & ViewBridge::cast<std::string,std::string>(const std::string & base) const
{
	return base;
}


template<>
std::list<ap::mvc::Listener<Implementation2,std::string> *> & ViewBridge::listeners<std::string>() const
{
	return mock_get_string_listeners();
}


template<>
std::list<ap::mvc::Listener<Implementation2,integer> *> & ViewBridge::listeners<integer>() const
{
	return mock_get_int_listeners();
}


TEST(mvc, view_connect_and_disconnect)
{
	ViewBridge vc;
	ViewString v;

	Implementation2::type_data<std::string>::init_data empty_string_init_data;

	EXPECT_CALL(vc, is_connection_allowed(_, kCONNECTION_CONTEXT))
		.WillOnce(Return(true));
	EXPECT_CALL(vc, enact_connection(_, kCONNECTION_CONTEXT))
		.Times(1);
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::StringListener &>(Ref(v))))
		.WillOnce(ReturnRef(empty_string_init_data));
	EXPECT_CALL(v, mock_initialize(_))
		.Times(1);
	EXPECT_CALL(vc, on_registered_cb(An< ViewBridge::StringListener & >()))
		.Times(1);

	v.connect(vc, kCONNECTION_CONTEXT);

	ASSERT_EQ(v.bridge(), &vc);

	EXPECT_CALL(vc, is_connected(_))
		.WillOnce(Return(true));
	EXPECT_CALL(vc, enact_disconnection(_))
		.Times(1);

	v.disconnect();

	ASSERT_TRUE(v.bridge() == nullptr);
}


TEST(mvc, multiple_view_connect_and_disconnect)
{
	ViewBridge vc;
	ViewString vs;
	ViewInt vi;

	Implementation2::type_data<std::string>::init_data empty_string_init_data;
	Implementation2::type_data<integer>::init_data int_init_data;
	int_init_data.push_back(integer{56});

	EXPECT_CALL(vc, is_connection_allowed(_, kCONNECTION_CONTEXT))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_connection(_, kCONNECTION_CONTEXT))
		.Times(2);

	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::StringListener &>(Ref(vs))))
		.WillOnce(ReturnRef(empty_string_init_data));
	EXPECT_CALL(vs, mock_initialize(_))
		.Times(1);

	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::IntListener &>(Ref(vi))))
		.WillOnce(ReturnRef(int_init_data));
	EXPECT_CALL(vi, mock_initialize(_))
		.Times(1);

	EXPECT_CALL(vc, on_registered_cb(An< ViewBridge::IntListener & >()))
		.Times(1);

	EXPECT_CALL(vc, on_registered_cb(An< ViewBridge::StringListener & >()))
		.Times(1);

	vs.connect(vc, kCONNECTION_CONTEXT);
	vi.connect(vc, kCONNECTION_CONTEXT);

	ASSERT_EQ(vs.bridge(), &vc);
	ASSERT_EQ(vi.bridge(), &vc);

	EXPECT_CALL(vc, is_connected(_))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_disconnection(_))
		.Times(2);

	vi.disconnect();
	vs.disconnect();

	ASSERT_TRUE(vs.bridge() == nullptr);
	ASSERT_TRUE(vi.bridge() == nullptr);
}


TEST(mvc, propagate_event)
{
	ViewBridge vc;
	ViewString v;

	Implementation2::type_data<std::string>::init_data empty_string_init_data;

	EXPECT_CALL(vc, is_connection_allowed(_, kCONNECTION_CONTEXT))
		.WillOnce(Return(true));
	EXPECT_CALL(vc, enact_connection(_, kCONNECTION_CONTEXT))
		.Times(1);
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::StringListener &>(Ref(v))))
		.WillOnce(ReturnRef(empty_string_init_data));
	EXPECT_CALL(v, mock_initialize(_))
		.Times(1);

	v.connect(vc, kCONNECTION_CONTEXT);

	ASSERT_EQ(v.bridge(), &vc);

	std::list<ViewBridge::StringListener*> string_listeners;

	EXPECT_CALL(vc, mock_get_string_listeners())
		.WillOnce(ReturnRef(string_listeners));
	EXPECT_CALL(vc, mock_get_int_listeners())
		.Times(0);

	vc.propagate_generic_event(std::string("hello!"));

	string_listeners.push_back(&v);

	EXPECT_CALL(vc, mock_get_string_listeners())
		.WillOnce(ReturnRef(string_listeners));
	EXPECT_CALL(vc, mock_get_int_listeners())
		.Times(0);
	EXPECT_CALL(v, handle_event(std::string("how low?")))
		.Times(1);

	vc.propagate_generic_event(std::string("how low?"));

	EXPECT_CALL(vc, is_connected(_))
		.WillOnce(Return(true));
	EXPECT_CALL(vc, enact_disconnection(_))
		.Times(1);
}


TEST(mvc, propagate_generic_event)
{
	ViewBridge vc;
	ViewString vs;
	ViewAll va;

	Implementation2::type_data<std::string>::init_data string_init_data;
	string_init_data.push_back("only testin'");
	Implementation2::type_data<integer>::init_data int_init_data;

	EXPECT_CALL(vc, is_connection_allowed(_, kCONNECTION_CONTEXT))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_connection(_, kCONNECTION_CONTEXT))
		.Times(2);
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::StringListener &>(_)))
		.Times(2)
		.WillRepeatedly(ReturnRef(string_init_data));
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::IntListener &>(Ref(va))))
		.WillOnce(ReturnRef(int_init_data));
	EXPECT_CALL(vs, mock_initialize(_))
		.Times(1);
	EXPECT_CALL(va, mock_string_initialize(_))
		.Times(1);
	EXPECT_CALL(va, mock_int_initialize(_))
		.Times(1);

	vs.connect(vc, kCONNECTION_CONTEXT);
	va.connect(vc, kCONNECTION_CONTEXT);

	ASSERT_EQ(vs.bridge(), &vc);
	ASSERT_EQ(va.bridge(), &vc);

	std::list<ViewBridge::StringListener*> string_listeners;

	EXPECT_CALL(vc, mock_get_string_listeners())
		.WillOnce(ReturnRef(string_listeners));
	EXPECT_CALL(vc, mock_get_int_listeners())
		.Times(0);

	vc.propagate_generic_event(std::string("hello!"));

	string_listeners.push_back(&vs);
	string_listeners.push_back(&va);

	EXPECT_CALL(vc, mock_get_string_listeners())
		.WillOnce(ReturnRef(string_listeners));
	EXPECT_CALL(vc, mock_get_int_listeners())
		.Times(0);
	EXPECT_CALL(vs, handle_event(std::string("how low?")))
		.Times(1);
	EXPECT_CALL(va, handle_event(std::string("how low?")))
		.Times(1);

	vc.propagate_generic_event(std::string("how low?"));

	EXPECT_CALL(vc, is_connected(_))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_disconnection(_))
		.Times(2);
}


struct IBaseEvent
{
	virtual ~IBaseEvent() {}
};


template<>
const std::string & ViewBridge::cast<std::string,IBaseEvent>(const IBaseEvent & base) const
{
	return dynamic_cast<const std::string &>(base);
}


template<>
const integer & ViewBridge::cast<integer,IBaseEvent>(const IBaseEvent & base) const
{
	return dynamic_cast<const integer &>(base);
}


struct PolymorphicEvent
	: IBaseEvent
	, integer
	, std::string
{
	PolymorphicEvent(int i, const std::string & s)
		: integer{i}
		, std::string(s)
		{}
};


template<>
const std::string & ViewBridge::cast<std::string,PolymorphicEvent>(const PolymorphicEvent & base) const
{
	return base;
}


template<>
const integer & ViewBridge::cast<integer,PolymorphicEvent>(const PolymorphicEvent & base) const
{
	return base;
}


struct OnlyStringEvent
	: IBaseEvent
	, std::string
{
	template<typename ...TArgs>
	OnlyStringEvent(TArgs&&... args) : std::string(std::forward<TArgs>(args)...) {}
};


template<>
const std::string & ViewBridge::cast<std::string,OnlyStringEvent>(const OnlyStringEvent & base) const
{
	return base;
}


TEST(mvc, propagate_polymorphic_events)
{
	ViewBridge vc;
	ViewString vs;
	ViewAll va;

	Implementation2::type_data<std::string>::init_data string_init_data;
	string_init_data.push_back("only testin'");
	Implementation2::type_data<integer>::init_data int_init_data;

	EXPECT_CALL(vc, is_connection_allowed(_, kCONNECTION_CONTEXT))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_connection(_, kCONNECTION_CONTEXT))
		.Times(2);
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::StringListener &>(_)))
		.Times(2)
		.WillRepeatedly(ReturnRef(string_init_data));
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::IntListener &>(Ref(va))))
		.WillOnce(ReturnRef(int_init_data));
	EXPECT_CALL(vs, mock_initialize(_))
		.Times(1);
	EXPECT_CALL(va, mock_string_initialize(_))
		.Times(1);
	EXPECT_CALL(va, mock_int_initialize(_))
		.Times(1);

	vs.connect(vc, kCONNECTION_CONTEXT);
	va.connect(vc, kCONNECTION_CONTEXT);

	ASSERT_EQ(vs.bridge(), &vc);
	ASSERT_EQ(va.bridge(), &vc);

	std::list<ViewBridge::StringListener*> string_listeners;
	std::list<ViewBridge::IntListener*> int_listeners;

	string_listeners.push_back(&vs);
	string_listeners.push_back(&va);
	int_listeners.push_back(&va);

	EXPECT_CALL(vc, mock_get_string_listeners())
		.WillOnce(ReturnRef(string_listeners));
	EXPECT_CALL(vc, mock_get_int_listeners())
		.WillOnce(ReturnRef(int_listeners));
	EXPECT_CALL(vs, handle_event(std::string("how low?")))
		.Times(1);
	EXPECT_CALL(va, handle_event(std::string("how low?")))
		.Times(1);
	EXPECT_CALL(va, handle_event(Matcher<const integer &>(_)))
		.Times(1);

	vc.propagate_generic_event(PolymorphicEvent(57, "how low?"));

	EXPECT_CALL(vc, is_connected(_))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_disconnection(_))
		.Times(2);
}


TEST(mvc, propagate_multiple_events)
{
	ViewBridge vc;
	ViewString vs;
	ViewAll va;

	Implementation2::type_data<std::string>::init_data string_init_data;
	string_init_data.push_back("only testin'");
	Implementation2::type_data<integer>::init_data int_init_data;

	EXPECT_CALL(vc, is_connection_allowed(_, kCONNECTION_CONTEXT))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_connection(_, kCONNECTION_CONTEXT))
		.Times(2);
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::StringListener &>(_)))
		.Times(2)
		.WillRepeatedly(ReturnRef(string_init_data));
	EXPECT_CALL(vc, mock_get_init_data(Matcher<const ViewBridge::IntListener &>(Ref(va))))
		.WillOnce(ReturnRef(int_init_data));
	EXPECT_CALL(vs, mock_initialize(_))
		.Times(1);
	EXPECT_CALL(va, mock_string_initialize(_))
		.Times(1);
	EXPECT_CALL(va, mock_int_initialize(_))
		.Times(1);

	vs.connect(vc, kCONNECTION_CONTEXT);
	va.connect(vc, kCONNECTION_CONTEXT);

	ASSERT_EQ(vs.bridge(), &vc);
	ASSERT_EQ(va.bridge(), &vc);

	std::list<ViewBridge::StringListener*> string_listeners;
	std::list<ViewBridge::IntListener*> int_listeners;

	string_listeners.push_back(&vs);
	string_listeners.push_back(&va);
	int_listeners.push_back(&va);

	OnlyStringEvent event1("hello!");
	PolymorphicEvent event2(2, "hello?");
	OnlyStringEvent event3("how low?");
	std::list<IBaseEvent *> event_list;
	event_list.push_back(&event1);
	event_list.push_back(&event2);
	event_list.push_back(&event3);

	EXPECT_CALL(vc, mock_get_string_listeners())
	 	.Times(3)
		.WillRepeatedly(ReturnRef(string_listeners));
	EXPECT_CALL(vc, mock_get_int_listeners())
		.WillOnce(ReturnRef(int_listeners));
	EXPECT_CALL(vs, handle_event(std::string("hello!")))
	 	.Times(1);
	EXPECT_CALL(va, handle_event(std::string("hello!")))
	 	.Times(1);
	EXPECT_CALL(vs, handle_event(std::string("hello?")))
		.Times(1);
	EXPECT_CALL(va, handle_event(std::string("hello?")))
		.Times(1);
	EXPECT_CALL(vs, handle_event(std::string("how low?")))
		.Times(1);
	EXPECT_CALL(va, handle_event(std::string("how low?")))
		.Times(1);
	EXPECT_CALL(va, handle_event(Matcher<const integer &>(_)))
	 	.Times(1);

	vc.propagate_events(event_list.begin(), event_list.end());

	EXPECT_CALL(vc, is_connected(_))
		.Times(2)
		.WillRepeatedly(Return(true));
	EXPECT_CALL(vc, enact_disconnection(_))
		.Times(2);
}


enum class eAction
{
	CREATION,
	MODIFICATION,
	DESTRUCTION
};


enum class eValueType
{
	STRING,
	INTEGER
};


class ICommand
{
public:
	eAction action() const { return _action; }
	eValueType type() const { return _type; }
	const std::string & key() const { return _key; }

protected:
	ICommand(eAction action, eValueType type, std::string key)
		: _action(action), _type(type), _key(key) {}

private:
	eAction _action;
	eValueType _type;
	std::string _key;
};


class CreateCommand : public ICommand
{
public:
	CreateCommand(eValueType type, const std::string & key) : ICommand(eAction::CREATION, type, key) {}
	virtual ~CreateCommand() = default;
};


class ModifyIntCommand : public ICommand
{
public:
	ModifyIntCommand(const std::string & key, int new_value)
		: ICommand(eAction::MODIFICATION, eValueType::INTEGER, key)
		, _new_value(new_value) {}
	virtual ~ModifyIntCommand() = default;

	int new_value() const { return _new_value; }

private:
	int _new_value;
};


class ModifyStringCommand : public ICommand
{
public:
	ModifyStringCommand(const std::string & key, const std::string & new_value)
		: ICommand(eAction::MODIFICATION, eValueType::STRING, key)
		, _new_value(new_value) {}
	virtual ~ModifyStringCommand() = default;

	const std::string & new_value() const { return _new_value; }

private:
	std::string _new_value;
};


class DestroyCommand : public ICommand
{
public:
	DestroyCommand(eValueType type, const std::string & key) : ICommand(eAction::DESTRUCTION, type, key) {}
	virtual ~DestroyCommand() = default;
};


class IReply
{
public:
	bool accepted() { return _accepted; }

	bool operator==(const IReply & rep) const { return _accepted == rep._accepted; }

protected:
	IReply(bool accepted) : _accepted(accepted) {}

private:
	bool _accepted;
};


class SuccessReply : public IReply
{
public:
	SuccessReply() : IReply(true) {}
	virtual ~SuccessReply() = default;
};


class FailureReply : public IReply
{
public:
	FailureReply() : IReply(false) {}
	virtual ~FailureReply() = default;
};


template<typename T>
class XEvent
{
public:
	eAction action() const { return _action; }
	const std::string & key() const { return _key; }

	bool operator==(const XEvent & ev) const { return _action == ev._action && _key == ev._key; }

protected:
	XEvent(eAction action, std::string key)
		: _action(action), _key(key) {}

private:
	eAction _action;
	std::string _key;
};


template<typename T>
class CreatedEvent : public XEvent<T>
{
public:
	CreatedEvent(const std::string & key) : XEvent<T>(eAction::CREATION, key) {}
	virtual ~CreatedEvent() = default;
};


template<typename T>
class ModifiedEvent : public XEvent<T>
{
public:
	ModifiedEvent(const std::string & key, T new_value)
		: XEvent<T>(eAction::MODIFICATION, key)
		, _new_value(new_value) {}
	virtual ~ModifiedEvent() = default;

	const T & new_value() const { return _new_value; }

private:
	T _new_value;
};


template<typename T>
class DestroyedEvent : public XEvent<T>
{
public:
	DestroyedEvent(const std::string & key) : XEvent<T>(eAction::DESTRUCTION, key) {}
	virtual ~DestroyedEvent() = default;
};


using IStringEvent = XEvent<std::string>;
using IIntEvent = XEvent<int>;


class Inputer3;
class InputBridge3;
class View3;
class ViewBridge3;


struct Implementation3
{
	typedef Inputer3 inputer;
	typedef InputBridge3 input_bridge;
	typedef ICommand message;
	typedef IReply reply;
	typedef View3 view;
	typedef ViewBridge3 view_bridge;
	typedef ViewBridge3 event_emiter;
	typedef unsigned int connection_context;
	template<typename T>
	struct type_data {};
};


template<>
struct Implementation3::type_data<IStringEvent>
{
	typedef std::map<std::string,std::string> init_data;
};


template<>
struct Implementation3::type_data<IIntEvent>
{
	typedef std::map<std::string,int> init_data;
};


class View3 : public ap::mvc::View<Implementation3>
{
public:
	virtual ~View3() {}
};


class ViewString3
	: public View3
	, public ap::mvc::Listener<Implementation3, IStringEvent>
{
public:
	virtual void initialize(Implementation3::type_data<IStringEvent>::init_data && data) override
	{
		_values = data;
	}

	MOCK_METHOD1(handle_event, void (const IStringEvent & event));

	const Implementation3::type_data<IStringEvent>::init_data & values() const { return _values; }

private:
	Implementation3::type_data<IStringEvent>::init_data _values;
};


class ViewInt3
	: public View3
	, public ap::mvc::Listener<Implementation3, IIntEvent>
{
public:
	virtual void initialize(Implementation3::type_data<IIntEvent>::init_data && data) override
	{
		_values = data;
	}

	MOCK_METHOD1(handle_event, void (const IIntEvent & event));

	const Implementation3::type_data<IIntEvent>::init_data & values() const { return _values; }

private:
	Implementation3::type_data<IIntEvent>::init_data _values;
};


class Inputer3 : public ap::mvc::Inputer<Implementation3>
{
public:
	MOCK_METHOD0(accepted_cb, void());
	MOCK_METHOD0(rejected_cb, void());
	MOCK_METHOD1(mock_reply_cb, void(Implementation3::reply));
	virtual void reply_cb(Implementation3::reply && reply) override { mock_reply_cb(reply); }
};


template<typename TSpoke, typename TImplementor>
class SpokeContainer
{
public:
	typedef std::set<TImplementor *, std::less<>> container;

	void add(TSpoke & spoke) { _spokes.insert( impl(spoke) ); }
	void remove(TSpoke & spoke) { _spokes.erase( impl(spoke) ); }
	bool has(const TSpoke & spoke) const { return _spokes.find( impl(spoke) ) != _spokes.end(); }
	const container & spokes() const { return _spokes; }

protected:
	static TImplementor * impl(TSpoke & spoke) { return static_cast<TImplementor *>(&spoke); }
	static const TImplementor * impl(const TSpoke & spoke) { return static_cast<const TImplementor *>(&spoke); }

private:
	container _spokes;
};


class ViewBridge3
	: public ap::mvc::ViewBridge<Implementation3>
	, public ap::mvc::Emiter<Implementation3, IStringEvent, IIntEvent>
{
	friend class ap::mvc::Hub<ViewBridge3>;
	friend class ap::mvc::ViewBridge<Implementation3>;
	typedef ap::mvc::Spoke<View3> Spoke;
	typedef SpokeContainer<Spoke, View3> view_container;
	typedef ap::mvc::Listener<Implementation3, IIntEvent> IntListener;
	typedef Implementation3::type_data<IIntEvent>::init_data IntInitData;
	typedef ap::mvc::Listener<Implementation3, IStringEvent> StringListener;
	typedef Implementation3::type_data<IStringEvent>::init_data StringInitData;

public:
	template<typename TTarget, typename TBase>
	const TTarget & cast(const TBase & base) const
	{
		throw std::bad_cast();
	}

	template<typename TEvent>
	std::list<ap::mvc::Listener<Implementation3,TEvent> *> listeners() const;

	MOCK_CONST_METHOD0(mock_get_string_listeners, std::list<StringListener *> & ());
	MOCK_CONST_METHOD0(mock_get_int_listeners, std::list<IntListener *> & ());

	template<typename TEvent>
	bool filter(const TEvent &, ap::mvc::Listener<Implementation3,TEvent> *) const { return true; }

protected:
	bool is_connection_allowed(const Spoke & view, Implementation3::connection_context) { return false == _views.has(view); }
	void enact_connection(Spoke & view, Implementation3::connection_context) { _views.add(view); }
	bool is_connected(const Spoke & view) { return _views.has(view); }
	void enact_disconnection(Spoke & view) { _views.remove(view); }

	const view_container::container & views() const { return _views.spokes(); }

private:
	view_container _views;
};


template<>
const IIntEvent & ViewBridge3::cast<IIntEvent,IIntEvent>(const IIntEvent & base) const
{
	return base;
}


template<>
const IStringEvent & ViewBridge3::cast<IStringEvent,IStringEvent>(const IStringEvent & base) const
{
	return base;
}


template<>
std::list<ap::mvc::Listener<Implementation3,IStringEvent> *> ViewBridge3::listeners<IStringEvent>() const
{
	return mock_get_string_listeners();
}


template<>
std::list<ap::mvc::Listener<Implementation3,IIntEvent> *> ViewBridge3::listeners<IIntEvent>() const
{
	return mock_get_int_listeners();
}


class InputBridge3 : public ap::mvc::InputBridge<Implementation3>
{
	friend class ap::mvc::Hub<InputBridge3>;
	typedef ap::mvc::Spoke<Inputer3> Spoke;
	typedef SpokeContainer<Spoke, Inputer3> inputer_container;

protected:
	bool is_connection_allowed(const Spoke & inputer, Implementation3::connection_context) { return false == _inputers.has(inputer); }
	void enact_connection(Spoke & inputer, Implementation3::connection_context) { _inputers.add(inputer); }
	bool is_connected(const Spoke & inputer) { return _inputers.has(inputer); }
	void enact_disconnection(Spoke & inputer) { _inputers.remove(inputer); }

private:
	inputer_container _inputers;
};


class Model
	: public ViewBridge3
	, public InputBridge3
{
public:
	Model() : _accept_input(true) {}

	ViewBridge3 & view_bridge() { return static_cast<ViewBridge3&>(*this); }
	InputBridge3 & input_bridge() { return static_cast<InputBridge3&>(*this); }
	const ViewBridge3 & view_bridge() const { return static_cast<const ViewBridge3&>(*this); }
	const InputBridge3 & input_bridge() const { return static_cast<const InputBridge3&>(*this); }

	MOCK_METHOD1(on_registered_cb, void(ap::mvc::Listener<Implementation3,IStringEvent> &));
	MOCK_METHOD1(on_registered_cb, void(ap::mvc::Listener<Implementation3,IIntEvent> &));

	virtual Implementation3::type_data<IStringEvent>::init_data
	get_init_data(const ap::mvc::Listener<Implementation3,IStringEvent> &) override
	{
		Implementation3::type_data<IStringEvent>::init_data tmp(_string_values);
		return std::move(tmp);
	}

	virtual Implementation3::type_data<IIntEvent>::init_data
	get_init_data(const ap::mvc::Listener<Implementation3,IIntEvent> &) override
	{
		Implementation3::type_data<IIntEvent>::init_data tmp(_int_values);
		return std::move(tmp);
	}

	void accept_input(bool accept) { _accept_input = accept; }

	std::map<std::string,std::string> & string_values() { return _string_values; }
	std::map<std::string,int> & int_values() { return _int_values; }

protected:
	bool _accept_input;
	std::map<std::string,std::string> _string_values;
	std::map<std::string,int> _int_values;
};


class SynchronousModel : public Model
{
public:
	virtual ap::mvc::eInputStatus handle_input(Inputer3 & inputer, Implementation3::message && message) override
	{
		if ( false == _accept_input )
		{
			return ap::mvc::eInputStatus::REJECTED;
		}

		on_reply(inputer, SuccessReply());

		if (message.type() == eValueType::STRING)
		{
			switch( message.action() )
			{
			case eAction::CREATION:
				{
					_string_values[message.key()] = std::string("");
					CreatedEvent<std::string> event( message.key() );
					ap::mvc::Emiter<Implementation3,IStringEvent>::propagate_event(event);
				}
				break;
			case eAction::MODIFICATION:
				{
					ModifyStringCommand& actual_command = static_cast<ModifyStringCommand&>(message);
					_string_values[message.key()] = actual_command.new_value();
					ModifiedEvent<std::string> event( message.key(), actual_command.new_value() );
					ap::mvc::Emiter<Implementation3,IStringEvent>::propagate_event(event);
				}
				break;
			case eAction::DESTRUCTION:
				{
					_string_values.erase( message.key() );
					DestroyedEvent<std::string> event( message.key() );
					ap::mvc::Emiter<Implementation3,IStringEvent>::propagate_event(event);
				}
				break;
			}
		}
		else
		{
			switch( message.action() )
			{
			case eAction::CREATION:
				{
					_int_values[message.key()] = -1;
					CreatedEvent<int> event( message.key() );
					ap::mvc::Emiter<Implementation3,IIntEvent>::propagate_event(event);
				}
				break;
			case eAction::MODIFICATION:
				{
					ModifyIntCommand& actual_command = static_cast<ModifyIntCommand&>(message);
					_int_values[message.key()] = actual_command.new_value();
					ModifiedEvent<int> event( message.key(), actual_command.new_value() );
					ap::mvc::Emiter<Implementation3,IIntEvent>::propagate_event(event);
				}
				break;
			case eAction::DESTRUCTION:
				{
					_int_values.erase( message.key() );
					DestroyedEvent<int> event( message.key() );
					ap::mvc::Emiter<Implementation3,IIntEvent>::propagate_event(event);
				}
				break;
			}
		}

		return ap::mvc::eInputStatus::ACCEPTED;
	}
};


constexpr unsigned NB_VIEWS = 3;
constexpr unsigned NB_INPUTERS = 2;
constexpr unsigned NB_VALUES = 10;
const char * const VALUES[NB_VALUES] = {
		"V01"
	,	"V02"
	,	"V03"
	,	"V04"
	,	"V05"
	,	"V06"
	,	"V07"
	,	"V08"
	,	"V09"
	,	"V10"
};


struct SynchronousModelFixture : ::testing::Test
{
	SynchronousModel _model;
	ViewString3 _views[NB_VIEWS];
	Inputer3 _inputers[NB_INPUTERS];
	std::list<ap::mvc::Listener<Implementation3,IStringEvent> *> _string_listeners;
	std::list<ap::mvc::Listener<Implementation3,IIntEvent> *> _int_listeners;

	void SetUp() override
	{
		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			_string_listeners.push_back( _views+i );
			_views[i].connect(_model.view_bridge(), kCONNECTION_CONTEXT);
		}
		for ( unsigned i = 0; i < NB_INPUTERS; ++i )
		{
			_inputers[i].connect(_model.input_bridge(), kCONNECTION_CONTEXT);
		}
	}

	void create(const char * key)
	{
		EXPECT_CALL(_inputers[0], accepted_cb())
			.Times(1);
		EXPECT_CALL(_inputers[0], mock_reply_cb(SuccessReply()))
			.Times(1);
		EXPECT_CALL(_model.view_bridge(), mock_get_string_listeners())
			.WillOnce(ReturnRef(_string_listeners));

		CreatedEvent<std::string> event(key);

		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			EXPECT_CALL(_views[i], handle_event(event))
				.Times(1);
		}

		const ap::mvc::eInputStatus status = _inputers[0].send_input(CreateCommand(eValueType::STRING, key));
		EXPECT_EQ(status, ap::mvc::eInputStatus::ACCEPTED);
	}

	void modify(const char * key, const char * new_value)
	{
		EXPECT_CALL(_inputers[0], accepted_cb())
			.Times(1);
		EXPECT_CALL(_inputers[0], mock_reply_cb(SuccessReply()))
			.Times(1);
		EXPECT_CALL(_model.view_bridge(), mock_get_string_listeners())
			.WillOnce(ReturnRef(_string_listeners));

		ModifiedEvent<std::string> event(key, new_value);

		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			EXPECT_CALL(_views[i], handle_event(event))
				.Times(1);
		}

		const ap::mvc::eInputStatus status = _inputers[0].send_input(ModifyStringCommand(key, new_value));
		EXPECT_EQ(status, ap::mvc::eInputStatus::ACCEPTED);
	}

	void destroy(const char * key)
	{
		EXPECT_CALL(_inputers[0], accepted_cb())
			.Times(1);
		EXPECT_CALL(_inputers[0], mock_reply_cb(SuccessReply()))
			.Times(1);
		EXPECT_CALL(_model.view_bridge(), mock_get_string_listeners())
			.WillOnce(ReturnRef(_string_listeners));

		DestroyedEvent<std::string> event(key);

		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			EXPECT_CALL(_views[i], handle_event(event))
				.Times(1);
		}

		const ap::mvc::eInputStatus status = _inputers[0].send_input(DestroyCommand(eValueType::STRING, key));
		EXPECT_EQ(status, ap::mvc::eInputStatus::ACCEPTED);
	}
};


TEST_F(SynchronousModelFixture, setup_and_teardown) {}


TEST_F(SynchronousModelFixture, reject_input)
{
	_model.accept_input( false );
	EXPECT_CALL(_inputers[0], rejected_cb())
		.Times(1);
	const ap::mvc::eInputStatus status = _inputers[0].send_input(CreateCommand(eValueType::STRING, VALUES[3]));
	EXPECT_EQ(status, ap::mvc::eInputStatus::REJECTED);
}


TEST_F(SynchronousModelFixture, accept_input)
{
	create(VALUES[3]);
}


TEST_F(SynchronousModelFixture, test_all_commands)
{
	create(VALUES[0]);
	create(VALUES[1]);
	create(VALUES[3]);
	create(VALUES[7]);
	modify(VALUES[3], VALUES[2]);
	modify(VALUES[7], VALUES[3]);
	create(VALUES[4]);
	destroy(VALUES[3]);
	modify(VALUES[4], VALUES[2]);
	modify(VALUES[0], VALUES[4]);

#define EXPECT_STR_AT( _idx, _value ) EXPECT_EQ( std::string( _value ) , _model.string_values()[VALUES[ _idx ]]);
#define EXPECT_NONE_AT( _idx ) EXPECT_EQ( _model.string_values().find(VALUES[ _idx ]), _model.string_values().end() );

	EXPECT_STR_AT ( 0, VALUES[4] );
	EXPECT_STR_AT ( 1, "" );
	EXPECT_NONE_AT( 2 );
	EXPECT_NONE_AT(	3 );
	EXPECT_STR_AT ( 4, VALUES[2] );
	EXPECT_NONE_AT( 5 );
	EXPECT_NONE_AT( 6 );
	EXPECT_STR_AT ( 7, VALUES[3] );
	EXPECT_NONE_AT( 8 );
	EXPECT_NONE_AT( 9 );

#undef EXPECT_STR_AT
#undef EXPECT_NONE_AT
}


class AsynchronousModel : public Model
{
public:
	virtual ap::mvc::eInputStatus handle_input(Inputer3 & inputer, Implementation3::message && message) override
	{
		return ap::mvc::eInputStatus::DELAYED;
	}

	void do_input(Inputer3 & inputer, const Implementation3::message & message)
	{
		if ( false == _accept_input )
		{
			on_async_rejected(inputer);
			return;
		}

		on_reply(inputer, SuccessReply());

		if (message.type() == eValueType::STRING)
		{
			switch( message.action() )
			{
			case eAction::CREATION:
				{
					_string_values[message.key()] = std::string("");
					CreatedEvent<std::string> event( message.key() );
					ap::mvc::Emiter<Implementation3,IStringEvent>::propagate_event(event);
				}
				break;
			case eAction::MODIFICATION:
				{
					const ModifyStringCommand& actual_command = static_cast<const ModifyStringCommand&>(message);
					_string_values[message.key()] = actual_command.new_value();
					ModifiedEvent<std::string> event( message.key(), actual_command.new_value() );
					ap::mvc::Emiter<Implementation3,IStringEvent>::propagate_event(event);
				}
				break;
			case eAction::DESTRUCTION:
				{
					_string_values.erase( message.key() );
					DestroyedEvent<std::string> event( message.key() );
					ap::mvc::Emiter<Implementation3,IStringEvent>::propagate_event(event);
				}
				break;
			}
		}
		else
		{
			switch( message.action() )
			{
			case eAction::CREATION:
				{
					_int_values[message.key()] = -1;
					CreatedEvent<int> event( message.key() );
					ap::mvc::Emiter<Implementation3,IIntEvent>::propagate_event(event);
				}
				break;
			case eAction::MODIFICATION:
				{
					const ModifyIntCommand& actual_command = static_cast<const ModifyIntCommand&>(message);
					_int_values[message.key()] = actual_command.new_value();
					ModifiedEvent<int> event( message.key(), actual_command.new_value() );
					ap::mvc::Emiter<Implementation3,IIntEvent>::propagate_event(event);
				}
				break;
			case eAction::DESTRUCTION:
				{
					_int_values.erase( message.key() );
					DestroyedEvent<int> event( message.key() );
					ap::mvc::Emiter<Implementation3,IIntEvent>::propagate_event(event);
				}
				break;
			}
		}

		on_async_accepted(inputer);
	}
};


struct AsynchronousModelFixture : ::testing::Test
{
	AsynchronousModel _model;
	ViewString3 _blinds[NB_VIEWS];
	ViewInt3 _views[NB_VIEWS];
	Inputer3 _inputers[NB_INPUTERS];
	std::list<ap::mvc::Listener<Implementation3,IStringEvent> *> _string_listeners;
	std::list<ap::mvc::Listener<Implementation3,IIntEvent> *> _int_listeners;

	void SetUp() override
	{
		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			_int_listeners.push_back( _views+i );
			_views[i].connect(_model.view_bridge(), kCONNECTION_CONTEXT);
			_blinds[i].connect(_model.view_bridge(), kCONNECTION_CONTEXT);
		}
		for ( unsigned i = 0; i < NB_INPUTERS; ++i )
		{
			_inputers[i].connect(_model.input_bridge(), kCONNECTION_CONTEXT);
		}
	}

	void create(const char * key)
	{
		const ap::mvc::eInputStatus status = _inputers[1].send_input(CreateCommand(eValueType::INTEGER, key));
		EXPECT_EQ(status, ap::mvc::eInputStatus::DELAYED);

		EXPECT_CALL(_inputers[1], accepted_cb())
			.Times(1);
		EXPECT_CALL(_inputers[1], mock_reply_cb(SuccessReply()))
			.Times(1);
		EXPECT_CALL(_model.view_bridge(), mock_get_int_listeners())
			.WillOnce(ReturnRef(_int_listeners));

		CreatedEvent<int> event(key);

		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			EXPECT_CALL(_views[i], handle_event(event))
				.Times(1);
		}

		_model.do_input(_inputers[1], CreateCommand(eValueType::INTEGER, key));
	}

	void modify(const char * key, int new_value)
	{
		const ap::mvc::eInputStatus status = _inputers[1].send_input(ModifyIntCommand(key, new_value));
		EXPECT_EQ(status, ap::mvc::eInputStatus::DELAYED);

		EXPECT_CALL(_inputers[1], accepted_cb())
			.Times(1);
		EXPECT_CALL(_inputers[1], mock_reply_cb(SuccessReply()))
			.Times(1);
		EXPECT_CALL(_model.view_bridge(), mock_get_int_listeners())
			.WillOnce(ReturnRef(_int_listeners));

		ModifiedEvent<int> event(key, new_value);

		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			EXPECT_CALL(_views[i], handle_event(event))
				.Times(1);
		}

		_model.do_input(_inputers[1], ModifyIntCommand(key, new_value));
	}

	void destroy(const char * key)
	{
		const ap::mvc::eInputStatus status = _inputers[1].send_input(DestroyCommand(eValueType::INTEGER, key));
		EXPECT_EQ(status, ap::mvc::eInputStatus::DELAYED);

		EXPECT_CALL(_inputers[1], accepted_cb())
			.Times(1);
		EXPECT_CALL(_inputers[1], mock_reply_cb(SuccessReply()))
			.Times(1);
		EXPECT_CALL(_model.view_bridge(), mock_get_int_listeners())
			.WillOnce(ReturnRef(_int_listeners));

		DestroyedEvent<int> event(key);

		for ( unsigned i = 0; i < NB_VIEWS; ++i )
		{
			EXPECT_CALL(_views[i], handle_event(event))
				.Times(1);
		}

		_model.do_input(_inputers[1], DestroyCommand(eValueType::INTEGER, key));
	}
};


TEST_F(AsynchronousModelFixture, setup_and_teardown) {}


TEST_F(AsynchronousModelFixture, reject_input)
{
	_model.accept_input( false );
	const ap::mvc::eInputStatus status = _inputers[1].send_input(CreateCommand(eValueType::INTEGER, VALUES[3]));
	EXPECT_EQ(status, ap::mvc::eInputStatus::DELAYED);

	EXPECT_CALL(_inputers[1], rejected_cb())
		.Times(1);

	_model.do_input(_inputers[1], CreateCommand(eValueType::INTEGER, VALUES[3]));
}


TEST_F(AsynchronousModelFixture, accept_input)
{
	create(VALUES[3]);
}


#if 0
TEST_F(AsynchronousModelFixture, test_all_commands)
{
	create(VALUES[0]);
	create(VALUES[1]);
	create(VALUES[3]);
	create(VALUES[7]);
	modify(VALUES[3], VALUES[2]);
	modify(VALUES[7], VALUES[3]);
	create(VALUES[4]);
	destroy(VALUES[2]);
	modify(VALUES[4], VALUES[2]);
	modify(VALUES[0], VALUES[4]);
	EXPECT_TRUE(
		std::equal(
				_model.keys().begin()
			,	_model.keys().end()
			,	VALUES + 1
			,	VALUES + 5
			)
		);
}
#endif

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
