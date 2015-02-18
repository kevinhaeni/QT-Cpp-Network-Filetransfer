#include "stdafx.h"

#define LOGLOG(s) { std::stringstream ss_; ss_ << s; loglog(ss_.str()); }

util::ThreadMutex g_sync;

typedef std::vector<std::string> TStrings;
TStrings g_log;

void
loglog(const std::string& s)
{
#if 1
	util::ScopedLock lock(&g_sync);
	g_log.push_back(s);
#else
	std::cout << s << std::endl;
#endif
}

void
testMemoryStream()
{

	{
		util::MemoryStream memstream;

		std::string s = "42-1234";
		const char* sz = s.c_str();
		unsigned int len = s.length();
		memstream << len;
		memstream.write(reinterpret_cast<const unsigned char*>(sz), len);

		memstream.seekg(0);

		len = 0;
		memstream >> len;

		util::ScopedArray<char> buf(new char[len + 1]);
		memset(buf.get(), 0, len + 1);

		memstream.read(reinterpret_cast<unsigned char*>(buf.get()), len);

		std::string s2 = buf.get();

		assert(s == s2);
	}

	{
		for (int j = 0; j < 10; ++j)
		for (int i = 0; i <= 255; ++i)
		{
			unsigned char buf[] = { 7, (unsigned char)i };
			util::MemoryStream memstream(buf, buf + sizeof(buf));

			unsigned char value = 0;
			memstream >> value;

			assert(7 == value);
		}
	}

#if 0
	{
		for (int i = std::numeric_limits<int>::min(); i <= std::numeric_limits<int>::max(); ++i)
		{
			util::MemoryStream memstream;

			memstream << i;
			memstream.seekg(0);

			int j = 0;
			memstream >> j;

			assert(i == j);

			if (std::numeric_limits<int>::max() == i)
				break;
		}
	}
#endif
}

void
testGetOpt()
{
	{
		util::GetOpt opt(_T("\"aaa\" bbb"));
		assert(2 == opt.argv.size() &&
			   opt.argv[0] == _T("aaa") &&
			   opt.argv[1] == _T("bbb"));
	}

	{
		util::GetOpt opt(_T("\"aaa\"bbb"));
		assert(1 == opt.argv.size() &&
			   opt.argv[0] == _T("aaabbb"));
	}

	{
		util::GetOpt opt(_T("ccc \"aaa\" \"bbb\""));
		assert(3 == opt.argv.size() &&
			   opt.argv[0] == _T("ccc") &&
			   opt.argv[1] == _T("aaa") &&
			   opt.argv[2] == _T("bbb"));
	}

	{
		util::GetOpt opt(_T("\"a\"\"a\"\"a\""));
		assert(1 == opt.argv.size() &&
			   opt.argv[0] == _T("a\"a\"a"));
	}

	{
		util::GetOpt opt(_T(""));
		assert(0 == opt.argv.size());
	}
}

void
testSharedPtr()
{
	struct InstanceCounter
	{
		static int& count()
		{
			static int s_count = 0;
			return s_count;
		}

		InstanceCounter(int tag_) : tag(tag_) { count()++; }
		~InstanceCounter() { count()--; }

		int tag;
	};

	{
		util::SharedPtr<InstanceCounter> counter(new InstanceCounter(0));
		assert(InstanceCounter::count() == 1);

		util::SharedPtr<InstanceCounter> counter2(counter);
		assert(InstanceCounter::count() == 1);

		util::SharedPtr<InstanceCounter> counter3;
		counter3 = counter2;
		assert(InstanceCounter::count() == 1);
		assert(counter3 == counter2);
		assert(!(counter3 < counter2) && !(counter2 < counter3));

		counter3 = 0;
		assert(InstanceCounter::count() == 1);
		assert(counter3 != counter2);
		assert(counter3 < counter2 || counter2 < counter3);

		counter = 0;
		assert(InstanceCounter::count() == 1);
	}

	assert(InstanceCounter::count() == 0);

	{
		util::SharedPtr<InstanceCounter> counter(new InstanceCounter(1));
		assert(InstanceCounter::count() == 1);

		util::SharedPtr<InstanceCounter> counter2(new InstanceCounter(2));
		assert(InstanceCounter::count() == 2);

		counter = counter2;
		assert(2 == counter->tag);
		assert(InstanceCounter::count() == 1);
	}

	assert(InstanceCounter::count() == 0);
}

