/*
 * shapeHelperFunctions.cpp
 *
 *  Created on: Nov 14, 2016
 *      Author: alex
 */

#include <iostream>
#include <cerrno>
#include <sstream>
#include <fstream>
#include <string>
#include <cctype>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

#include "shapeHelperFunctions.h"

using namespace std;

char *Strduplicate(const char *source) {
	char *dest = (char *) malloc(strlen(source) + 1); // +1 = '\0'
	if (dest == NULL)
		return NULL;
	strcpy(dest, source);
	return dest;
}

void Check_directory(char *tmpdir) {
	const char *DIRarg = tmpdir;
	DIR* dir = opendir(DIRarg);
	if (dir) {
		/* Directory exists */
		closedir(dir);
	} else if (errno == ENOENT) {
		/* Directory does not exist */
		struct stat st = { 0 };
		if (stat(DIRarg, &st) == -1) {
			mkdir(DIRarg, 0700); // Create directory /w full permission for owner
		} else {
			perror("Couldn't create a directory");
			exit(EXIT_FAILURE);
		}
		cout << "Given path(tmpDir) was created because it did not exist."
				<< endl;
	}
}

string Get_cwd(string word) {
	if (word[0] == '.' && word[1] == '/') // for Exec_GnuScript command only
		word = word.substr(2);

    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    strcat(cwd,"/");
    strcat(cwd,word.c_str());
    string returnVal(cwd);
    return returnVal;
}

int GetArgs(string name) {
	if (name == "circle" || name == "square")
		return 3;
	else if (name == "semicircle" || name == "ring" || name == "ellipse")
		return 4;
	cout << "\tGiven utility name is wrong : " << name << " !Aborting command" << endl;
	return -1;
}

const char* Create_fifo(char *tmpdir, pid_t handler, int numberOfWorker) {
	stringstream nameOfFifo;
	nameOfFifo << tmpdir << handler << "_w" << numberOfWorker << ".fifo";
	const char *fifoName = nameOfFifo.str().c_str();

	if (mkfifo(fifoName, 0622) == -1) {
		if ( errno != EEXIST) {
			char errorbuf[64];
			sprintf(errorbuf, "mkfifo. Name : %s", fifoName);
			perror(errorbuf);
			exit(EXIT_FAILURE);
		}
	}
	return fifoName;
}

int Open_fifo(const char *fifoName) {
	cout << "openning : " << fifoName << endl;
	int fd;
	if ((fd = open(fifoName, O_RDONLY | O_NONBLOCK)) < 0) {
		char errorbuf[64];
		sprintf(errorbuf, "Open_fifo : %s", fifoName);
		perror(errorbuf);
		exit(EXIT_FAILURE);
	}
	return fd;
}

void Create_ExecArgv(char *returnArray[],char *inputFile,const char* fifoName,
		int workerNum,const char* utilityPath,int pointsPerWorker,stringstream& tokenw)
{
	returnArray[0] = Strduplicate(utilityPath);
	returnArray[1] = (char *)malloc(2);
	returnArray[1] = (char *)"-i";
	returnArray[2] = Strduplicate(inputFile);
	returnArray[3] = (char *)malloc(2);
	returnArray[3] = (char *)"-o";
	returnArray[4] = Strduplicate(fifoName);
	returnArray[5] = (char *)malloc(2);
	returnArray[5] = (char *)"-f";
	asprintf(&(returnArray[6]), "%d", workerNum * pointsPerWorker * 4);
	returnArray[7] = (char *)malloc(2);
	returnArray[7] = (char *)"-n";
	asprintf(&(returnArray[8]), "%d",pointsPerWorker);
	returnArray[9] = (char *)malloc(2);
	returnArray[9] = (char *)"-a";
	string word;
	int pos = 10;
	while (tokenw >> word) {
		if ( (pos == 13) && isalpha(word[0]) ) {
			returnArray[pos] = (char *)malloc(1);
			returnArray[pos] = Strduplicate(word.c_str());
		} else
			asprintf(&(returnArray[pos]), "%d",atoi(word.c_str()));
		pos++;
	}
	returnArray[pos] = (char *)malloc(1);
	returnArray[pos] = NULL;
}

