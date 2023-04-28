#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <exception>
#include <sys/types.h>
#include <fcntl.h>
#include <bitset>
#include <sys/stat.h>
#include <iomanip>
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



#define _DEBUG 1
//#undef _DEBUG
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

static bool isComplexCommand(const std::string& cmdLine){
  for(auto& c : cmdLine){
    if(c == '?' || c =='*'){
      return true;
    }
  }
  return false;
}

/****************Built-in exec implemntaions****************/
void ChPromptCommand::execute(){
  std::string cmdLine = getCmdLine().c_str();
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(cmdLine.c_str(),args);
  SmallShell* smash = &SmallShell::getInstance();
  smash->setName((argsNum !=1) ? args[1] : "smash");
}

void ChangeDirCommand::execute(){//what if no args ?
  std::string cmdLine = getCmdLine();
  char* args[COMMAND_MAX_ARGS];
  int cmdLen = _parseCommandLine(cmdLine.c_str(),args);
  if(cmdLen > 2){
    std::cerr<<"smash error: cd: too many arguments"<<std::endl;
    return;
  }
  SmallShell& smash = SmallShell::getInstance();
  char buffer[COMMAND_ARGS_MAX_LENGTH];
  char* cwd = getcwd(buffer,COMMAND_ARGS_MAX_LENGTH);
  if(cwd == nullptr){
    perror("smash error: getcwd failed");
  }
  if(strcmp(args[1],"-") == 0){//go to the last pwd.
    if(smash.noDirHistory()){
      std::cerr<<"smash error: cd: OLDPWD not set"<<std::endl;
      return;
    }
    else{
      if(chdir(smash.getLastDir().c_str())==-1){
        perror("smash error: chdir failed");
        return;
      }
      smash.updateLastDir(cwd);
      return;
    }
  }
  else{
    if(chdir(args[1])==-1){
      perror("smash error: chdir failed");
      return;
    }
     smash.updateLastDir(cwd);
  }
}

void GetCurrDirCommand::execute(){
  char buffer[COMMAND_ARGS_MAX_LENGTH];
  char* cwd = getcwd(buffer,COMMAND_ARGS_MAX_LENGTH);
  if(cwd == nullptr){
    perror("smash error: getcwd failed");
  }
  std::cout<<cwd<<std::endl;
}

void ForegroundCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(),args);
  if(argsNum>2){
    std::cerr<<"smash error: fg: invalid arguments"<<std::endl;
    return;
  }
  if(jobsPtr->jobsNum() == 0){
    std::cerr<<"smash error: fg: jobs list is empty"<<std::endl;
    return;
  }
  int targetJID =(argsNum == 2) ? std::stoi(args[1]) : jobsPtr->getMaxJID();
  if(jobsPtr->getJobById(targetJID)==nullptr){
    std::cerr<<"smash error: fg: job-id "<<targetJID<<" does not exist"<<std::endl;
    return;
  }
  int targetPID = jobsPtr->getJobById(targetJID)->getPID();
  std::string targetCmdLine =  jobsPtr->getJobById(targetJID)->getCmd();

  //all above might better be preprocceed with a helper function in the constructor
  //In that case its fine but if we can run a built-in command in bg it will cause problems.
  if(kill(targetPID,SIGCONT) == -1){
    perror("smash error: kill failed");
    return;
  }
  jobsPtr->removeJobById(targetJID);
  std::cout<<targetCmdLine<<std::endl;
  waitpid(targetPID,nullptr,0);
  //check if second argument is a number
}

void BackgroundCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(),args);
  if(argsNum>2){
    std::cerr<<"smash error: bg: invalid arguments"<<std::endl;
    return;
  }

  int targetJID;
  JobsList::JobEntry* je = nullptr;
  if(argsNum == 2){
    try{
      je = jobsPtr->getJobById(std::stoi(args[1]));
    }catch(const std::out_of_range&){
      std::cerr<<"smash error: bg: invalid arguments"<<std::endl;
    }
    if(!isNum(args[1])){
      std::cerr<<"smash error: bg: invalid arguments"<<std::endl;
    }
    if(je == nullptr){
      std::cerr<<"smash error: bg: job-id "<< args[1]<<" does not exist"<<std::endl;
      return;
    }
    if(!je->stopped()){
      std::cerr<<"smash error: bg: job-id "<< args[1]<<" is already running in the background"<<std::endl;
      return;
    }
  }

  if(argsNum == 1){
    je = jobsPtr->getLastStoppedJob(&targetJID);
    if(je == nullptr){
      std::cerr<<"smash error: bg: there is no stopped jobs to resume"<<std::endl;
      return;
    }
  }
  targetJID = je->getJID();
  int targetPID = je->getPID();
  std::cout<< jobsPtr->getJobById(targetJID)->getCmd()<<std::endl;
  if(kill(targetPID,SIGCONT) == -1){
    perror("smash error: kill failed");
    return;
  }
  je->run();
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
    std::cerr<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  std::string sigStr = args[1];
  int sigNum = UNINITIALIZED;
  int JID = UNINITIALIZED;

  try{
    sigNum = std::stoi(sigStr.substr(1,sigStr.size()+1));
    JID = std::stoi(args[2]);
  }catch(const std::out_of_range&){
    std::cerr<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  if(sigNum > 31 || sigNum <0 || JID == -1){
    std::cerr<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  JobsList::JobEntry* je = jobsPtr->getJobById(JID);
  if(je == nullptr){
    std::cerr<<"smash error: kill: job-id "<<JID<<" does not exist"<<std::endl;
    return;
  }
  if(kill(je->getPID(),sigNum)==-1){
    perror("smash error: kill failed");
    return;
  }
  if(sigNum == SIGSTOP){
    je->stop();
  }
  std::cerr<<"signal number "<<sigNum<<" was sent to pid " <<je->getPID()<<std::endl;
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
  int PID = (child()) ? 0 : fork();
  if(PID == 0){
    if(isComplexCommand(cmdLine)){
        if(execlp("/bin/bash","/bin/bash","-c",cmdLine.c_str(),nullptr) == -1){
          perror("smash error: execlp failed");
          smash.quit();
          return;
        }
    }
    else{
      if(execvp(args[0],args) == -1){
        perror("smash error: execvp failed");
        smash.quit();
        return;
      }
    }
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
/************************************************************/


static std::string createCmdLine(char** args,int start,int end){
  std::string cmdLine = "";
  for(int i=start;i<end;++i){
    cmdLine+=args[i];
    if(i !=end -1){
      cmdLine+=" ";
    }
  }
  return cmdLine;
}

static std::string findRedirectionPip(char** args,int argNum,int* index = nullptr){
   if(index !=nullptr) *(index) = -1;
  for(int i=0; i<argNum;++i){
    if(strcmp(args[i],">") == 0){
      if(index !=nullptr) (*index)= i;
      return ">";
    }
    if(strcmp(args[i],">>") == 0){
      if(index !=nullptr) (*index)= i;
      return ">>";
    }
    if(strcmp(args[i],"|") == 0){
      if(index !=nullptr) (*index)= i;
      return "|";
    }
    if(strcmp(args[i],"|&") == 0){
      if(index !=nullptr) (*index)= i;
      return "|&";
    }
  }
  return "";
}


void RedirectionCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(getCmdLine().c_str(),args);
  int i;
  std::string redirectionCommand = findRedirectionPip(args,argsNum,&i);
  if( i == argsNum -1  || i==0){//there is no right or left cmd
    return; //what error to print?
  }
  SmallShell& smash=SmallShell::getInstance();
  std::string cmd1 = createCmdLine(args,0,i);
  std::string cmd2 = createCmdLine(args,i+1,argsNum); //might need to change
  int PID = fork();
  if(PID == 0){
    close(STDOUT_FILENO);
    if(redirectionCommand == ">"){
      open(cmd2.c_str(), O_RDWR | O_CREAT | O_TRUNC); //TODO Do we need to clean the file?
    }
    else{//redirection == ">>"
      open(cmd2.c_str(),O_RDWR | O_CREAT | O_APPEND);
    }
  smash.executeCommand(cmd1.c_str(),true);
  }
  else{
    waitpid(PID,nullptr,0);
  }
}



PipeCommand::PipeCommand(const char * cmd_line) : Command(cmd_line){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(getCmdLine().c_str(),args);
  int i;
  std::string str = findRedirectionPip(args, argsNum, &i);
  SmallShell& smash=SmallShell::getInstance();

  std::string cmd1 = createCmdLine(args,0,i);
  std::string cmd2 = createCmdLine(args,i+1,argsNum); //might need to change
  cmd1.c_str();
  cmd2.c_str();

  char cmd1NoBSign[cmd1.size()+1];
  char cmd2NoBSign[cmd2.size()+1];

  cmd1NoBSign[cmd1.size()] = '\0';
  cmd2NoBSign[cmd2.size()] = '\0';

  strcpy(cmd1NoBSign,cmd1.c_str());
  strcpy(cmd2NoBSign,cmd2.c_str());

  _removeBackgroundSign(cmd1NoBSign);
  _removeBackgroundSign(cmd2NoBSign);

  sign = str;
  signPlace = (i == 0 || i == argsNum - 1)? -1 : i;
  this->cmd1 = cmd1NoBSign;
  this->cmd2 = cmd2NoBSign;
  }

void PipeCommand::execute()
{
  char* args1[COMMAND_MAX_ARGS];
  char* args2[COMMAND_MAX_ARGS];

  int args1Num = _parseCommandLine(cmd1.c_str(),args1);
  int args2Num = _parseCommandLine(cmd2.c_str(),args2);

  if( signPlace == -1){//there is no right or left cmd
    return; //what error to print?
  }

  int fd[2];
  pipe(fd);
  int channel = (this->sign == "|")? 1 : (this->sign == "|&")? 2 : -1;

  if(channel == -1){
    std::cerr << "no such piping sign!!!" << endl;
  }

  SmallShell& smash=SmallShell::getInstance();
  

  int PID = fork();
  
  if (PID == 0) {
    // first child 
    dup2(fd[1],channel);
    close(fd[0]);
    close(fd[1]);
    //execv(args1[0], args1);
    smash.executeCommand(cmd1.c_str(),true);
  }

  if (fork() == 0 && PID > 0) { 
    // second child 
    dup2(fd[0],0);
    close(fd[0]);
    close(fd[1]);
    //execv(args2[0], args2);
    smash.executeCommand(cmd2.c_str(),true);
  }
  close(fd[0]);
  close(fd[1]);
  }

void SetcoreCommand::execute(){
  char * args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(), args);

  //are arguments valid?
  if(argsNum != 3){
    std::cerr << "smash error: setcore: invalid arguments" << endl;
    return;
  }

  int coreNum = -1, JID;
  try
  {
    coreNum = std::stoi(args[2]);
    JID = std::stoi(args[1]);
  }
  catch(const std::out_of_range& e){
    std::cerr << "smash error: setcore: invalid arguments" << endl;
    return;
  }
  if(!isNum(args[1]) || !isNum(args[2])){
    std::cerr << "smash error: setcore: invalid arguments" << endl;
    return;
  }

  //get the PID of the process with JID
  SmallShell& smash = SmallShell::getInstance();
  int PID = smash.getJobPID(JID);

  if(PID == -1){
    std::cerr << "smash error: setcore: job-id "<< JID <<" does not exist" << endl;
    return;
  }

  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet); // clear the set of CPUs
  CPU_SET(coreNum, &cpuSet); // add CPU core 1 to the set

  if (sched_setaffinity(PID, sizeof(cpuSet), &cpuSet) == -1) {
    std::cerr << "smash error: setcore: invalid core number" << std::endl;
    return;
  }
}

