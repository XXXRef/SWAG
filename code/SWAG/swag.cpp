#include "swag.hpp"

#include <sstream>
#include <cstdlib>

#if defined(WIN32)
	#include <WinSock2.h>

	#pragma comment(lib, "ws2_32.lib")
#elif defined(__linux__)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <errno.h>
	#include <string.h>
	#include <sys/types.h>
	#include <time.h>
#endif

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
	constexpr TYPE_PORT SWAG_PORT = 8888;
	WebWorker(this, SWAG_PORT).run();
}

//==========Workers
void SWAG::WebWorker::run() {
	//init socket
	sockaddr_in address{0};
	int addrlen = sizeof(address);
	int sockfd;

	constexpr auto REQUEST_BUFFER_SIZE = 0x1000;
	char requestBuffer[REQUEST_BUFFER_SIZE]{0};
	const std::string successResponseBuffer = "HTTP/1.1 200 OK\r\n";
	const std::string failureResponseBuffer = "HTTP/1.1 400 Bad Request\r\n";

#ifdef WIN32
	WSADATA wsaData;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		//TODO add error handling
		_swag->_outputter.print(__FUNCTION__": [ERROR] WSAStartup() failed\n");
		return;
	}
#endif

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (0 == sockfd) {
		_swag->_outputter.print(__FUNCTION__": [ERROR] socket() failed\n");
		return;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	//TODO place port value somewhere else
	address.sin_port = htons((unsigned short)_port);
	if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
		_swag->_outputter.print(__FUNCTION__": [ERROR] bind() failed ");
		_swag->_outputter.print(std::to_string(WSAGetLastError())+"\n");
		return;
	}

	constexpr auto SOCK_QUEUE_LIMIT = 5;
	//TODO add error handling
	listen(sockfd, SOCK_QUEUE_LIMIT);
	
	while (true) {
		//Init acceptor socket
		int hClientSocket = accept(sockfd, (struct sockaddr*)&address, &addrlen);
		if (0 == hClientSocket) {
			_swag->_outputter.print(__FUNCTION__": [ERROR] accept() failed\n");
			return;
		}

		//Aquire request contents
		auto recvSize = recv(hClientSocket, requestBuffer, sizeof(requestBuffer), 0);
		 
		//Response on request
		const char* PROCESS_METHOD_PATTERN = "GET ";
		if (std::strncmp(requestBuffer, PROCESS_METHOD_PATTERN, strlen(PROCESS_METHOD_PATTERN))) {
			send(hClientSocket, failureResponseBuffer.data(), failureResponseBuffer.size(), 0);
			closesocket(hClientSocket);
			continue;
		}

		//Parse data
		istringstream sstr{ std::string(requestBuffer, recvSize) };
		std::string curLine;

		//Get URI
		std::getline(sstr, curLine, '\n');
		auto requestURI = std::string(
			curLine.cbegin() + curLine.find_first_of(' ') + 1,
			curLine.cbegin() + curLine.find_last_of(' ')
		);

		//Get User-Agent
		std::string UserAgent;
		bool flagUserAgentFound=false;
		const char* USERAGENT_HEADER_PATTERN = "User-Agent:";
		while (std::getline(sstr, curLine, '\n')) {
			if (curLine == "\r\n") { break; } //blank line - footer section end
			if (curLine.find(USERAGENT_HEADER_PATTERN)==0) {
				//UserAgent = curLine.substr(curLine.find_first_of(' ') + 1, curLine.length()-1); // assuming '\r\n' in the end of every line
				UserAgent = std::string(
					curLine.cbegin()+ curLine.find_first_of(' ') + 1,
					curLine.cend()-1
				);
				flagUserAgentFound = true;
				break;
			}
		}

		if (!flagUserAgentFound) {
			send(hClientSocket, failureResponseBuffer.data(), failureResponseBuffer.size(), 0);
			closesocket(hClientSocket);
			continue;
		}

		send(hClientSocket, successResponseBuffer.data(), successResponseBuffer.size(), 0);
		closesocket(hClientSocket);

		if (UserAgent.size() != std::strlen(UserAgent.data())) {
			UserAgent = UserAgent.substr(0, std::strlen(UserAgent.data()));
		}

		//Store data
		_swag->_processingQueue.push({ requestURI, UserAgent });
	}

#ifdef WIN32
	WSACleanup();
#endif
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
