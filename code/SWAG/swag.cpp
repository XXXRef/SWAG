#include "swag.hpp"

#include "sha1.hpp"

#include <Windows.h>
#include <cstdlib>

using namespace std;

SWAG::SWAG() = default;

void SWAG::run() {
	//Fire up processor workers in separate threads
	ProcessorWorker testProcessorWorker = SWAG::ProcessorWorker(this);
	thread(&SWAG::ProcessorWorker::run, &testProcessorWorker).detach();

	ProcessorWorker testProcessorWorker1 = SWAG::ProcessorWorker(this);
	thread(&SWAG::ProcessorWorker::run, &testProcessorWorker1).detach();

	ProcessorWorker testProcessorWorker2 = SWAG::ProcessorWorker(this);
	thread(&SWAG::ProcessorWorker::run, &testProcessorWorker2).detach();

	//Fire up web worker in current thread
	WebWorker(this).run();
}

//==========Workers
void SWAG::WebWorker::run() {
	while (true) {
		_swag->_processingQueue.push({ "TEST URI", "TEST USERAGENT" });
		::Sleep(5000);
	}
}

void SWAG::ProcessorWorker::run() {
	SHA1 hasher;

	while (true) {
		//Acquire item from queue
		RequestData data;
		_swag->_processingQueue.pop(data);

		//Process data (calc SHA1, hitcount, ...)
		hasher.update(data.URI);
		auto URIHash = hasher.final();
		hasher.update(data.UserAgent);
		auto UAHash = hasher.final();

		//Output
		_swag->_outputter.print(_produceLogMessage(data.URI, URIHash, 5, data.UserAgent, UAHash, 10));
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