void GetFileTypeCommand::execute(){
  char * args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(), args);
  struct stat file_stat;

  if(argsNum != 2 || stat(args[1], &file_stat) != 0){
    std::cerr << "smash error: gettype: invalid aruments" << endl;
    return;
  } 

  std::string type_;

    if (S_ISREG(file_stat.st_mode)) {
        type_ = "regular file";
    } else if (S_ISDIR(file_stat.st_mode)) {
        type_ = "directory";
    } else if (S_ISLNK(file_stat.st_mode)) {
        type_ = "symbolic link";
    } else if (S_ISCHR(file_stat.st_mode)) {
        type_ = "character device";
    } else if (S_ISBLK(file_stat.st_mode)) {
        type_ = "block device";
    } else if (S_ISFIFO(file_stat.st_mode)) {
        type_ = "FIFO";
    } else if (S_ISSOCK(file_stat.st_mode)) {
        type_ = "socket";
    }
    std::cout << args[1] << "'s type is “"<< type_ <<"” and takes up " << file_stat.st_size <<" bytes"<<std::endl;
}

static mode_t int_to_mod(int val){
  std::string perm;
  mode_t mode = 0;
  int uga[3];

  for (int i = 0; i < 3; i++)
  {
    uga[i] = val % 10;
    val /= 10;
  }
  
  std::string usr = std::bitset<3>(uga[2]).to_string(); //to binary
  std::string grp = std::bitset<3>(uga[1]).to_string(); //to binary
  std::string all = std::bitset<3>(uga[0]).to_string(); //to binary
  
  perm += usr;
  perm += grp;
  perm +=all;

  if (perm[0] == '1')
    mode |= S_IRUSR;
  if (perm[1] == '1')
    mode |= S_IWUSR;
  if (perm[2] == '1')
    mode |= S_IXUSR;
  if (perm[3] == '1')
    mode |= S_IRGRP;
  if (perm[4] == '1')
    mode |= S_IWGRP;
  if (perm[5] == '1')
    mode |= S_IXGRP;
  if (perm[6] == '1')
    mode |= S_IROTH;
  if (perm[7] == '1')
    mode |= S_IWOTH;
  if (perm[8] == '1')
    mode |= S_IXOTH;

  return mode;
}

