#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <exception>
using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
#define UNINITIALIZED -1;
#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

static bool isNum(const std::string& str){
    for(auto& c : str){
      if(c< '0' || c > '9'){
        return false;
      }
    }
  return true;
}


/****************Built-in exec implemntaions****************/
void ChPromptCommand::execute(){
  std::string cmdLine = getCmdLine().c_str();
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(cmdLine.c_str(),args);
  SmallShell* smash = &SmallShell::getInstance();
  smash->setName((argsNum !=1) ? args[1] : "smash");
}

void ChangeDirCommand::execute(){
  std::string cmdLine = getCmdLine();
  char* args[COMMAND_MAX_ARGS];
  int cmdLen = _parseCommandLine(cmdLine.c_str(),args);
  if(cmdLen > 2){
    std::cout<<"smash error: cd: too many arguments"<<std::endl;
    return;
  }
  SmallShell& smash = SmallShell::getInstance();
  char buffer[COMMAND_ARGS_MAX_LENGTH];
  char* pwd = getcwd(buffer,COMMAND_ARGS_MAX_LENGTH);
  if(strcmp(args[1],"-") == 0){//go to the last pwd.
    if(smash.noDirHistory()){
      std::cout<<"smash error: cd: OLDPWD not set"<<std::endl;
      return;
    }
    else{
      chdir(smash.getLastDir().c_str());
      smash.rmLastDir();
      return;
    }
  }
  else{
    chdir(args[1]);
    smash.recordDir(std::string(pwd));
  }
}

void GetCurrDirCommand::execute(){
  char buffer[COMMAND_ARGS_MAX_LENGTH];
  char* cwd = getcwd(buffer,COMMAND_ARGS_MAX_LENGTH);
  std::cout<<cwd<<std::endl;
}

void ForegroundCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(),args);
  if(argsNum>2){
    std::cout<<"smash error: fg: invalid arguments"<<std::endl;
    return;
  }
  if(jobsPtr->jobsNum() == 0){
    std::cout<<"smash error: fg: jobs list is empty"<<std::endl;
    return;
  }
  int targetJIT =(argsNum == 2) ? std::stoi(args[1]) : jobsPtr->getMaxJID();
  if(jobsPtr->getJobById(targetJIT)==nullptr){
    std::cout<<"smash error: fg: job-id "<<targetJIT<<" does not exist"<<std::endl;
    return;
  }
  int targetPID = jobsPtr->getJobById(targetJIT)->getPID();
  std::string targetCmdLine =  jobsPtr->getJobById(targetJIT)->getCmd();

  //all above might better be preprocceed with a helper function in the constructor
  //In that case its fine but if we can run a built-in command in bg it will cause problems.
  kill(targetPID,SIGCONT); // should not return -1
  jobsPtr->removeJobById(targetJIT);
  std::cout<<targetCmdLine<<std::endl;
  waitpid(targetPID,nullptr,0);
}

void BackgroundCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(),args);
  if(argsNum>2){
    std::cout<<"smash error: bg: invalid arguments"<<std::endl;
    return;
  }
  int targetJIT;
  JobsList::JobEntry* je = nullptr;
  if(argsNum == 2){
    je = jobsPtr->getJobById(std::stoi(args[1]));
    if(je == nullptr){
      std::cout<<"smash error: bg: job-id "<< args[1]<<" does not exist"<<std::endl;
      return;
    }
    if(!je->stopped()){
      std::cout<<"smash error: bg: job-id "<< args[1]<<" is already running in the background"<<std::endl;
    }
  }
  if(argsNum ==1){
    je = jobsPtr->getLastStoppedJob(&targetJIT);
    if(je == nullptr){
      std::cout<<"smash error: bg: there is no stopped jobs to resume"<<std::endl;
      return;
    }
  }
  targetJIT = je->getJID();
  int targetPID = je->getPID();
  std::string targetCmdLine =  jobsPtr->getJobById(targetJIT)->getCmd();
  kill(targetPID,SIGCONT); // should not return -1
  je->run();
  std::cout<<targetCmdLine<<std::endl;
}

void QuitCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(),args);
  jobsPtr->killAllJobs();
  if(argsNum>=2 && (strcmp(args[1],"kill") == 0)){
    std::cout<<"smash: sending SIGKILL signal to "<<jobsPtr->jobsNum()<<" jobs:"<<std::endl;
    jobsPtr->printForQuit();
  }
  jobsPtr->quit();
  SmallShell& smash = SmallShell::getInstance();
  smash.quit();
}

void JobsCommand::execute(){
  jobsPtr->printJobsList();
}

void KillCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(),args);
  if(argsNum != 3 || args[1][0] != '-' ||!isNum(args[2])){
    std::cout<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  std::string sigStr = args[1];
  int sigNum = UNINITIALIZED;
  int JID = UNINITIALIZED;

  try{
    sigNum = std::stoi(sigStr.substr(1,sigStr.size()+1));
    JID = std::stoi(args[2]);
  }catch(const std::out_of_range&){
    std::cout<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  if(sigNum > 31 || sigNum <0 || JID == -1){
    std::cout<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  JobsList::JobEntry* je = jobsPtr->getJobById(JID);
  if(je == nullptr){
    std::cout<<"smash error: kill: job-id "<<JID<<" does not exist"<<std::endl;
    return;
  }
  kill(je->getPID(),sigNum);
  std::cout<<"signal number "<<sigNum<<" was sent to pid " <<je->getPID()<<std::endl;
}

/********************ExternalCommands********************/
void ExternalCommand::execute(){
  std::string cmdLine = getCmdLine();
  bool bg = _isBackgroundComamnd(cmdLine.c_str());
  char cmdNoBg[cmdLine.size()+1];
  cmdNoBg[cmdLine.size()] = '\0';
  strcpy(cmdNoBg,cmdLine.c_str());
  _removeBackgroundSign(cmdNoBg);
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(cmdNoBg,args);
  SmallShell& smash = SmallShell::getInstance();
  int PID = fork();
  if(PID ==0){
    execvp(args[0],args);
  }
  if(PID > 0){
    if(bg){
      smash.addJop(PID,getCmdLine());
    }
    else{
      waitpid(PID,nullptr,0);
    }
  }
}



/****************************Jobs****************************/
int JobsList::findMaxJID() const{
  int max = UNINITIALIZED;
  for(auto& job : jobs){
    if(job->getJID() > max){
      max = job->getJID();
    }
  }
  return max;
}

void JobsList::addJob(const std::string& cmdLine,int PID, bool isStopped){
  removeFinishedJobs();
  int JID = (jobs.size() == 0) ? 1 : maxJID+1;
  maxJID=JID;
  JobEntry* je = new JobEntry(JID,PID,cmdLine,isStopped);
  jobs.push_back(je);
  je->startTimer();
}

void JobsList::printJobsList(){
  removeFinishedJobs();
  for(auto& job : jobs){
      std::cout<<"["<<job->getJID()<<"]"<<" ";
      std::cout<<job->getCmd()<<" ";
      std::cout<<":"<<" ";
      std::cout<<job->getPID()<<" ";
      std::cout<<difftime(time(nullptr),job->startingTime)<<" secs ";
      if(job->stopped()){
        std::cout<<"(stopped)";
      }
      std::cout<<std::endl;
  }
}

void JobsList::printForQuit(){
  for(auto& job : jobs){
      std::cout<<job->getPID()<<": ";
      std::cout<<job->getCmd()<<std::endl;
  }
}

void JobsList::removeFinishedJobs(){
  stack<int> toRmStack;
  int status;
  for(auto& job : jobs){
    int pid = waitpid(job->getPID(),&status,WNOHANG);
    if(pid){
      toRmStack.push(job->getJID());
    }
  }
  while(!toRmStack.empty()){
    removeJobById(toRmStack.top());
    toRmStack.pop();
  }
}

JobsList::JobEntry* JobsList::getJobById(int JID){
  for(auto& job : jobs){
    if (job->getJID() == JID){
      return job;
    }
  }
  return nullptr;
}

void JobsList::quit(){
  while(!jobs.empty()){
      delete jobs.back();
      jobs.pop_back();
  }
}

void JobsList::removeJobById(int JID){
  for (std::list<JobEntry*>::iterator it = jobs.begin(); it != jobs.end(); ++it){
    if((*it)->getJID() == JID){
      delete *it;
      jobs.erase(it);
      maxJID = findMaxJID();
      return;
    }
  } 
}

void JobsList::killAllJobs(){
  for(auto &job : jobs){
    kill(job->getPID(),SIGKILL);
  }
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJIDptr){
  JobEntry* jp = jobs.back();
  *lastJIDptr = jp->getJID();
  return jp;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *JIDptr){
  for (std::list<JobEntry*>::reverse_iterator it = jobs.rbegin(); it != jobs.rend(); ++it){
    if((*it)->stopped() == true){
      *JIDptr = (*it)->getJID();
      return (*it);
    }
  }
  return nullptr;
}





void SmallShell::addJop(int PID, const std::string& cmdLine){
  jobs.addJob(cmdLine,PID);
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line) {
  if(cmd_line == nullptr){
    return nullptr;
  }
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  char builtInCmdLine[strlen(cmd_line)+1];
  builtInCmdLine[strlen(cmd_line)] = '\0';
  strcpy(builtInCmdLine,cmd_line);
  _removeBackgroundSign(builtInCmdLine);
  if (firstWord.compare("chprompt") == 0) {
    return new ChPromptCommand(builtInCmdLine);
  }
  if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(builtInCmdLine);
  }
  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(builtInCmdLine);
  }
  if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(builtInCmdLine);
  }
  if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(builtInCmdLine,&jobs);
  }
  if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(builtInCmdLine,&jobs);
  }
  if (firstWord.compare("bg") == 0) {
    return new BackgroundCommand(builtInCmdLine,&jobs);
  }
  if (firstWord.compare("quit") == 0) {
    return new QuitCommand(builtInCmdLine,&jobs);
  }
  if (firstWord.compare("kill") == 0) {
    return new KillCommand(builtInCmdLine,&jobs);
  }
  return new ExternalCommand(cmd_line);
}

void SmallShell::executeCommand(const char *cmd_line) {
  jobs.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);
  if(cmd != nullptr){
    cmd->execute();
  }
  delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}