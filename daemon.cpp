#include <iostream>
#include <thread>
#include <chrono>
#include <ifaddrs.h>
#include <net/if.h>
#include <cstdlib>
#include <signal.h>
#include <systemd/sd-daemon.h>
#include <mutex>
#include <condition_variable>
#include <atomic>

int main (int argc, char** argv) {
	std::condition_variable cv;
	std::mutex mut;
	std::atomic<bool> cont{true};
	sigset_t waitedSigs;
	try {
		sigemptyset(&waitedSigs);
		sigaddset(&waitedSigs, SIGTERM);
		sigprocmask(SIG_BLOCK, &waitedSigs, nullptr);
		sd_notify(0, u8"READY=1");
	}
	catch (std::exception& exc) { // Something went wrong.
		sd_notifyf(0, u8"STATUS=Failed to start up: %s\n ERRNO=%i", exc.what(), 1);
		return(1);
	}
	std::thread thr([&cv, &cont, &waitedSigs](){ 
			int sign;
			sigwait(&waitedSigs, &sign);
			sd_notify(0, u8"STOPPING=1");
			cont.store(false);
			cv.notify_all();
			});
	thr.detach();
	bool eth_running = false; // Is ethernet cable connected?
	while (cont.load()) {
		struct ifaddrs* addrs;
		if (getifaddrs(&addrs) == -1) { // Something went wrong...
			sd_notifyf(0, u8"STATUS=Failed to start up.\n ERRNO=1");
			return 1;
		}
		std::string eth;
		std::string wifi;
		if (argc > 1) {
			eth = argv[1];
			if (argc > 2) {
				wifi = argv[2];
			}
		}
		for (struct ifaddrs* ifa = addrs; ifa != nullptr; ifa = ifa->ifa_next) {
			if (eth == ifa->ifa_name) {
				if (ifa->ifa_flags & IFF_RUNNING) { // If running (cable connected)...
					if (!eth_running) { // And we didn't know about it...
						eth_running = true;
						std::system(u8"dhcpcd");
						if (cont.load()) {
							std::unique_lock<std::mutex> lock(mut);
							cv.wait_for(lock, std::chrono::seconds(3), [&cont]{return !cont.load();});
						}
						std::system((std::string(u8"iwctl device ") + wifi + std::string(u8" set-property Powered off")).c_str()); // Disable wifi.
					}
				}
				else if (eth_running) { // Else if we didn't know about disconnecting...
					eth_running = false;
					std::system((std::string(u8"iwctl device ") + wifi + std::string(u8" set-property Powered on")).c_str()); // Enable wifi.
				}
				break;
			}
		}
		freeifaddrs(addrs);
		if (cont.load()) {
			std::unique_lock<std::mutex> lock(mut);
			cv.wait_for(lock, std::chrono::seconds(3), [&cont]{return !cont.load();});
		}
	}
}
