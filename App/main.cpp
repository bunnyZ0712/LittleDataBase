#include <iostream>
#include <string>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include "NetCon.h"

static void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{
	pid_t pid;
	int status;
	pid = waitpid(-1, &status, WNOHANG);
	std::cout << "回收" << pid << std::endl;
}

int main()
{
	struct sigaction sa;
	sa.sa_sigaction = ngx_signal_handler;
	sa.sa_flags = SA_SIGINFO;           //可以传入更多信息
	sigemptyset(&sa.sa_mask);
	int ret = sigaction(SIGCHLD, &sa, nullptr);

	// std::cout << ret << std::endl;
	std::shared_ptr<SockServer> sock = SockServer::GetInstance();

	std::thread work(&SockServer::ListenWork, sock.get());
	work.detach();
	sock ->Work();

    return 0;
}