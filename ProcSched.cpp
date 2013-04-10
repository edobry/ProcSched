#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits.h>
#include <unordered_map>
#include <queue>

using namespace std;

//Utilities
class Random{
public:
	int operator()() {
		if(rands[0] < 0){
			ifstream randoms("random-numbers.txt");	
			if (randoms.is_open())
			{
				string line;
				getline(randoms, line);
				for(int i = 0; i < 10000; i++)
					rands[i] = stoi(line);
				randoms.close();
			}
		}
		if(next > 10000) next = 0;
		return rands[next++];
	}
	Random() : next(0) {}
private:
	int rands[100000];
	int next;
};
Random RandNum = Random();
double getProbability(){ return RandNum()/INT_MAX; }

struct Process {
public:
	int ID;
	int arrivalTime;
	int totalCPU;
	int avgBurst;

	//Commands
	int Start();
	int Pause();
	int Resume();
	int Stop();

	//Queries
	bool BurstFinished(){
		if(CPUelapsed == totalCPU) return true;
		else if(CPUelapsed < (avgBurst - 1))
			return false;
		else return evenDistribution(CPUelapsed, totalCPU);
	}

	Process(int id, int aT, int tC, int aB)
		: ID(id), arrivalTime(aT), totalCPU(tC), avgBurst(aB), CPUelapsed(0) {}

private: 	
	int CPUelapsed;
	int quantRemaining;

	int evenDistribution(int elap, int total){
		return getProbability() <= (CPUelapsed == (avgBurst - 1)) 
			? 1/3
			: 1/2;
	}

	int tick();
};

class ProcessQueue{
private:
	unordered_map<int, vector<Process*>> inner;
public:
	ProcessQueue(){
		inner = unordered_map<int, vector<Process*>>();
	}

	void Enqueue(Process* process){
		int time = process->arrivalTime;
		if(!inner.count(time))
			inner[time] = vector<Process*>();

		inner[time].push_back(process);
	}

	vector<Process*> AtTime(int time){
		return inner.count(time) ? inner[time] : vector<Process*>();
	}

	int count(){
		return inner.size();
	}
};


//Initialization
void readOpts(string& ProcessFile, int& IOdelay, int& ContextSwitchDelay, int& CTSSQueues, bool& Debug){
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
				ProcessFile = val;
				break;
			case 1:
				IOdelay = stoi(val);
				break;
			case 2:
				ContextSwitchDelay = stoi(val);
				break;
			case 3:
				CTSSQueues = stoi(val);
				break;
			case 4:
				Debug = val == "true";
				break;
			default: break;
			}
			cout << optName << ": " << val << endl;

		}
		scheduling.close();
	}
}

void readProcesses(string ProcessFile, ProcessQueue& processes){
	ifstream processFile(ProcessFile);

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
	}
}

//Scheduling Algorithms
class Scheduler{
public:
	virtual int tick();

	Scheduler(ProcessQueue& processes) : processes(processes), queueWait(queue<Process*>()) {}
protected:
	int time;
	ProcessQueue processes;
	queue<Process*> queueWait;
};

class FCFSScheduler : public Scheduler{
public:
	int tick(){
		vector<Process*> incoming = processes.AtTime(time++);
		if(incoming.size() > 0)
			for(int i = 0; i < incoming.size(); i++){
				queueWait.push(incoming.back());
				incoming.pop_back();
			}
			return 0;
	}
};

class CTSScheduler : public Scheduler{
};

int main(int argc, char* argv[])
{
	//Initialization
	string ProcessFile;
	int IOdelay, ContextSwitchDelay, CTSSQueues;
	bool Debug;
	ProcessQueue processes = ProcessQueue();

	readOpts(ProcessFile, IOdelay, ContextSwitchDelay, CTSSQueues, Debug);
	readProcesses(ProcessFile, processes);

	//Scheduling
	//scheduleFCFS(processes);

	return 0;
}