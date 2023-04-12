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



/****************Built-in exec implemntaions****************/

void ChPromptCommand::execute(){
  std::string cmdLine = getCmdLine().c_str();
  char* args[COMMAND_MAX_ARGS];
  _parseCommandLine(cmdLine.c_str(),args);
  SmallShell* instance = &SmallShell::getInstance();
  instance->setName(args[1]);
  //no error handling required
}

void ChangeDirCommand::execute(){
  std::string cmdLine = getCmdLine();
  char* args[COMMAND_MAX_ARGS];
  int cmdLen = _parseCommandLine(cmdLine.c_str(),args);
  if(cmdLen > 2){
    printf("smash error: cd: too many arguments\n");
    return;
  }
  SmallShell& smash = SmallShell::getInstance();
  char buffer[COMMAND_ARGS_MAX_LENGTH];
  char* pwd = getcwd(buffer,COMMAND_ARGS_MAX_LENGTH);
  if(strcmp(args[1],"-") == 0){//go to the last pwd.
    if(smash.noDirHistory()){
      printf("smash error: cd: OLDPWD not set");
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
  printf("%s\n",cwd);
}

SmallShell::SmallShell() : shellName("smash"),dirHistory(), calledCd(false){
  char buffer[COMMAND_ARGS_MAX_LENGTH];
  char* cwd = getcwd(buffer,COMMAND_ARGS_MAX_LENGTH);
  dirHistory.push(std::string(cwd));
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  char builtInCmdLine[strlen(cmd_line)+1];
  builtInCmdLine[strlen(cmd_line)] = NULL;
  strcpy(builtInCmdLine,cmd_line);
  _removeBackgroundSign(builtInCmdLine);
  if (firstWord.compare("showpid") == 0) 
  {
    return new ShowPidCommand(builtInCmdLine);
  }
  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(builtInCmdLine);
  }
  if (firstWord.compare("chprompt") == 0) {
    return new ChPromptCommand(builtInCmdLine);
  }
  if (firstWord.compare("cd") == 0) {
    char buffer[COMMAND_ARGS_MAX_LENGTH];
    return new ChangeDirCommand(builtInCmdLine,new char*(getcwd(buffer,COMMAND_ARGS_MAX_LENGTH)));
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  Command* cmd = CreateCommand(cmd_line);
  if(cmd != nullptr){
    cmd->execute();
  }
  delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}