#include <deque>
#include <mutex>
#include <condition_variable>

template<class T> class ThreadsafeQueue {
public:
	ThreadsafeQueue() noexcept;
	ThreadsafeQueue(const ThreadsafeQueue&);
	ThreadsafeQueue& operator=(const ThreadsafeQueue&);
	ThreadsafeQueue(ThreadsafeQueue&&);
	ThreadsafeQueue& operator=(ThreadsafeQueue&&);
	virtual ~ThreadsafeQueue();

	std::size_t size() const;
	bool empty() const;

	void push(const T&);
	void push(T&&);
	void pop(T&);

protected:
	//TIP always mark mutex objects as mutable - very helpful in copy-control members
	mutable std::mutex _m;
	std::condition_variable _cv;

	std::deque<T> _data;
};

