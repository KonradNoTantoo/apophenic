#ifndef MVC_HXX
#define MVC_HXX

#include <utility>


namespace ap
{
namespace mvc
{


template<class TImplementor>
class Hub;
template<class TImplementor>
class Spoke;


class IHub {};


template<class TImplementor>
class Hub : public IHub
{
public:
	template<class TOther, typename TContext>
	void connect(Spoke<TOther> & spoke, TContext context)
	{
		if (self()->is_connection_allowed(spoke, context))
		{
			if ( spoke.connection_cb(*this, context, true) )
			{
				self()->enact_connection(spoke, context);
			}
		}
		else
		{
			spoke.connection_cb(*this, context, false);
		}
	}

	template<class TOther>
	void disconnect(Spoke<TOther> & spoke)
	{
		if (self()->is_connected(spoke))
		{
			if ( spoke.disconnection_cb(*this, true) )
			{
				self()->enact_disconnection(spoke);
			}
		}
		else
		{
			spoke.disconnection_cb(*this, false);
		}
	}

private:
	TImplementor * self() { return static_cast<TImplementor*>(this); }
	const TImplementor * self() const { return static_cast<const TImplementor*>(this); }
};


template<class TImplementor>
class Spoke
{
	template<class TOther> friend class Hub;

public:
	Spoke() : _hub(nullptr) {}

	template<class TOther, typename TContext>
	void connect(Hub<TOther> & h, TContext context) { h.connect(*this, context); }

	template<class TOther>
	void disconnect() { if( nullptr != _hub ) static_cast<Hub<TOther>*>(_hub)->disconnect(*this); }

protected:
	template<class TOther, typename TContext>
	bool connection_cb(Hub<TOther> & h, TContext context, bool allowed)
	{
		if ( _hub == nullptr && self()->on_connection_attempt(h, context, allowed) && allowed )
		{
			_hub = &h;
			return true;
		}

		return false;
	}

	template<class TOther>
	bool disconnection_cb(Hub<TOther> & h, bool allowed)
	{
		if ( _hub != nullptr && self()->on_disconnection_attempt(h, allowed) && allowed )
		{
			_hub = nullptr;
			return true;
		}

		return false;
	}

	IHub * hub() const { return _hub; }

private:
	TImplementor * self() { return static_cast<TImplementor*>(this); }
	const TImplementor * self() const { return static_cast<const TImplementor*>(this); }

	IHub * _hub;
};

template<typename TImplementations, typename... TEvents>
class Emiter;
template<typename TImplementations, typename... TEvents>
class Listener;
template<typename TImplementations>
class View;
template<typename TImplementations>
class ViewBridge;
template<typename TImplementations>
class Inputer;
template<typename TImplementations>
class InputBridge;


template<typename TImplementations, typename TEvent>
class Emiter<TImplementations, TEvent>
{
	typedef typename TImplementations::event_emiter TImplementor;
	typedef typename TImplementations::template type_data<TEvent>::init_data TInitData;

public:
	typedef Listener<TImplementations, TEvent> TListener;

	void propagate_event(const TEvent & event) const
	{
		for (auto & listener : self()->template listeners<TEvent>())
		{
			if (self()->filter(event, listener))
			{
				listener->handle_event(event);
			}
		}
	}

	template<typename TGeneric>
	void propagate_generic_event(const TGeneric & generic) const
		try
		{
			const TEvent & event = self()->template cast<TEvent>(generic);
			propagate_event(event);
		}
		catch(const std::bad_cast &) {}

	template<typename TEventIterator>
	void propagate_events(TEventIterator start, TEventIterator finish) const
	{
		while(start != finish) propagate_generic_event<>( **start++ );
	}

	template<typename TGeneric>
	static constexpr bool can_register_listener(const Listener<TImplementations,TGeneric> & listener)
	{
		return std::is_same<TGeneric,TEvent>();
	}

	template<typename TOther>
	bool on_listener_registered(TOther & other)
	{
		TListener* const listener = dynamic_cast<TListener*>(&other);

		if (nullptr != listener)
		{
			on_registered_cb(*listener);
			return initialize_listener(*listener);
		}

		return true;
	}

