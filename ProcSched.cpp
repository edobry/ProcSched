#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <unordered_map>
#include <queue>

using namespace std;

//Utilities
struct Options{
	string ProcessFile;
	int IOdelay, ContextSwitchDelay, CTSSQueues;
	bool Debug;

	Options() {}
	Options(string pF, int io, int cSD, int ctssQ, bool d) : ProcessFile(pF), IOdelay(io),
		ContextSwitchDelay(cSD), CTSSQueues(ctssQ), Debug(d) {}
};
Options SchedulerOptions;

enum ProcessorState {
	Idle, Running, ContextSwitch
};

class Random{
public:
	Random() : next(0) {}
	double operator()() {
		if(rands[0] == 0){
			ifstream randoms("random-numbers.txt");	
			if (randoms.is_open())
			{
				for(int i = 0; i < 10000; i++){
					string line;
					getline(randoms, line);
					rands[i] = stoi(line);
				}
				randoms.close();
			}
		}
		if(next > 10000) next = 0;
		return rands[next++];
	}
private:
	int rands[10000];
	int next;
};
Random RandNum = Random();
double getProbability(){ return RandNum() / std::numeric_limits<int>::max();}

struct Process {
public:
	int ID;
	int arrivalTime;
	int totalCPU;
	int avgBurst;
	int CTSSIndex;

	//Queries
	bool BurstFinished(){
		return (CPUelapsed < (avgBurst - 1)) 
			? false
			: (getProbability() <= (CPUelapsed == (avgBurst - 1)
				? 1.0/3.0
				: 1.0/2.0));
	}

	bool Finished() {
		return CPUelapsed >= totalCPU;
	}

	void Reset(){
		CPUelapsed = 0;
	}

	void Tick(){
		CPUelapsed++;
	}

	void TickCTSS(){
		CPUelapsed++;
		quantRemaining--;
	}

	bool QuantEnded() {
		return quantRemaining == 0;
	}

	bool HalfQuantRemaining() {
		return 2*quantRemaining >= quantTotal;
	}

	void QuantUp() {
		quantTotal *= 2;
		quantRemaining = quantTotal;
	}

	void QuantDown() {
		quantTotal /= 2;
		quantRemaining = quantTotal;
	}

	void QuantReset() {
		quantRemaining = quantTotal;
	}

	Process(int id, int aT, int tC, int aB)
		: ID(id), arrivalTime(aT), totalCPU(tC), avgBurst(aB), CPUelapsed(0), quantTotal(1), quantRemaining(1), CTSSIndex(0) {}

private: 	
	int CPUelapsed;

	// CTSS-related variables
	int quantRemaining;
	int quantTotal;
};
struct WaitingProcess{
	Process* process;
	int timeReady;

	WaitingProcess(Process* p, int tR) : process(p), timeReady(tR) {}
};

class ProcessQueue{
private:
	unordered_map<int, vector<Process*>> inner;
	int count;
public:
	ProcessQueue() : count(0), inner(unordered_map<int, vector<Process*>>()) {}

	void Enqueue(Process* process){
		int time = process->arrivalTime;
		if(!inner.count(time))
			inner[time] = vector<Process*>();

		inner[time].push_back(process);
		count++;
	}

	vector<Process*> AtTime(int time){
		return inner.count(time) > 0 ? inner.at(time) : vector<Process*>();
	}

	void RemoveTime(int time){
		count -= inner[time].size(); 
		inner.erase(time);
	}

	int Count(){
		return count;
	}
};

//Initialization
void readOpts(){
	SchedulerOptions = Options();
	ifstream scheduling("scheduling.txt");

	if (scheduling.is_open())
	{
		string line;
		for(int i = 0; scheduling.good(); i++)
		{
			getline(scheduling, line);
			int splitPos = line.find('=');
			string optName = line.substr(0, splitPos);
			string val = line.substr(splitPos+1);
			switch(i){
			case 0:
				SchedulerOptions.ProcessFile = val;
				break;
			case 1:
				SchedulerOptions.IOdelay = stoi(val);
				break;
			case 2:
				SchedulerOptions.ContextSwitchDelay = stoi(val);
				break;
			case 3:
				SchedulerOptions.CTSSQueues = stoi(val);
				break;
			case 4:
				SchedulerOptions.Debug = val == "true";
				break;
			default: break;
			}
			cout << optName << ": " << val << endl;
		}
		cout << "==============================================================" << endl << endl;
		scheduling.close();
	}
}

void readProcesses(ProcessQueue& processes){
	ifstream processFile(SchedulerOptions.ProcessFile);

	if (processFile.is_open())
	{
		string line;
		while(processFile.good())
		{
			getline(processFile, line);
			stringstream ss(line);
			int id, aT, tC, aB;
			ss >> id >> aT >> tC >> aB;
			processes.Enqueue(new Process(id, aT, tC, aB));
		}
		processFile.close();

		cout << "Processes queued: " << processes.Count() << endl;
	}
}

//Scheduling Algorithms
class Scheduler{
public:
	virtual void tick(){ }
	virtual bool IsDone(){ return false; }
	virtual void scheduleProcess(Process* p){}

	void Schedule(){
		do{
			tick();
		} while(!IsDone());
	}

	Scheduler(ProcessQueue& processes) : time(0), processes(processes), queueWait(queue<WaitingProcess*>()), state(Idle) {}
protected:
	int time;
	Process* current;
	ProcessQueue processes;
	queue<WaitingProcess*> queueWait;
	ProcessorState state;

	virtual void runNextReadyProcess() {}

