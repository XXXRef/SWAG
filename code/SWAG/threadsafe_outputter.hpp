#ifndef HEADER_THREADSAFE_OUTPUTTER
#define HEADER_THREADSAFE_OUTPUTTER

#include <mutex>
#include <ostream>
#include <iostream>

class ThreadsafeOutputter {
public:
	ThreadsafeOutputter(std::ostream& par_outStream = std::cout) noexcept;

	//TODO out stream object setter

	void print(const std::string&) const;

private:
	mutable std::mutex _m;

	std::ostream* _pOutStream;
};

#endif