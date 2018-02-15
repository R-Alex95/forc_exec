/*
 * square.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: alex
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib> // atof
#include <cstdio>
#include <cerrno>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <math.h>

#include "shapeHelperFunctions.h"

using namespace std;

// Absolute value macro
#define abs(x) ( (x) < 0 ? - (x) : (x) )

int main(int argc,char **argv) {
	cout << "Program |square| is running." << endl;
	char *inputBinaryFile,*outFile;
	int offset,pointsToReadCount;
	bool offsetExists = false , pointsExists = false;
	float utilityArgs[3];

	/* Getting arguments */
	if (argc < 9 || argc > 13) {
		cerr << "ERROR : Wrong arguments for square.cpp!" << endl;
		//cerr << "EXAMPLE : ./utilityX -i InputBinaryFile -o OutputFile -a UtilityArgs [-f Offset] [-n PointsToReadCount]" << endl;
		exit(EXIT_FAILURE);
	}
	else {
		int i = 1;
		while (i < argc) {
			if (strncmp(argv[i],"-i",2) == 0) {
				inputBinaryFile = argv[i + 1];
			}
			else if (strncmp(argv[i],"-o",2) == 0) {
				outFile = argv[i + 1];
			}
			else if (strncmp(argv[i],"-a",2) == 0) {
				i++;
				for (int j = 0 ; j < 3 ; j++,i++)
					utilityArgs[j] = atof(argv[i]);
			}
			else if (strncmp(argv[i],"-f",2) == 0) {
				offset = atoi(argv[i + 1]);
				offsetExists = true;
			}
			else if (strncmp(argv[i],"-n",2) == 0) {
				pointsToReadCount = atoi(argv[i + 1]);
				pointsExists = true;
			}
			i = i + 2;
		}
	}

	cout.precision(1);
	cout << "Arguments : inputBinaryFile : " << inputBinaryFile << endl << "outFile : " << outFile << endl << "utilityArgs : ";
	for (int j = 0 ; j < 3 ; j++)
		cout << fixed << utilityArgs[j] << " ";
	cout << endl;
	if (offsetExists == true) {
		cout << "offset : " << offset << endl;
	}
	if (pointsExists == true) {
		cout << "pointsToReadCount : " << pointsToReadCount << endl;
		if (pointsToReadCount == 0) {
			cerr << "Don't run the program with points to read arguments equals to ZERO!!!" << endl;
			exit(EXIT_FAILURE);
		}
	}

    ifstream ifile (inputBinaryFile, ios::in | ios::binary);
    int fd;
	if ((fd = open(outFile, O_WRONLY)) < 0) {
		char errorbuf[128];
		sprintf(errorbuf,"|WORKER| with offset '%d' Named-pipe open error",offset);
		perror(errorbuf);
		exit(EXIT_FAILURE);
	}
    float tmpX = 0.0,tmpY = 0.0;
    int pointscounter = 0;
    if ( ifile.is_open() ) {
    	if (offsetExists) {
    		/* Getting file length and if offset falls within file , point to that specific byte */
    		ifile.seekg(0, ios::end);
    		int length = ifile.tellg();
    		if ( offset > length ) {
    			cerr << "Offset bytes exceed file length!\nProgram Exiting..." << endl;
    			exit(EXIT_FAILURE);
    		}
    		else
    			ifile.seekg(offset,ios::beg);
    	}
		while (!ifile.eof()) {
			ifile.read( (char*)(&tmpX), sizeof(float) );
			ifile.read( (char*)(&tmpY), sizeof(float) );
			if (abs(tmpX - utilityArgs[0]) + abs(tmpY - utilityArgs[1]) <= utilityArgs[2] )
				Fifo_write(tmpX,tmpY,fd,offset);
			if (pointsExists == true) {
				pointscounter++;
				if (pointscounter >= pointsToReadCount)
					break;
			}
		}
    }
    else {
    	perror ("Failed to open InputFile\n");
    }
    ifile.close();
    close(fd);
    exit(EXIT_SUCCESS);
}

