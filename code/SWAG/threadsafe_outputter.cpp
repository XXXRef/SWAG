#include "threadsafe_outputter.hpp"

using namespace std;

ThreadsafeOutputter::ThreadsafeOutputter(std::ostream& par_outStream) noexcept : _pOutStream(&par_outStream) {}

//TODO inline?
void ThreadsafeOutputter::print(const std::string& par_msg) const {
	lock_guard lg(_m);

	*_pOutStream << par_msg << endl;
}
