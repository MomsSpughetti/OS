#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

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

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if(firstWord.compare("chprompt") == 0) {
    return new chpromptCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0){
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0){
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0){
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0){
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0){
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0){
    return new GetCurrDirCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

//---------------------------Command class----------------------------//

Command::Command(const char* cmd_line){
  try
  {
    args = new char*[COMMAND_MAX_ARGS];
  }
  catch(const std::exception& e)
  {
    //std::cerr << e.what() << '\n';
    throw;
  }
  
  this->args_length = _parseCommandLine(cmd_line, this->args);
}

//-----------------------BuitInCommands----------------------//

//---------------------------1--------------------------------//

chpromptCommand::chpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line){
}

void chpromptCommand::execute(){
  //change smash
  SmallShell& smash = SmallShell::getInstance();
  if(this->getArgs_length() == 1){
    smash.setShellName("smash");
  } else{
    char ** _args = this->getArgs();
    smash.setShellName(_args[1]);
  }
}

//---------------------------3------------------------------//
GetCurrDirCommand::GetCurrDirCommand(const char * cmd_line) : BuiltInCommand(cmd_line){
}

void GetCurrDirCommand::execute(){
  char * dir = getcwd(nullptr, 0);
  std::cout << dir;
}

//--------------------------BuiltInCommand class--------//
  BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line){

  }

//------------------------jobsList class implementation---------------------------//

//------------JobEntry-----------------------------
JobsList::JobEntry::JobEntry(int PID, Command *cmd, bool isStopped){

}

//-------------------------------------------------

JobsList::JobsList() = default;
JobsList::~JobsList() = default;

void JobsList::addJob(Command *cmd, bool isStopped = false){
  int PID = 0;
  this->getLastJob(&PID);

  JobEntry *je = new JobEntry(PID + 1, cmd, isStopped);
}

void JobsList::printJobsList(){

}
void JobsList::killAllJobs(){
  SmallShell & smash = SmallShell::getInstance();

  for(auto &i : jobs){
    //kill i (send sigkill to the process)
  }
}

bool JobsList::JobEntry::getisStopped() const{
  return isStopped;
}

void JobsList::JobEntry::setIsStopped(bool vl){
  this->isStopped = vl;
}

bool isFinished(const JobsList::JobEntry& obj){
  return obj.getisStopped();
}

void JobsList::removeFinishedJobs(){
    jobs.remove_if(isFinished);
}

JobsList::JobEntry * JobsList::getJobById(int jobId){
  for(auto & i : jobs){
    if (i.getPID() == jobId){
      return &i;
    }
  }
}

void JobsList::removeJobById(int jobId){
  for (std::list<JobEntry>::iterator it = jobs.begin(); it != jobs.end(); ++it){
    if(it->getPID() == jobId){
      jobs.erase(it);
      return;
    }
}
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJobId){
  JobEntry je = jobs.back();
  *lastJobId = je.getPID();
  return &je;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId){
  for (std::list<JobEntry>::reverse_iterator it = jobs.rbegin(); it != jobs.rend(); ++it){
    if(it->getisStopped() == true){
      *jobId = it->getPID();
      return &(*it);
    }

}