	void getIncoming(){
		vector<Process*> incoming = processes.AtTime(time);
		int numIncoming = incoming.size();
		processes.RemoveTime(time++);
		cout << "Processes incoming: " << numIncoming << endl;

		if(numIncoming > 0)
			for(int i = 0; i < numIncoming; i++){
				Process* current = incoming.back();
				cout << "Queueing process #" << current->ID << endl;
				scheduleProcess(current);
				incoming.pop_back();
			}
	}
	void checkWaiting(){
		while(!queueWait.empty() && queueWait.front()->timeReady == time){
			scheduleProcess(queueWait.front()->process);
			queueWait.pop();
		}
	}
};

class FCFSScheduler : public Scheduler{
public:
	void tick(){
		cout << "====================================================" << endl
			<< "Tick " << time << endl << "-----------------------" << endl;

		getIncoming();
		checkWaiting();

		//Run process
		switch(state){
		case Idle:
			cout << "Processes ready: " << queueReady.size() << endl;
			runNextReadyProcess();
			break;
		case ContextSwitch:
			cout << "Context switch delay... " << delayLeft-- << " ticks" << endl;
			if(delayLeft == 0) state = Idle;
			break;
		case Running:
			current->Tick();
			cout << "Running proccess #" << current->ID << endl;

			if(current->Finished()){
				cout << "Proccess #" << current->ID << " is finished" << endl;
				current->Reset();
				current = NULL;
				state = Idle;
				return;
			}
			else if(current->BurstFinished()){
				cout << "Burst finished, process waiting" << endl;
				queueWait.push(new WaitingProcess(current, time+SchedulerOptions.IOdelay));
				current = NULL;

				state = ContextSwitch;
				delayLeft = SchedulerOptions.ContextSwitchDelay;
			}			
			break;
		}
		return;
	}

	bool IsDone(){
		cout << "Processes yet to arive: " << processes.Count() << endl
			<< "Processes waiting: " << queueWait.size() << endl;
		return processes.Count() == 0 && queueWait.size() == 0 && queueReady.size() == 0 && current == NULL;
	}

	void scheduleProcess(Process* p){ queueReady.push(p); }

	FCFSScheduler(ProcessQueue& processes) : Scheduler(processes), delayLeft(0) {}
protected:
	void runNextReadyProcess(){
		current = queueReady.front();
		queueReady.pop();

		cout << "Running proccess #" << current->ID << endl;
		state = Running;
		current->Tick();
	}
private:
	int delayLeft;
	queue<Process*> queueReady;
};

class CTSScheduler : public Scheduler{
private:
	vector<queue<Process*>> queues;
	int delayLeft;
protected:
	void runNextReadyProcess() {
		for (size_t i = 0; i < queues.size(); i++) {
			if (queues[i].empty())
				continue;
			current = queues[i].front();
			queues[i].pop();
			state = Running;
			current->TickCTSS();
		}
	}
public:
	void scheduleProcess(Process *p) {
		queues[p->CTSSIndex].push(p);
	}

	void tick(){
		cout << "====================================================" << endl
			<< "Tick " << time << endl << "-----------------------" << endl;

		getIncoming();
		checkWaiting();

		//Run process
		switch(state){
		case Idle:
			//cout << "Processes ready: " << queueReady.size() << endl;
			runNextReadyProcess();
			break;
		case ContextSwitch:
			cout << "Context switch delay... " << delayLeft-- << " ticks" << endl;
			if(delayLeft == 0) state = Idle;
			break;	
		case Running:
			for (size_t i = 0; i < current->CTSSIndex; i++) {
				if (!queues[i].empty()) {
					queues[current->CTSSIndex].push(current);
					state = ContextSwitch;
					delayLeft = SchedulerOptions.ContextSwitchDelay;
					break;
				}
			}
			cout << "Running proccess #" << current->ID << endl;
			if (current->QuantEnded() && !current->BurstFinished()) {
				if (current->CTSSIndex < queues.size() - 1) {
					current->CTSSIndex++;
					queues[current->CTSSIndex].push(current);
					current->QuantUp();
				} else {
					queues[current->CTSSIndex].push(current);
					current->QuantReset();
				}
			} else if (current->BurstFinished() && current->HalfQuantRemaining()) {
				if (current->CTSSIndex > 0) {
					current->CTSSIndex--;
					current->QuantDown();
				}
			} 
			if(current->BurstFinished()){
				cout << "Burst finished, process waiting" << endl;
				current->QuantReset();
				queueWait.push(new WaitingProcess(current, time+SchedulerOptions.IOdelay));

				state = ContextSwitch;
				delayLeft = SchedulerOptions.ContextSwitchDelay;
			}
			else current->TickCTSS();
			break;
		}
		return;
	}

	bool IsDone(){
		cout << "Processes yet to arive: " << processes.Count() << endl
			<< "Processes waiting: " << queueWait.size() << endl;
		bool empty = true;
		for(int i = 0; i < SchedulerOptions.CTSSQueues && empty; i++){
			empty = queues.at(i).empty();
		}
		return processes.Count() == 0 && queueWait.size() == 0 && empty;
	}

	CTSScheduler(ProcessQueue& processes) : Scheduler(processes), queues(vector<queue<Process*> >(SchedulerOptions.CTSSQueues)) {}
};

int main(int argc, char* argv[])
{
	//Initialization
	ProcessQueue processes = ProcessQueue();

	readOpts();
	readProcesses(processes);

	//Scheduling
	cout << "Starting Scheduling: FCFS" << endl;
	FCFSScheduler scheduler(processes);
	scheduler.Schedule();
	
	cout << endl << endl << endl;

	cout << "Starting Scheduling: CTSS" << endl;
	CTSScheduler scheduler2(processes);
	scheduler2.Schedule();

	return 0;
}