void
testSimpleClientServerCommunication()
{
	struct ClientBindingDelegate : net::IBindingDelegate, net::IStreamListenerDelegate
	{
		//
		// net::IBindingDelegate
		//

		virtual void onStreamCreated(net::TStreamPtr stream)
		{
			net::StreamListener::instance().addDelegate(stream, this);

			const char ping[] = "ping";
			net::StreamListener::instance().writeStream(
				stream->id(), reinterpret_cast<const unsigned char*>(ping), sizeof(ping) - 1);
		}

		//
		// net::IBindingDelegate
		//

		std::string received;

		virtual void onDataReceived(
			::net::IStream::TId streamId,
			const unsigned char* buf,
			size_t bufSize)
		{
			const char* sz = reinterpret_cast<const char*>(buf);
			received += std::string(sz, sz + bufSize);

			assert(received.length() <= std::string("pong").length());
			if (received == "pong")
				net::StreamListener::instance().cancelRun();
		}

		virtual void onStreamDied(::net::IStream::TId streamId)
		{
			// It's OK
		}
	};

	struct ServerBindingDelegate : net::IBindingDelegate, net::IStreamListenerDelegate
	{
		//
		// net::IBindingDelegate
		//

		net::TStreamPtr m_stream;

		virtual void onStreamCreated(net::TStreamPtr stream)
		{
			m_stream = stream;

			net::StreamListener::instance().addDelegate(stream, this);
		}

		//
		// net::IBindingDelegate
		//

		std::string received;

		virtual void onDataReceived(
			::net::IStream::TId streamId,
			const unsigned char* buf,
			size_t bufSize)
		{
			const char* sz = reinterpret_cast<const char*>(buf);
			received += std::string(sz, sz + bufSize);

			assert(received.length() <= std::string("ping").length());
			if (received == "ping")
			{
				// Send data back
				const char pong[] = "pong";
				net::StreamListener::instance().writeStream(streamId, reinterpret_cast<const unsigned char*>(pong), sizeof(pong) - 1);
			}
		}

		virtual void onStreamDied(::net::IStream::TId streamId)
		{
			// It's OK
		}
	};

	net::TBindingPtr server = net::BindingFactory::createBinding(net::BindingFactory::BINDING_TCP_SERVER);
	net::TBindingPtr client = net::BindingFactory::createBinding(net::BindingFactory::BINDING_TCP_CLIENT);

	const char* address = "127.0.0.1:7777";

	ServerBindingDelegate serverBindingDelegate;
	server->bind(address, &serverBindingDelegate);

	ClientBindingDelegate clientBindingDelegate;
	client->bind(address, &clientBindingDelegate);

	net::StreamListener::instance().run();
}

void
testMessenger()
{
	struct IntMessage : msg::IMessage
	{
		IntMessage(int v_ = 0) : value(v_) {}

		enum {
			TYPE_ID = 7
		};

		int value;

		virtual util::T_UI4 typeId() const
		{
			return TYPE_ID;
		}

		virtual void save(TOStream& out)
		{
			out << value;
		}

		virtual void load(TIStream& in)
		{
			in >> value;
		}
	};

	struct MsgFactory : msg::IMessageFactory
	{
		virtual msg::TMessagePtr createMessage(util::T_UI4 messageType)
		{
			msg::TMessagePtr msg;

			switch (messageType)
			{
			case IntMessage::TYPE_ID:
				{
					msg = new IntMessage;
				}
				break;
			default:
				assert(!"Unknown message type");
				throw std::logic_error("Unknown message type");
			}

			return msg;
		}
	} msgFactory;

	struct MsgBindingDelegate : msg::IBindingDelegate, msg::IMessengerDelegate
	{
		//
		// msg::IBindingDelegate
		//

		virtual void onStreamCreated(net::IStream::TId streamId)
		{
			msg::Messenger& messenger = msg::Messenger::instance();

			messenger.addDelegate(streamId, this);
			messenger.sendMessage(streamId, new IntMessage(10));
		}

		//
		// msg::IMessengerDelegate
		//

		virtual void onMessageReceived(
			::net::IStream::TId streamId,
			::msg::TMessagePtr message)
		{
			msg::IMessage* m = message.get();
			IntMessage* iMsg = 0;
			if (iMsg = dynamic_cast<IntMessage*>(m))
			{
				if (0 >= iMsg->value)
				{
					net::StreamListener::instance().cancelRun();
				}
				else
				{
					iMsg->value--;

					// Send back
					msg::Messenger::instance().sendMessage(streamId, message);
				}
			}
		}

		virtual void onStreamDied(::net::IStream::TId streamId)
		{
			// It's ok, we're forcing a stream to close
			assert(1);
		}

	} bindingDelegate;

	msg::Messenger& messenger = msg::Messenger::instance();
	messenger.setMessageFactory(&msgFactory);
	messenger.setBindingDelegate(&bindingDelegate);

	net::TBindingPtr server = net::BindingFactory::createBinding(net::BindingFactory::BINDING_TCP_SERVER);
	net::TBindingPtr client = net::BindingFactory::createBinding(net::BindingFactory::BINDING_TCP_CLIENT);

	const char* address = "127.0.0.1:7777";

	server->bind(address, &messenger);
	client->bind(address, &messenger);

	net::StreamListener::instance().run();

	messenger.setMessageFactory(0);
	messenger.setBindingDelegate(0);
}