	bool initialize_listener(TListener & listener) { return listener.initialize(get_init_data(listener)); }

protected:
	virtual void on_registered_cb(TListener & listener) = 0;
	virtual TInitData get_init_data(const TListener & listener) = 0;

private:
	TImplementor * self() { return static_cast<TImplementor*>(this); }
	const TImplementor * self() const { return static_cast<const TImplementor*>(this); }
};


template<typename TImplementations, typename TEvent, typename... TEvents>
class Emiter<TImplementations, TEvent, TEvents...>
	: public Emiter<TImplementations, TEvent>
	, public Emiter<TImplementations, TEvents...>
{
	typedef typename TImplementations::event_emiter TImplementor;
	typedef Emiter<TImplementations, TEvent> TCurrent;
	typedef Emiter<TImplementations, TEvents...> TNext;

public:
	template<typename TGeneric>
	void propagate_generic_event(const TGeneric & generic) const
	{
		try
		{
			const TEvent & event = self()->template cast<TEvent,TGeneric>(generic);
			TCurrent::propagate_event(event);
		}
		catch(const std::bad_cast &) {}

		TNext::propagate_generic_event(generic);
	}

	template<typename TEventIterator>
	void propagate_events(TEventIterator start, TEventIterator finish) const
	{
		while(start != finish) propagate_generic_event<>( **start++ );
	}

	template<typename TGeneric>
	static constexpr bool can_register_listener(const Listener<TImplementations,TGeneric> & listener)
	{
		return TCurrent::template can_register_listener<TGeneric>(listener)
			|| TNext::template can_register_listener<TGeneric>(listener);
	}

	template<typename TOther>
	bool on_listener_registered(TOther & other)
	{
		typename TCurrent::TListener* const listener =
			dynamic_cast<typename TCurrent::TListener*>(&other);

		if (	nullptr != listener
			&&	false == TCurrent::initialize_listener(*listener) // has side effect
			)
		{
			return false;
		}

		return TNext::on_listener_registered(other);
	}

private:
	TImplementor * self() { return static_cast<TImplementor*>(this); }
	const TImplementor * self() const { return static_cast<const TImplementor*>(this); }
};


template<typename TImplementations, typename TEvent>
class Listener<TImplementations, TEvent>
{
	typedef typename TImplementations::template type_data<TEvent>::init_data TInitData;

public:
	virtual void handle_event(const TEvent & event) = 0;
	virtual bool initialize(TInitData && event) = 0;
};


template<typename TImplementations, typename TEvent, typename... TEvents>
class Listener<TImplementations, TEvent, TEvents...>
	: public Listener<TImplementations, TEvent>
	, public Listener<TImplementations, TEvents...>
{

};


template<typename TImplementations>
class ViewBridge : public Hub<typename TImplementations::view_bridge> {};


template<typename TImplementations>
class View : public Spoke<typename TImplementations::view>
{
	friend class ViewBridge<TImplementations>;
	friend class Spoke<typename TImplementations::view>;
	typedef typename TImplementations::view TImplementor;
	typedef typename TImplementations::view_bridge TBridge;
	typedef typename TImplementations::connection_context TContext;

public:
	virtual ~View() { disconnect(); }

	void disconnect() { Spoke<typename TImplementations::view>::template disconnect<TBridge>(); }

	TBridge * bridge() const { return static_cast<TBridge*>(this->hub()); }

protected:
	bool on_connection_attempt(Hub<TBridge> & c, TContext context, bool allowed)
	{
		return allowed && static_cast<TBridge&>(c).on_listener_registered(*self());
	}

	bool on_disconnection_attempt(Hub<TBridge> & , bool) const { return true; }

private:
	TImplementor * self() { return static_cast<TImplementor*>(this); }
	const TImplementor * self() const { return static_cast<const TImplementor*>(this); }
};


enum class eInputStatus
{
		REJECTED
	,	DELAYED
	,	ACCEPTED
};


template<typename TImplementations>
class InputBridge : public Hub<typename TImplementations::input_bridge>
{
	friend class Inputer<TImplementations>;
	typedef typename TImplementations::inputer TInputer;
	typedef typename TImplementations::message TMessage;
	typedef typename TImplementations::reply TReply;

protected:
	virtual eInputStatus handle_input(TInputer & inputer, TMessage && message) = 0;

	void on_reply(TInputer & inputer, TReply && reply) { inputer.reply_cb(std::move(reply)); }
	void on_async_accepted(TInputer & inputer) { inputer.accepted_cb(); }
	void on_async_rejected(TInputer & inputer) { inputer.rejected_cb(); }
};


template<typename TImplementations>
class Inputer : public Spoke<typename TImplementations::inputer>
{
	friend class InputBridge<TImplementations>;
	friend class Spoke<typename TImplementations::inputer>;
	typedef typename TImplementations::inputer TInputer;
	typedef typename TImplementations::input_bridge TBridge;
	typedef typename TImplementations::message TMessage;
	typedef typename TImplementations::reply TReply;
	typedef typename TImplementations::connection_context TContext;

public:
	virtual ~Inputer() { disconnect(); }

	eInputStatus send_input(TMessage && message)
	{
		const eInputStatus status = bridge()->handle_input(static_cast<TInputer&>(*this), std::move(message));

		switch( status )
		{
		case eInputStatus::REJECTED:
			rejected_cb();
			break;
		case eInputStatus::DELAYED:
			// do nothing, input bridge will have to call accepted_cb or rejected_cb
			break;
		case eInputStatus::ACCEPTED:
			accepted_cb();
			break;
		}

		return status;
	}

	void disconnect() { Spoke<typename TImplementations::inputer>::template disconnect<TBridge>(); }

	TBridge * bridge() const { return static_cast<TBridge*>(this->hub()); }

protected:
	virtual void accepted_cb() = 0;
	virtual void rejected_cb() = 0;
	virtual void reply_cb(TReply && reply) = 0;

	bool on_connection_attempt(Hub<TBridge> & , TContext , bool ) const { return true; }
	bool on_disconnection_attempt(Hub<TBridge> & , bool ) const { return true; }
};


}
}

#endif // MVC_HXX
