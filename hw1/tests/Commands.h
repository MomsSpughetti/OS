#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_



#include <vector>
#include <string>
#include <stack>
#include <list>
#include <time.h>
#include <unistd.h>
#include <exception>
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
  std::string cmdLine;
  bool isChild;
 public:
  Command(const char* cmd_line, bool isChild = false) : cmdLine(std::string(cmd_line)), isChild(isChild){}
  virtual ~Command() = default;
  virtual void execute() = 0;
  std::string getCmdLine() const{return cmdLine;} 
  bool child(){return isChild;}
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line, bool isChild = false): Command(cmd_line,isChild){}
  virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command { 
 public:
  ExternalCommand(const char* cmd_line, bool isChild = false): Command(cmd_line,isChild){}
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
  explicit PipeCommand(const char* cmd_line, bool isChild = false);
  virtual ~PipeCommand()  = default;
  void execute() override;
  
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line, bool isChild = false): Command(cmd_line,isChild){}
  virtual ~RedirectionCommand()  = default;
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};



class ChPromptCommand : public BuiltInCommand{
  public:
  ChPromptCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
  ChangeDirCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~ChangeDirCommand() = default;
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~GetCurrDirCommand() = default;
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~ShowPidCommand() = default;
  void execute() override;//TODO smash or current smash Name?
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
    JobEntry(int JID,int PID, const std::string& cmdLine, bool isStopped = false)
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
  int getJIDByPID(int PID) const;
  void stopJob(int PID){for(auto& jPtr : jobs){if(jPtr->getPID()== PID) jPtr->stop();}}
  // TODO: Add extra methods or modify exisitng ones as needed

};


class ForegroundCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs, bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~ForegroundCommand() = default;
  void execute() override;
};


class BackgroundCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs, bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~BackgroundCommand() = default;
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  JobsList* jobsPtr;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs, bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~QuitCommand() = default;
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~ChmodCommand() = default;
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  GetFileTypeCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~GetFileTypeCommand() = default;
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~SetcoreCommand() = default;
  void execute() override;
};


class KillCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs, bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~KillCommand() = default;
  void execute() override;
};

class JobsCommand : public BuiltInCommand{
  JobsList* jobsPtr;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs, bool isChild = false): BuiltInCommand(cmd_line,isChild), jobsPtr(jobs){}
  virtual ~JobsCommand() = default;
  void execute() override;
};



class SmallShell {
 private:
  int PID;
  std::string shellName;
  std::string lastDir;
  std::string currentCmdLine;
  JobsList jobs;
  bool finished;
  bool isChild;
  int currentProcess;

  class GetpidFailed : public std::exception{};
  SmallShell() : PID(getpid()),shellName("smash"),lastDir(""),currentCmdLine(""),jobs(),finished(false),currentProcess(-1){}
 public:
  std::string getName() const{return shellName;}
  void setName(const std::string& newName){shellName = newName;}
  Command *CreateCommand(const char* cmd_line, bool isChild =false);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  void setChild(){isChild = true;}
  bool child()const{return isChild;}
  int getCurrentProcess() const {return currentProcess;}
  void setCurrentProcess(int newProcessPID){currentProcess=newProcessPID;}

  std::string getCurrentCommand() const{return currentCmdLine;}
  void setCurrentCommand(const std::string cmdLine){currentCmdLine =cmdLine;}
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
  std::string getLastDir() const {return this->lastDir;}
  void updateLastDir(const std::string& lastDir){this->lastDir= lastDir;}
  bool noDirHistory() const{return lastDir == "";}
  /**************Jobs****************/
  void addJob(int PID,const std::string& cmdLine, bool stop = false);
  void removeJob(int JID){jobs.removeJobById(JID);};
  void quit(){finished =true;}
  bool isFinished(){return finished;}
  int getJobPID(int JID) {return (jobs.getJobById(JID) == nullptr)? -1 : jobs.getJobById(JID)->getPID();}
  int getJobJID(int PID) const{return jobs.getJIDByPID(PID);};
  int getPID() const {return PID;}
  void stopJob(int PID){jobs.stopJob(PID);}
};

#endif //SMASH_COMMAND_H_