void ChmodCommand::execute(){
  char * args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(), args);
  int mod;

   //check is valid
   try
   {
      mod = std::stoi(args[1]);
   }catch(const std::exception& e){
      std::cerr << e.what() << '\n';
   }
   
   if(argsNum != 3||!isNum(args[1])){
    std::cerr << "smash error: chmod: invalid aruments";
    return;
   }

   mode_t modi = int_to_mod(mod);
  int chmodResult = chmod(args[2], modi);
  if(chmodResult != 0){
    std::cerr << "smash error: chmod failed" << endl;
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
  int status, pid;
  auto findJIDByPID = [this](int pid){
        for(auto& job : jobs){
          if(job->getPID() == pid){
            return job->getJID();
          }
        }
        return 0;
  };

  do{
    pid= waitpid(-1,nullptr,WNOHANG);
    if(pid > 0){
      toRmStack.push(findJIDByPID(pid));
    }
    else{
      break;
    }
  }while(pid !=0);

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
      jobs.erase(it);
      delete *it;
      maxJID = findMaxJID();
      return;
    }
  } 
}

void JobsList::killAllJobs(){
  for(auto &job : jobs){
    if(kill(job->getPID(),SIGKILL) == -1){
      perror("smash error: kill failed");
      return;
    }
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
Command* SmallShell::CreateCommand(const char* cmd_line, bool isChild) {
  if(cmd_line == nullptr){
    return nullptr;
  }
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  char builtInCmdLine[strlen(cmd_line)+1];
  builtInCmdLine[strlen(cmd_line)] = '\0';
  strcpy(builtInCmdLine,cmd_line);
  _removeBackgroundSign(builtInCmdLine);
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(cmd_line,args);
  std::string redirectionCommand = findRedirectionPip(args,argsNum);
  SmallShell& smash = SmallShell::getInstance();
  if(isChild){
    smash.setChild();
  }
  if(firstWord.compare("chmod") == 0){
    return new ChmodCommand(builtInCmdLine);
  }
  if(redirectionCommand == "|" || redirectionCommand == "|&"){
    return new PipeCommand(cmd_line);
  }
  if(redirectionCommand == ">" || redirectionCommand == ">>" ){
    return new RedirectionCommand(cmd_line);
  }
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
  if (firstWord.compare("setcore") == 0) {
    return new SetcoreCommand(builtInCmdLine);
  }
  if (firstWord.compare("getfiletype") == 0) {
    return new GetFileTypeCommand(builtInCmdLine);
  }
  if (firstWord.compare("chmod") == 0) {
    return new ChmodCommand(builtInCmdLine);
  }
  return new ExternalCommand(cmd_line,isChild);
}

void SmallShell::executeCommand(const char *cmd_line, bool isChild) {
  jobs.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line,isChild);
  if(cmd != nullptr){
    cmd->execute();
  }
  delete cmd;
}