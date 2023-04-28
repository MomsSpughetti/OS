#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_



#include <vector>
#include <string>
#include <stack>
#include <list>
#include <time.h>
#include <unistd.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
  std::string cmdLine;

 public:
  Command(const char* cmd_line) : cmdLine(std::string(cmd_line)){}
  virtual ~Command() = default;
  virtual void execute() = 0;
  std::string getCmdLine() const{return cmdLine;} 
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line): Command(cmd_line){}
  virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command { 
  bool isChild;
 public:
  ExternalCommand(const char* cmd_line,bool isChild = false) : Command(cmd_line),isChild(isChild){}
  bool child() const{return isChild;}
  virtual ~ExternalCommand() = default;
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 private:
  std::string sign;
  int signPlace;
  std::string cmd1;
  std::string cmd2;

 public:
  explicit PipeCommand(const char* cmd_line);
  virtual ~PipeCommand()  = default;
  void execute() override;
  
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line) : Command(cmd_line) {} 
  virtual ~RedirectionCommand()  = default;
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};



class ChPromptCommand : public BuiltInCommand{
  public:
  ChPromptCommand(const char* cmd_line)
   : BuiltInCommand(cmd_line){}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
  ChangeDirCommand(const char* cmd_line)
   : BuiltInCommand(cmd_line){}
  virtual ~ChangeDirCommand() = default;
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~GetCurrDirCommand() = default;
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ShowPidCommand() = default;
  void execute() override{printf("smash pid is %d\n",getpid());} //TODO smash or current smash Name?
};


class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
    int JID;
    int PID;
    std::string cmdLine;
    bool isStopped;
    friend class JobsList;
    time_t startingTime;
   public:
    JobEntry(int JID,int PID, const std::string& cmdLine, bool isStopped)
     : JID(JID),PID(PID),cmdLine(cmdLine),isStopped(isStopped){}
    ~JobEntry() =default;
    bool stopped() const{return isStopped;};
    void run(){isStopped = false;}
    void stop(){isStopped = true;}
    int getJID() const{return JID;}
    int getPID() const{return PID;}
    void startTimer(){ startingTime = time(nullptr);}
    std::string getCmd() const {return cmdLine;}
  };
 // TODO: Add your data members
 private:
  std::list<JobEntry*> jobs;
  int maxJID;
 public:
  JobsList() : jobs(),maxJID(-1) {}
  ~JobsList(){quit();}
  void addJob(const std::string& cmdLine,int PID, bool isStopped = false);
  void printJobsList();
  void printForQuit();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry* getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  int jobsNum() const{return jobs.size();}
  int findMaxJID() const;
  int getMaxJID() const{return maxJID;};
  void quit();
  // TODO: Add extra methods or modify exisitng ones as needed

};


class ForegroundCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobsPtr(jobs){}
  virtual ~ForegroundCommand() = default;
  void execute() override;
};


class BackgroundCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobsPtr(jobs){}
  virtual ~BackgroundCommand() = default;
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  JobsList* jobsPtr;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobsPtr(jobs){}
  virtual ~QuitCommand() = default;
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  GetFileTypeCommand(const char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};


class KillCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobsPtr(jobs){}
  virtual ~KillCommand() = default;
  void execute() override;
};

class JobsCommand : public BuiltInCommand{
  JobsList* jobsPtr;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobsPtr(jobs){}
  virtual ~JobsCommand() = default;
  void execute() override;
};

class SmallShell {
 private:
  std::string shellName;
  std::stack<std::string> dirHistory;
  JobsList jobs;
  bool finished;
  bool isChild;
  SmallShell() : shellName("smash"),dirHistory(),jobs(),finished(false){}
 public:
  std::string getName() const{return shellName;}
  void setName(const std::string& newName){shellName = newName;}
  Command *CreateCommand(const char* cmd_line, bool isChild =false);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  void setChild(){isChild = true;}
  bool child()const{return isChild;}
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell() = default;
  void executeCommand(const char* cmd_line, bool isChild =false);
  // TODO: add extra methods as needed

  /**********CD-Functions***********/
  std::string getLastDir() const {return dirHistory.top();}
  void recordDir(const std::string& lastDir){dirHistory.push(lastDir);}
  void rmLastDir(){dirHistory.pop();}
  bool noDirHistory() const{return dirHistory.empty();}
  /**************Jobs****************/
  void addJop(int PID,const std::string& cmdLine);
  void quit(){finished =true;}
  bool isFinished(){return finished;}
  int getJobPID(int JID){return (jobs.getJobById(JID) == nullptr)? -1 : jobs.getJobById(JID)->getPID();}
};

#endif //SMASH_COMMAND_H_