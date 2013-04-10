#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Process {
	int ID;
	int arrivalTime;
	int totalCPU;
	int avgBurst;

	Process(int id, int aT, int tC, int aB)
		: ID(id), arrivalTime(aT), totalCPU(tC), avgBurst(aB) {}
};

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

void readProcesses(string ProcessFile, vector<Process*>& processes){
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
			processes.push_back(new Process(id, aT, tC, aB));
		}
		processFile.close();
	}
}

int main(int argc, char* argv[])
{
	string ProcessFile;
	int IOdelay, ContextSwitchDelay, CTSSQueues;
	bool Debug;
	vector<Process*> processes = vector<Process*>();

	readOpts(ProcessFile, IOdelay, ContextSwitchDelay, CTSSQueues, Debug);
	readProcesses(ProcessFile, processes);



	return 0;
}