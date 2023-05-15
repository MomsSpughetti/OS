#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {

    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    if(smash.getPID() == -1){
        perror("smash error: getpid failed");
        return 0;
    }
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
