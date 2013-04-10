#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
	string ProcessFile;
	int IOdelay, ContextSwitchDelay, CTSSQueues;
	bool Debug;

	ifstream myfile("scheduling.txt");

	if (myfile.is_open())
	{
		string line;
		for(int i = 0; myfile.good(); i++)
		{
			getline(myfile, line);
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
		myfile.close();
		cin >> ProcessFile;
	}

	return 0;
}