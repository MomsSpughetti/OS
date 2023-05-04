#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  std::cout<<"smash: got ctrl-Z"<<std::endl;
  SmallShell& smash = SmallShell::getInstance();
  int currentProcess = smash.getCurrentProcess();
  if(currentProcess > 0){
    if(kill(currentProcess,SIGSTOP) == -1){
      perror("smash error: kill failed");
      return;
    }
    smash.addJob(currentProcess,smash.getCurrentCommand(),true);
    smash.setCurrentProcess(-1);
    smash.setCurrentCommand("");
    std::cout<<"smash: process "<<currentProcess<<" was stopped"<<std::endl;
  }
  return;
}

void ctrlCHandler(int sig_num) {
  std::cout<<"smash: got ctrl-C"<<std::endl;
  SmallShell& smash = SmallShell::getInstance();
  int currentProcess = smash.getCurrentProcess();
  int JID;
  if(currentProcess > 0){
    if(kill(currentProcess,SIGKILL) == -1){
      perror("smash error: kill failed");
      return;
  }
  smash.removeJob(smash.getJobJID(currentProcess));
  std::cout<<"smash: process "<<currentProcess<<" was killed"<<std::endl;
  }
  return;
}
  

void alarmHandler(int sig_num) {
  std::cout<<"smash: got an alarm"<<std::endl;
  
  SmallShell& smash = SmallShell::getInstance();
  TimedJob toKill = smash.getTimedListHead();
  int PID = toKill.getPID();
  int JID = smash.getJobJID(PID);
  if(PID > 0){
    std::cout<<PID<<std::endl;
    if(kill(PID,SIGKILL) == -1){
      perror("smash error: kill failed");
      return;
     }
  if(JID == -1){
   std::cout<<"timeout "<<toKill.getDuration()<<" "<<smash.getCurrentCommand()<<" timed out"<<std::endl;
  }else{
   std::cout<<"timeout "<<toKill.getDuration()<<" "<<smash.getJobCmdLine(JID)<<" timed out"<<std::endl;
  }
  smash.removeJob(smash.getJobJID(PID));
  smash.popTimedJobsList();
  }
}

