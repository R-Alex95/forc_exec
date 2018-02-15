/*
 * shapeMaster.cpp
 *
 *  Created on: Nov 10, 2016
 *      Author: alex
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib> // atof
#include <cstdio>
#include <cerrno>
#include <cmath> // ceil()

#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h> // [mkfifo();]
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h> // PIPE_BUF http://man7.org/linux/man-pages/man7/pipe.7.html

#include "shapeHelperFunctions.h"

using namespace std;

int main(int argc,char **argv) {
	cout << "Program |shapeMaster| is running " << endl;
	srand(time(NULL));
	char *inputBinaryFile,*tmpdir;
	int workersCount = 0;
	if (argc != 7) {
		cerr << "ERROR : Wrong arguments for main.cpp(utilityX)!" << endl;
		cerr << "EXAMPLE : ./shapes -i InputBinaryFile -w WorkersCount -d TempDir" << endl;
		exit(EXIT_FAILURE);
	}
	else {
		int i = 1;
		while (i < argc) {
			if (strncmp(argv[i],"-i",2) == 0) {
				inputBinaryFile = argv[i + 1];
			}
			else if (strncmp(argv[i],"-w",2) == 0) {
				workersCount = atoi(argv[i + 1]);
				if (workersCount == 0) {
					cerr << "ERROR : Program can't run with 0 workers.Exiting,try again" << endl;
					exit(EXIT_FAILURE);
				}
			}
			else if (strncmp(argv[i],"-d",2) == 0) {
				tmpdir = Strduplicate(argv[i + 1]);
				// append '/' as last char if missed by user
				int length = strlen(tmpdir);
				if (tmpdir[length - 1] != '/') {
					tmpdir[length] = '/';
					tmpdir[length + 1] = '\0';
				}
			}
			cout << argv[i] << endl;
			i = i + 2;
		}
	}
	cout << "Arguments : inputBinaryFile : " << inputBinaryFile << endl
			<< "Amount of workers : " << workersCount << endl
			<< "Temporal Directory : " << tmpdir << endl;

	Check_directory(tmpdir);

    ifstream ifile (inputBinaryFile, ios::in | ios::binary);
    int totalPoints = 0;
    if ( ifile.is_open() ) {
        ifile.seekg(0, ios::end);
    	int length = ifile.tellg();
		totalPoints = length/8; // 8 = 2 * 4 (= bytes of float)
    }
    else
    	perror ("Failed to open InputFile\n");
    ifile.close();
    cout << "Total points read :" << totalPoints << endl;
    // Round up so last worker won't miss any points at the end of file
    int pointsPerWorker = ceil(totalPoints/(float)workersCount);
    cout << "Points per Worker :" << pointsPerWorker << endl;

	string line;
	bool exitFlag = false;
	do {
		int commandCount = 1;
		getline(cin,line);
		if (line == "exit")
			break;
		string token;
		for (uint i = 0 ; i < line.length() ; i++) {
			if (line[i] == ',')
				commandCount++;
			if (line[i] == ';') // erase everything after ';'
				line.resize(i);
		}
		cout << "commandCount : " << commandCount << endl;

		string nameOfScript = Create_GnuScript(tmpdir,commandCount);

		stringstream ss(line);
		// splitting line by comma(s)(',')
		int numOfHandler = commandCount;
		while (getline(ss,token,',')) {
			if (token == "exit") {
				exitFlag = true;
				break;
			}
			/* Create handlers based on how many commas(=commands) exist */
			pid_t pidHandler = fork();
			numOfHandler--;
			if (!pidHandler) { // if child = handler
				/* String manipulation */
				pidHandler = getpid();
				size_t found = token.find_last_of(" ");
				string colour = token.substr(found + 1);
				token.resize(found); // erasing colour from string
				found = token.find_first_of(" ");
				string utilityName = token.substr(0,found);
				string utilityPath = Get_cwd(utilityName);
				token = token.substr(found + 1); // erasing utilityName from string

				Append_GnuScript(nameOfScript,colour,getpid(),tmpdir,numOfHandler);

				const char *nameOfFifos[workersCount];
				for (int j = 0; j < workersCount; j++) {
					// Create fifo name and fifo itself
					stringstream nameOfFifo;
					nameOfFifo << tmpdir << getpid() << "_w" << j << ".fifo";
					const char *fifoName = nameOfFifo.str().c_str();
					if (mkfifo(fifoName, 0622) == -1) {
						if ( errno != EEXIST) {
							char errorbuf[64];
							sprintf(errorbuf, "mkfifo. Name : %s", fifoName);
							perror(errorbuf);
							exit(EXIT_FAILURE);
						}
					}
					nameOfFifos[j] = Strduplicate(fifoName);
				}

				WorkersID* wid = (WorkersID*) malloc(workersCount * sizeof(WorkersID));

				/* Create N workers */
				for (int j = 0; j < workersCount; j++) {
					pid_t pidWorker = fork();
					if (!pidWorker) { // if worker do exec
						stringstream tokenwords(token);
						int argcUtility = GetArgs(utilityName);
						if (argcUtility == -1) // wrong name so child must exit
							exit(EXIT_FAILURE);

						//             -i +  -o +  -f +  -n +  -a & floats     + NULL
						int argcExec =  2  +  2  +  2  +  2  + 1 + argcUtility + 1;
						char *argvExec[argcExec];
						Create_ExecArgv(argvExec,inputBinaryFile,nameOfFifos[j],j,utilityPath.c_str(),pointsPerWorker,tokenwords);
for (int k = 0 ; k < argcExec ; k++)
	cout << argvExec[k] << endl;
						execvp(argvExec[0],argvExec);
						cout << "EXEC FAILED ON WORKER #" << getpid() << "with arguments :" << endl;
						for (int k = 0; k < argcExec; k++) {
							cout << argvExec[k] << endl;
						}
						exit(EXIT_FAILURE);
					} else {
						if (pidHandler != getpid()) // destroy rest useless handlers
							exit(0);
						cout << "Handler #" << getpid() << " created Worker #" << pidWorker << endl;
						wid[j].workerNum = j;
						wid[j].workerPID = pidWorker;
					}
				}
				/* HANDLER WORK */
				// Open and read from fifos
				int fdarr[workersCount];
				for (int j = 0; j < workersCount; j++) {
					if ((fdarr[j] = open(nameOfFifos[j], O_RDONLY | O_NONBLOCK )) < 0) {
						char errorbuf[64];
						sprintf(errorbuf,"Open_fifo : %s",nameOfFifos[j]);
						perror(errorbuf);
						exit(EXIT_FAILURE);
					}
				}
				// Create & open output file
				stringstream outputSS;
				outputSS << tmpdir << getpid() << ".out";
				const char *outFile = outputSS.str().c_str();
				ofstream output(outFile , ios::out);

				if (output.is_open()) {
					int status,bytes_r,childrenNum = workersCount;
					char stringRead[PIPE_BUF];
					pid_t pid;
					while (childrenNum > 0) {
					  pid = wait(&status);
					  int num = ReturnWorkerID(wid,workersCount,pid);
						if ((bytes_r = read(fdarr[num],stringRead,PIPE_BUF)) > 0) { //Read string characters
							stringRead[bytes_r] = '\0';		//Zero terminator
							cout << "Handler #" << getpid() << " received |" << bytes_r << "| bytes" << endl;// << stringRead << endl;
							fflush(stdout);
							output << stringRead;
							memset(stringRead,0,PIPE_BUF);
						}
					  close(fdarr[num]);
					  childrenNum--;  // 'Remove' children
					}
				} else
					cerr << "Handler #" << getpid() << " couldnt open output file!" << endl;
				output.close();
				free(wid);
				exit(1);
			}
			cout << "Parent #" << getpid() << " created Handler #" << pidHandler << endl; // Child can keep going and fork once
			wait(NULL); // wait for handlers to finish
			Exec_GnuScript(commandCount,tmpdir);

		}
		if (exitFlag == true)
			break;
	} while(!cin.eof());
	Remove_all(tmpdir);
	free(tmpdir);
	cout << "Program |shapes| has been terminated successfully" << endl;
	exit(EXIT_SUCCESS);
}


