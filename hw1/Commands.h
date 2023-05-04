#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_



#include <vector>
#include <string>
#include <stack>
#include <list>
#include <time.h>
#include <unistd.h>
#include <iostream>
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
  bool child() const{return isChild;}
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line , bool isChild = false): Command(cmd_line,isChild){}
  virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command { 
  bool isTimed;
 public:
  ExternalCommand(const char* cmd_line,bool isChild = false, bool isTimed = false) : Command(cmd_line,isChild),isTimed(isTimed){}
  bool timed() const{return isTimed;}
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
  explicit RedirectionCommand(const char* cmd_line,bool isChild = false): Command(cmd_line,isChild) {} 
  virtual ~RedirectionCommand()  = default;
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};



class ChPromptCommand : public BuiltInCommand{
  public:
  ChPromptCommand(const char* cmd_line,bool isChild = false): BuiltInCommand(cmd_line,isChild) {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
  ChangeDirCommand(const char* cmd_line, bool isChild = false): BuiltInCommand(cmd_line,isChild) {}
  virtual ~ChangeDirCommand() = default;
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line,bool isChild = false): BuiltInCommand(cmd_line,isChild) {}
  virtual ~GetCurrDirCommand() = default;
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line,bool isChild = false): BuiltInCommand(cmd_line,isChild) {}
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
  JobEntry* getJobById(int jobId) const;
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  int jobsNum() const{return jobs.size();}
  int findMaxJID() const;
  int getMaxJID() const{return maxJID;};
  void quit();
  int getJIDByPID(int PID) const;
  std::string getCmd(int JID) const{return getJobById(JID)->getCmd();}
  // TODO: Add extra methods or modify exisitng ones as needed

};

class ForegroundCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs,bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~ForegroundCommand() = default;
  void execute() override;
};


class BackgroundCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs,bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~BackgroundCommand() = default;
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  JobsList* jobsPtr;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs,bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~QuitCommand() = default;
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {

 public:
  explicit TimeoutCommand(const char* cmd_line,bool isChild = false) : BuiltInCommand(cmd_line,isChild) {}
  virtual ~TimeoutCommand() =default;
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  explicit ChmodCommand(const char* cmd_line,bool isChild = false): BuiltInCommand(cmd_line,isChild){}
  virtual ~ChmodCommand() = default;
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  explicit GetFileTypeCommand(const char* cmd_line,bool isChild = false): BuiltInCommand(cmd_line,isChild) {}
  virtual ~GetFileTypeCommand() = default;
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  explicit SetcoreCommand(const char* cmd_line,bool isChild = false): BuiltInCommand(cmd_line,isChild) {}
  virtual ~SetcoreCommand() = default;
  void execute() override;
};


class KillCommand : public BuiltInCommand {
  JobsList* jobsPtr;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs,bool isChild = false): BuiltInCommand(cmd_line,isChild),jobsPtr(jobs){}
  virtual ~KillCommand() = default;
  void execute() override;
};

class JobsCommand : public BuiltInCommand{
  JobsList* jobsPtr;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs,bool isChild = false): BuiltInCommand(cmd_line,isChild) , jobsPtr(jobs){}
  virtual ~JobsCommand() = default;
  void execute() override;
};


class TimedJob{
private:
  int PID;
  int duration;
  time_t startingTime;
public:
  TimedJob(int PID, int duration,const time_t& startingTime) : PID(PID), duration(duration),startingTime(startingTime) {}
  time_t getFinisingTime() const{
    return startingTime + time_t(duration);
  }
  int getPID() const{return PID;}
  int getDuration() const{return duration;}
};

class SmallShell {
 private:
  int pid;
  std::string shellName;
  std::string lastDir;
  std::string currentCmdLine;
  JobsList jobs;
   //PID : Time
  bool finished;
  bool isChild;
  int currentProcess;
  int currentTimedJobDuration;
  SmallShell() : pid(getpid()),shellName("smash"),lastDir(""),currentCmdLine(""),jobs(),finished(false),currentProcess(-1),currentTimedJobDuration(-1){}
 public:
  std::string getName() const{return shellName;}
  void setName(const std::string& newName){shellName = newName;}
  Command *CreateCommand(const char* cmd_line, bool isChild =false, bool isTimed = false);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  void setChild(){isChild = true;}
  bool child()const{return isChild;}
  int getCurrentProcess() const {return currentProcess;}
  void setCurrentProcess(int newProcessPID){currentProcess=newProcessPID;}
std::list<TimedJob> TimedJobsList;
  std::string getCurrentCommand() const{return currentCmdLine;}
  void setCurrentCommand(const std::string cmdLine){currentCmdLine =cmdLine;}
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell() = default;
  void executeCommand(const char* cmd_line, bool isChild =false, bool isTimed = false);
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
  int getPid() const{return pid;}
  void addTimedJob(int PID,int duration, const time_t& startingTime);
  void setTimedJob(int duration){currentTimedJobDuration = duration;}
  int getDuration() const{return currentTimedJobDuration;}
  TimedJob getTimedListHead() const{return TimedJobsList.front();}
  std::string getJobCmdLine(int JID){return jobs.getCmd(JID);}
  void popTimedJobsList(){TimedJobsList.pop_front();}
  void removeFinishedTimedJobs();
};

#endif //SMASH_COMMAND_H_
