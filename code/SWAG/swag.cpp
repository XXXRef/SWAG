#include "swag.hpp"



#include <Windows.h>
#include <cstdlib>

using namespace std;

SWAG::SWAG() = default;

void SWAG::run() {
	//Fire up processor workers in separate threads
	using TYPE_THREADAMOUNT = decltype(std::thread::hardware_concurrency()) ;
	
	const TYPE_THREADAMOUNT threadsAmount = std::thread::hardware_concurrency();
	const TYPE_THREADAMOUNT workersAmount = (threadsAmount < 3) ? 1 : threadsAmount - 1;

	for (auto i = workersAmount; i != 0; --i) {
		thread(&SWAG::ProcessorWorker::run, SWAG::ProcessorWorker(this)).detach();
	}

	//Fire up web worker in current thread
	WebWorker(this).run();
}

//==========Workers
void SWAG::WebWorker::run() {
	while (true) {
		_swag->_processingQueue.push({ "TEST URI", "TEST USERAGENT" });
		::Sleep(500);
	}
}

void SWAG::ProcessorWorker::run() {
	while (true) {
		//Acquire item from queue
		RequestData data;
		_swag->_processingQueue.pop(data);

		//Process data (calc SHA1, hitcount, ...)
		_hasher.update(data.URI);
		auto URIHash = _hasher.final();
		_hasher.update(data.UserAgent);
		auto UAHash = _hasher.final();

		//TIP TYPE_HITCOUNT as type could be here
		decltype(_URIHitcount)::value_type::second_type URIHitcount;
		auto iterURI = _URIHitcount.find(data.URI);
		URIHitcount = (iterURI == _URIHitcount.cend()) ? (_URIHitcount[data.URI] = 1) : (++iterURI->second);
		
		decltype(_UserAgentHitcount)::value_type::second_type UserAgentHitcount;
		auto iterUserAgent = _UserAgentHitcount.find(data.UserAgent);
		UserAgentHitcount = (iterUserAgent == _UserAgentHitcount.cend()) ? (_UserAgentHitcount[data.UserAgent] = 1) : (++iterUserAgent->second);

		//Output
		_swag->_outputter.print(_produceLogMessage(data.URI, URIHash, URIHitcount, data.UserAgent, UAHash, UserAgentHitcount));
	}
}

string SWAG::ProcessorWorker::_produceLogMessage(
	const string& par_URI, const string& par_URIHash, TYPE_HITCOUNT par_URIHitcount,
	const string& par_UserAgent, const string& par_UserAgentHash, TYPE_HITCOUNT par_UserAgentHitcount) {

	//https://stackoverflow.com/questions/20619236/how-to-get-utc-time
	auto getUTCTime = []() -> string {
		char outstr[200];
		time_t t;
		struct tm* tmp;
		const char* fmt = "%a, %d %b %y %T %z";

		t = time(NULL);
		tmp = gmtime(&t);
		if (tmp == NULL) {
			return "<invalid UTC time>";
		}

		if (strftime(outstr, sizeof(outstr), fmt, tmp) == 0) {
			return "<invalid UTC time>";
		}

		return outstr;
	};

	//Acquire thread ID
	auto TID = this_thread::get_id();
	stringstream ssTID;
	ssTID << TID;
	string strTID = ssTID.str();

	string res = 
		string("T:") + getUTCTime()
		+ "  TID:" + strTID
		+ "  URI:" + par_URI
		+ "  URI_hash:" + par_URIHash
		+ "  URI_hitcount:" + to_string(par_URIHitcount) 
		
		+ "  UserAgent:"+ par_UserAgent 
		+ "  UserAgent_hash:" + par_UserAgentHash 
		+ "  UserAgent_hitcount:"+ to_string(par_UserAgentHitcount)

		+"\n";

	return res;
}