int ReturnWorkerID(WorkersID* wid, int amount, pid_t pid) {
	for (int i = 0 ; i < amount ; i++) {
		if (wid[i].workerPID == pid)
			return i;
	}
	cout << "Worker with pid " << pid << " was not found!" << endl;
	return -1;
}


string Create_GnuScript(char *tmpdir,int cc) {
	stringstream ss;
	ss << tmpdir << cc << "_script.gnuplot";
	ofstream gnuFile(ss.str().c_str(),ios::out);
	gnuFile << "set terminal png" << endl;
	gnuFile << "set size ratio -1" << endl;
	gnuFile << "set xrange [-100:100]" << endl;
	gnuFile << "set yrange [-100:100]" << endl;
	gnuFile << "set output \"./" << cc << "_image.png\"" << endl;
	gnuFile << "plot ";
	gnuFile.close();
	return ss.str();
}

void Append_GnuScript(string nameToFile, string colour, pid_t handler,
		char *tmpdir,int num) {
	ofstream gnuFile(nameToFile.c_str(), ios::app);
	if (num != 0) {
		gnuFile << "\"./" << handler << ".out" << "\""
				<< " notitle with points pointsize 0.5 linecolor rgb \"" << colour
				<< "\", \\" << endl;
	} else {
		gnuFile << "\"./" << handler << ".out" << "\""
				<< " notitle with points pointsize 0.5 linecolor rgb \"" << colour
				<< "\"" << endl;

	}
	gnuFile.close();
}

void Exec_GnuScript(int commandCount, char* tmpdir) {
	pid_t pid;
	if ((pid = fork()) < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		chdir(tmpdir);
		stringstream ss;
		ss << "gnuplot " << commandCount << "_script.gnuplot";
		execl("/bin/sh", "sh", "-c", ss.str().c_str(), (char *) 0);
		cout << "Gnuplot exec failed" << endl;
	} else
		// parent waiting till child finish
		while (waitpid(pid, NULL, 0) > 0)
			;
}

void Remove_all(char * tmpdir) {
	DIR *d;
	struct dirent *dir;
	char const *cpath = tmpdir;
	d = opendir(cpath);
	int ret;
	if (d) {
		string p(tmpdir); // path
		while ((dir = readdir(d)) != NULL) {
			string delfile(dir->d_name);
			string delpath = p + delfile;
			if (delfile == "." || delfile == "..")
				// Ignore current('.') and previous('..') directory
				continue;
			ret = remove(delpath.c_str());
			if (ret == -1) {
				char errorbuf[124];
				sprintf(errorbuf, "\tCannot remove %s ", delpath.c_str());
				perror(errorbuf);
			} else if (ret == 0)
				cout << "\tRemoved : " << delpath << endl;
		}
	}
	rmdir(cpath);
}

void Fifo_write(float tmpX, float tmpY, int fd ,int offset) {
	int nwrite = 0;
	stringstream ss;
	ss.precision(1);
	ss << fixed << tmpX << "\t" << tmpY << endl;
	ss.seekg(0, ios::end);
	int size = ss.tellg();
	if ((nwrite = write(fd, ss.str().c_str(), size)) == -1) {
		char errorbuf[128];
		sprintf(errorbuf,"|WORKER| with offset '%d' Named-pipe write error",offset);
		perror(errorbuf);
		exit(EXIT_FAILURE);
	} else if (nwrite == EAGAIN) {
		char errorbuf[256];
		sprintf(errorbuf,
				"|WORKER| with offset '%d' Named-pipe write error.Not enough PIPE_BUF size for the current system.Run program with more workers",
				offset);
		perror(errorbuf);
		exit(EXIT_FAILURE);
	}
}

