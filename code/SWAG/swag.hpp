#ifndef HEADER_SWAG
#define HEADER_SWAG

#include <string>
#include <unordered_map>

#include "threadsafe_queue.hpp"
#include "threadsafe_outputter.hpp"

#include "utils.hpp"

#include "sha1.hpp"

//class - private namespace
class SWAG {
public:
	//Constructors (default, copy, move)
	SWAG();
	SWAG(const SWAG&) = delete;
	SWAG& operator=(const SWAG&) = delete;
	SWAG(SWAG&&) = delete;
	SWAG&& operator=(SWAG&&) = delete;

	void run();

private:
	using TYPE_HITCOUNT = std::size_t;

	class ISWAGWorker {
	public:
		ISWAGWorker(SWAG* par_swag) : _swag(par_swag) {}
		virtual void run() = 0;

	protected:
		SWAG* _swag;
	};

	class ProcessorWorker;
	friend class ProcessorWorker;
	class WebWorker;
	friend class WebWorker;

	struct RequestData {
		std::string URI;
		std::string UserAgent;
	};

private:
	ThreadsafeQueue<RequestData> _processingQueue;
	ThreadsafeOutputter _outputter;
};

//==========Workers
class SWAG::WebWorker: public ISWAGWorker {
public:
	WebWorker(SWAG* par_swag, TYPE_PORT par_port) : ISWAGWorker(par_swag), _port(par_port) {}
	void run() override;

private:
	TYPE_PORT _port;
};

class SWAG::ProcessorWorker : public ISWAGWorker {
public:
	ProcessorWorker(SWAG* par_swag) : ISWAGWorker(par_swag) {}

	void run() override;

private:
	//TODO string -> string_view
	static std::string _produceLogMessage(
		const std::string& par_URI, const std::string& par_URIHash, TYPE_HITCOUNT par_URIHitcount,
		const std::string& par_UserAgent, const std::string& par_UserAgentHash, TYPE_HITCOUNT par_UserAgentHitcount);

private:
	SHA1 _hasher;

	std::unordered_map<std::string, std::size_t> _URIHitcount;
	std::unordered_map<std::string, std::size_t> _UserAgentHitcount;
};

#endif
