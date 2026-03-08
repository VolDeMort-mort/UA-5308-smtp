#pragma once
#include <queue>
#include <functional>
#include <condition_variable>
#include <mutex>

class ThreadPool {
private:
	std::queue<std::function<void()>> tasks;
	std::mutex mutex;
	std::condition_variable cv;
	std::vector<std::thread> workers;
	int numOfWorkers = 0;
	bool stop = false;
	bool paused = false;

	void StartWorkers();
	void WorkerLoop();
	void JoinWorkers();
public:
	~ThreadPool() {
		Terminate();
	}
	bool Initialize(int workersNum);
	bool Terminate();
	bool AddTask(std::function<void()> task);
	bool Pause();
	bool Resume();
};