void
testMessenger2()
{
	struct IntMessage : msg::IMessage
	{
		IntMessage(int v_ = 0) : value(v_) {}

		enum {
			TYPE_ID = 7
		};

		int value;

		virtual util::T_UI4 typeId() const
		{
			return TYPE_ID;
		}

		virtual void save(TOStream& out)
		{
			out << value;
		}

		virtual void load(TIStream& in)
		{
			in >> value;
			assert(0 != value && "Message is deserialized incorrectly");
		}
	};

	struct MsgFactory : msg::IMessageFactory
	{
		virtual msg::TMessagePtr createMessage(util::T_UI4 messageType)
		{
			msg::TMessagePtr msg;

			switch (messageType)
			{
			case IntMessage::TYPE_ID:
				{
					msg = new IntMessage;
				}
				break;
			default:
				assert(!"Unknown message type");
				throw std::logic_error("Unknown message type");
			}

			return msg;
		}
	} msgFactory;

	struct MsgBindingDelegate : msg::IBindingDelegate, msg::IMessengerDelegate
	{
		//
		// msg::IBindingDelegate
		//

		virtual void onStreamCreated(net::IStream::TId streamId)
		{
			LOGLOG("Stream created: " << streamId);

			msg::Messenger& messenger = msg::Messenger::instance();

			messenger.addDelegate(streamId, this);
			messenger.sendMessage(streamId, new IntMessage(10));
		}

		//
		// msg::IMessengerDelegate
		//

		virtual void onMessageReceived(
			::net::IStream::TId streamId,
			::msg::TMessagePtr message)
		{
			msg::IMessage* m = message.get();
			IntMessage* iMsg = 0;
			if (iMsg = dynamic_cast<IntMessage*>(m))
			{
				int val = iMsg->value;
				assert(0 != val && "Message is deserialized incorrectly");
				if (1 >= val)
				{
					assert(1 == val);
					LOGLOG("Quitting: " << streamId);
					net::StreamListener::instance().cancelRun();
				}
				else
				{
					LOGLOG("Received: " << val << " " << streamId);

					iMsg->value--;

					// Send back
					msg::Messenger::instance().sendMessage(streamId, message);
				}
			}
		}

		virtual void onStreamDied(::net::IStream::TId streamId)
		{
			// It's ok, we're forcing a stream to close
			assert(1);
			LOGLOG("Stream died: " << streamId);
		}

	} bindingDelegate;

	msg::Messenger& messenger = msg::Messenger::instance();
	messenger.setMessageFactory(&msgFactory);
	messenger.setBindingDelegate(&bindingDelegate);

	net::TBindingPtr server = net::BindingFactory::createBinding(net::BindingFactory::BINDING_TCP_SERVER);
	net::TBindingPtr client = net::BindingFactory::createBinding(net::BindingFactory::BINDING_TCP_CLIENT);

	const char* address = "127.0.0.1:7777";

	server->bind(address, &messenger);
	client->bind(address, &messenger);

	net::StreamListener::instance().run();

	messenger.setMessageFactory(0);
	messenger.setBindingDelegate(0);

	util::ScopedLock lock(&g_sync);
	for (TStrings::const_iterator ii = g_log.begin(); ii != g_log.end(); ++ii)
	{
		std::cout << *ii << std::endl;
	}
}

int
main(int argc, char* argv[])
{
    WSADATA wsaData;
	memset(&wsaData, 0, sizeof(wsaData));

	int res = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR)
		throw util::Error("WSAStartup function failed");

    int dbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    _CrtSetDbgFlag(dbgFlag | _CRTDBG_ALLOC_MEM_DF);

	_CrtMemState memState;
    _CrtMemCheckpoint(&memState);

	try
	{
#if 0
		testMemoryStream();
#elif 1
		for (int i = 0; i < 100; ++i)
		{
			testMessenger2();
		}
#else
		testGetOpt();
		testSharedPtr();
		testMemoryStream();
		testSimpleClientServerCommunication();
		testMessenger();
//		testMessenger2();
#endif

		std::cout << "OK!" << std::endl;
	}
	catch (const std::exception& x)
	{
		std::cerr << "ERROR: " << x.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "ERROR: unknown exception occured" << std::endl;
	}

    _CrtMemState memState2;
    _CrtMemCheckpoint(&memState2);
    _CrtMemState diff;

    if (_CrtMemDifference(&diff, &memState, &memState2))
    {
		std::cerr << "WARNING: memory leaks detected" << std::endl;

		_CrtMemDumpStatistics(&diff);
        _CrtMemDumpAllObjectsSince(&memState);
    }

    _CrtSetDbgFlag(dbgFlag);

	::WSACleanup();

	return 0;
}
