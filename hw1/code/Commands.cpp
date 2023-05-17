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

bool isBuiltIn(const std::string& str){
  return (str=="chmod"||str=="cd"||str=="showpid"||str=="bg"||str=="fg"||
          str=="pwd"||str == "setcore"||str=="kill"||str=="quit"||str=="jobs"||
          str=="chprompt");
}

static bool isNum(const std::string& str){
  int i=0;
    for(auto& c : str){
      if(i==0 && c == '-'){
        if(str.size()==1){
          return false;
        }
        else{
          ++i;
          continue;
        }
      }
      if(c< '0' || c > '9'){
        return false;
      }
      ++i;
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

void ShowPidCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    if(child()){
      std::cout<<"smash pid is "<<smash.getPID()<< std::endl;
    }
    else{
      int PID = getpid();
      if(PID == -1){
        perror("smash error: getpid failed");
        return;
      }
      std::cout<<"smash pid is "<< PID << std::endl;
    }
}

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
  
  if(argsNum>2 || (argsNum == 2 && !isNum(args[1]))){
    std::cerr<<"smash error: fg: invalid arguments"<<std::endl;
    return;
  }
  if(jobsPtr->jobsNum() == 0 && argsNum ==1){
    std::cerr<<"smash error: fg: jobs list is empty"<<std::endl;
    return;
  }
  int targetJID = (argsNum == 2) ? std::stoi(args[1]) : jobsPtr->getMaxJID();
  if(jobsPtr->getJobById(targetJID)==nullptr){
    std::cerr<<"smash error: fg: job-id "<<targetJID<<" does not exist"<<std::endl;
    return;
  }
  int targetPID = jobsPtr->getJobById(targetJID)->getPID();
  std::string targetCmdLine =  jobsPtr->getJobById(targetJID)->getCmd();
  SmallShell& smash = SmallShell::getInstance();
  //all above might better be preprocceed with a helper function in the constructor
  //In that case its fine but if we can run a built-in command in bg it will cause problems.
  if(kill(targetPID,SIGCONT) == -1){
    perror("smash error: kill failed");
    return;
  }
  
  std::cout<<targetCmdLine<<" : "<<targetPID<<std::endl;
  int status;
  smash.setCurrentProcess(targetPID);
  smash.setCurrentCommand(targetCmdLine);
  smash.runFg();
  waitpid(targetPID,&status,WUNTRACED);
  if(WIFEXITED(status)){
    smash.removeJob(targetJID);
  }
  smash.terminateFg();
  smash.setCurrentProcess(-1);
  smash.setCurrentCommand("");
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
    if(!isNum(args[1])){
      std::cerr<<"smash error: bg: invalid arguments"<<std::endl;
      return;
    }
    je = jobsPtr->getJobById(std::stoi(args[1]));
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
  std::cout<< jobsPtr->getJobById(targetJID)->getCmd()<<" : "<<targetPID<<std::endl;
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

  if(!isNum(sigStr.substr(1,sigStr.size()+1))){
    std::cerr<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  sigNum = std::stoi(sigStr.substr(1,sigStr.size()+1));
  JID = std::stoi(args[2]);
  
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
  int PID;
  std::string timedCmd = timed() ? "timeout "+ std::to_string(smash.getDuration())+" "+cmdLine : cmdLine;
  if(child()){
    PID=0;
  }
  else{
    PID = fork();
    if(PID == -1){
      perror("smash error: fork failed");
      return;
    }
    if(PID == 0){
      if(setpgrp() == -1){
        perror("smash error: setpgrp failed");
        return;
      }
    }
  }

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
    if(timed()){
      smash.addTimedJob(PID, smash.getDuration(),time(nullptr),timedCmd);
    }
    if(bg){
      smash.addJob(PID,timedCmd);
    }
    else{
      smash.setCurrentProcess(PID);
      smash.setCurrentCommand(timedCmd);
      waitpid(PID,nullptr,WUNTRACED);
      smash.setCurrentProcess(-1);
      smash.setCurrentCommand("");
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
  char cmdNoBg1[cmd1.size()+1];
  char cmdNoBg2[cmd2.size()+1];
  strcpy(cmdNoBg1,cmd1.c_str());
  strcpy(cmdNoBg2,cmd2.c_str());
  _removeBackgroundSign(cmdNoBg1);
  _removeBackgroundSign(cmdNoBg2);
  cmd1= cmdNoBg1;
  cmd2 = cmdNoBg2;
  int PID = fork();
  if(PID == -1){
    perror("smash error: fork failed");
    return;
  }
  if(PID == 0){
    smash.setChild();
    if(setpgrp() == -1){
      perror("smash error: setpgrp failed");
      return;
    }
    if(close(STDOUT_FILENO)){
      perror("smash error: close failed");
      return;
    }
    if(redirectionCommand == ">"){
      if(open(cmd2.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0655) == -1){
        perror("smash error: open failed");
        return;
      } //TODO Do we need to clean the file?
    }
    else{//redirection == ">>"
      if(open(cmd2.c_str(),O_RDWR | O_CREAT | O_APPEND, 0655) == -1){
        perror("smash error: open failed");
        return;
      }
    }
    smash.executeCommand(cmd1.c_str(),true);
  }else{
    waitpid(PID,nullptr, 0);
  }
}



PipeCommand::PipeCommand(const char * cmd_line,bool isChild) : Command(cmd_line){
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

  int channel = (this->sign == "|") ? STDOUT_FILENO : (this->sign == "|&")? STDERR_FILENO : -1;
  int fd[2];
  if(pipe(fd)== -1){
    perror("smash error: pipe failed");
    return;
  }

  SmallShell& smash=SmallShell::getInstance();
  int PID1 = fork();
  if(PID1 == -1){
    perror("smash error: fork failed");
    return;
  }
  if (PID1 == 0) {
    smash.setChild();
    if(setpgrp() == -1){
      perror("smash error: setpgrp failed");
      return;
    }
    if(dup2(fd[1],channel) == -1){
      perror("smash error: dup2 failed");
      return;
    }
    if(close(fd[0]) == -1){
      perror("smash error: close failed");
      return;
    }
    if(close(fd[1]) == -1){
      perror("smash error: close failed");
       return;
    }
    smash.executeCommand(cmd1.c_str(),true);
  }
  int PID2 = (PID1 == 0) ? -2 : fork();
  if(PID2 == -1){
    perror("smash error: fork failed");
    return;
  } 
  if (PID2 == 0) {
    smash.setChild();
    if(setpgrp() == -1){
      perror("smash error: setpgrp failed");
      return;
    }
    if(dup2(fd[0],0) == -1){
      perror("smash error: dup2 failed");
      return;
    }
    if(close(fd[0]) == -1){
      perror("smash error: close failed");
       return;
    }
    if(close(fd[1]) == -1){
      perror("smash error: close failed");
       return;
    }    
    smash.executeCommand(cmd2.c_str(),true);
  }
  if(PID2 > 0){
    if(close(fd[0]) == -1){
      perror("smash error: close failed");
       return;
    }
    if(close(fd[1]) == -1){
      perror("smash error: close failed");
       return;
    }  
    waitpid(PID2, nullptr, 0);
  }
  }

void SetcoreCommand::execute(){
  char * args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(), args);

  //are arguments valid?
  if(argsNum != 3){
    std::cerr << "smash error: setcore: invalid arguments" << endl;
    return;
  }
  if(!isNum(args[1]) || !isNum(args[2])){
    std::cerr << "smash error: setcore: invalid arguments" << endl;
    return;
  }
  
  int coreNum = std::stoi(args[2]);
  int JID = std::stoi(args[1]);
  if(sysconf(_SC_NPROCESSORS_ONLN)<=coreNum){
    std::cerr << "smash error: setcore: invalid core number" << endl;
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
  CPU_SET(coreNum, &cpuSet); 
  if(sched_setaffinity(PID, sizeof(cpuSet), &cpuSet) == -1){
       perror("smash error: sched_setaffinity failed");
       return;
  }
}

void GetFileTypeCommand::execute(){
  char * args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(), args);
  struct stat file_stat;

  if(argsNum != 2){
    std::cerr << "smash error: getfiletype: invalid arguments" << endl;
    return;
  } 
  if(stat(args[1], &file_stat) == -1){
    perror("smash error: stat failed");
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
  int uga[4];

  for (int i = 0; i < 4; i++)
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

  if(uga[3] == 2)
    mode |= S_ISGID;
  if(uga[3] == 4)
    mode |= S_ISUID;
  if(uga[3] == 1)
    mode |= S_ISVTX;

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

  
  if(argsNum != 3|| !isNum(args[1]) || std::stoi(args[1]) < 0){
    std::cerr << "smash error: chmod: invalid aruments" << std::endl;
    return;
   }
   
  int mod = std::stoi(args[1]);
   
  mode_t modi = int_to_mod(mod);
  int chmodResult = chmod(args[2], modi);
  if(chmodResult == -1){
    perror("smash error: chmod failed");
    return;
  }
}

void TimeoutCommand::execute(){
  char* args[COMMAND_MAX_ARGS];
  int argsNum = _parseCommandLine(this->getCmdLine().c_str(),args);
  //TODO HANDLE ERRORS
  if(argsNum < 2){
    std::cerr<<"smash error: timeout: invalid arguments"<<std::endl;
    return;
  }
  std::string cmdLine = createCmdLine(args,2,argsNum);
  SmallShell& smash = SmallShell::getInstance();
  if(!isNum(args[1])){
    std::cerr<<"smash error: timeout: invalid arguments"<<std::endl;
    return;
  }
  int duration = std::stoi(args[1]);
  time_t finishingTime = duration +time(nullptr);
  time_t oldFinishingTime = (smash.TimedJobsNum()!=0) ? smash.getTimedListHead().getFinisingTime() : finishingTime +1; 
  if(finishingTime < oldFinishingTime){
    if(alarm(duration) == -1){
      perror("smash error: alarm failed");
    }
  }
  smash.setTimedJob(duration);
  smash.executeCommand(cmdLine.c_str(),false,true);
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
  for(auto& jobPtr : jobs){
      if(jobPtr->getPID() == PID){
        jobPtr->startTimer();
        return;
      }
  }
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
      std::cout<<difftime(time(nullptr),job->startingTime)<<" secs";
      if(job->stopped()){
        std::cout<<" (stopped)";
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
  SmallShell& smash = SmallShell::getInstance();
  int pid;
  do{
    pid = waitpid(-1,nullptr,WNOHANG);
    if(pid > 0){
      toRmStack.push(getJIDByPID(pid));
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

JobsList::JobEntry* JobsList::getJobById(int JID) const{
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
  SmallShell& smash = SmallShell::getInstance();
  for (std::list<JobEntry*>::iterator it = jobs.begin(); it != jobs.end(); ++it){
    if((*it)->getJID() == JID){
      smash.setFinished((*it)->getPID());
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



int JobsList::getJIDByPID(int PID) const{
  for(auto& jobPtr : jobs){
    if(jobPtr->getPID() == PID){
      return jobPtr->getJID();
    }
  }
  return -1;
}



void SmallShell::addJob(int PID, const std::string& cmdLine, bool stop){
  jobs.addJob(cmdLine,PID,stop);
}

void SmallShell::addTimedJob(int PID,int duration, const time_t& startingTime,const std::string& cmdLine){
  time_t finshingTime = duration + startingTime;
  for(std::list<TimedJob>::iterator it = TimedJobsList.begin(); it != TimedJobsList.end();++it){
      if(it->getFinisingTime() > finshingTime){
        TimedJobsList.insert(it,TimedJob(PID,duration,startingTime,cmdLine));
        return;
      }
  }
  TimedJobsList.push_back(TimedJob(PID,duration,startingTime,cmdLine));
}



static bool isEmptyLine(const char* cmdLine){
  while(*cmdLine != '\0'){
    if(!isspace(*cmdLine)){
      return false;
    }
    cmdLine++;
  }
  return true;
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line, bool isChild,bool isTimed) {
 
  if(cmd_line == nullptr || isEmptyLine(cmd_line)){
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
  if(argsNum == 0){
    return nullptr;
  }
  std::string redirectionCommand = findRedirectionPip(args,argsNum);
  SmallShell& smash = SmallShell::getInstance();
  if(isChild){
    smash.setChild();
  }
  if(isTimed && isBuiltIn(firstWord)){
      smash.addTimedJob(-1,getDuration(),time(nullptr),firstWord);
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
    return new ShowPidCommand(builtInCmdLine,isChild);
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
  if (firstWord.compare("fg") == 0){
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
  if (firstWord.compare("timeout") == 0){
    return new TimeoutCommand(cmd_line);
  }
  return new ExternalCommand(cmd_line,isChild,isTimed);
}

void SmallShell::executeCommand(const char *cmd_line, bool isChild,bool isTimed) {
  jobs.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line,isChild,isTimed);
  if(cmd != nullptr){
    cmd->execute();
  }
  delete cmd;
}
