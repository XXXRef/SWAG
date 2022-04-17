#ifndef HEADER_THREADSAFE_QUEUE
#define HEADER_THREADSAFE_QUEUE

#include "_threadsafe_queue.hpp"

template<class T> ThreadsafeQueue<T>::ThreadsafeQueue() noexcept = default;

template<class T> ThreadsafeQueue<T>::ThreadsafeQueue(const ThreadsafeQueue& rhs) {
	std::lock_guard lg(rhs._m);

	_data = rhs._data;
}

template<class T> ThreadsafeQueue<T>& ThreadsafeQueue<T>::operator=(const ThreadsafeQueue& rhs) {
	if (&rhs == this) { return *this; }

	std::scoped_lock sl(_m, rhs._m);

	_data = rhs._data;

	return *this;
}

template<class T> ThreadsafeQueue<T>::ThreadsafeQueue(ThreadsafeQueue&& rhs) {
	std::lock_guard lg(rhs._m);

	_data = std::move(rhs._data);
}

template<class T> ThreadsafeQueue<T>& ThreadsafeQueue<T>::operator=(ThreadsafeQueue&& rhs) {
	std::scoped_lock sl(_m, rhs._m);

	_data = std::move(rhs._data);

	return *this;
}

template<class T> ThreadsafeQueue<T>::~ThreadsafeQueue() = default;


template<class T> inline std::size_t ThreadsafeQueue<T>::size() const {
	std::lock_guard lg(_m);

	return _data.size();
}

template<class T> inline bool ThreadsafeQueue<T>::empty() const {
	std::lock_guard lg(_m);

	return _data.empty();
}

template<class T> void ThreadsafeQueue<T>::push(const T& par_obj) {
	std::lock_guard lg(_m);

	_data.push_back(par_obj);

	_cv.notify_one();
}

template<class T> void ThreadsafeQueue<T>::push(T&& par_obj) {
	std::lock_guard lg(_m);

	_data.push_back(std::move(par_obj));

	_cv.notify_one();
}

template<class T> void ThreadsafeQueue<T>::pop(T& par_obj) {
	std::unique_lock ul(_m);
	_cv.wait(ul, 
		[this] { 
			return !this->_data.empty();
		}
	);
	par_obj = _data.front();
	_data.pop_front();
}

#endif
