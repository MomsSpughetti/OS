#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    struct sigaction sa;
    sa.sa_handler = alarmHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    if(signal(SIGALRM , alarmHandler) ==-1) {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.getName()<<"> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
        if(smash.isFinished() || smash.child()){
            break;
        }
    }
    return 0;
}