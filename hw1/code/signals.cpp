#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include "sys/wait.h"
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
    smash.stopJob(currentProcess);
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
  while(smash.TimedJobsNum() != 0){
    TimedJob toKill = smash.getTimedListHead();
    int PID = toKill.getPID();
    int JID = smash.getJobJID(PID);
    time_t finishingTime = toKill.getFinisingTime();
    if(finishingTime <= time(nullptr)){
      if(toKill.isFinished() || waitpid(PID,nullptr,WNOHANG) > 0){
        smash.removeJob(JID);
        smash.popTimedJobsList();
        continue;
      }
      if(isBuiltIn(toKill.getCmdLine()) ){
         smash.popTimedJobsList();
         continue;
      }
      if(JID != -1 && kill(PID,SIGKILL) == -1){
      perror("smash error: kill failed");
      return;
      }
      if(smash.getCurrentProcess() == PID){
       smash.removeJob(JID);
       smash.setCurrentProcess(-1);
       smash.setCurrentCommand("");
      }
      std::cout<<"smash: "<<smash.getTimedJobCmd(toKill)<<" timed out!"<<std::endl; 
      smash.popTimedJobsList();
    }else{
      break;
    }
  }
  time_t newDuration = (smash.TimedJobsNum()!=0) ? (smash.getTimedListHead().getFinisingTime() - time(nullptr)) : 0;
  if(newDuration != 0){
    if(alarm(newDuration) == -1){
      perror("smash error: alarm failed"); 
    }
  }
}
  


