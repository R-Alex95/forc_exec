/*
 * semicircle.cpp
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

int main(int argc,char **argv) {
	cout << "Program |semicircle| is running." << endl;
	char *inputBinaryFile,*outFile,cardinalDirection;
	int offset,pointsToReadCount;
	bool offsetExists = false , pointsExists = false;
	float utilityArgs[3];

	/* Getting arguments */
	if (argc < 10 || argc > 14) {
		cerr << "ERROR : Wrong arguments for semicircle.cpp!" << endl;
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
				// Save the certain 3 first floats
				for (int j = 0 ; j < 3 ; j++,i++)
					utilityArgs[j] = atof(argv[i]);
				cardinalDirection = *(argv[i]);
				if (cardinalDirection != 'N' && cardinalDirection != 'E'
						&& cardinalDirection != 'W' && cardinalDirection != 'S') {
					cerr << "Wrong input for cardinalDirection.Only N/E/W/S is acceptable, not " << cardinalDirection << endl;
					exit(EXIT_FAILURE);
				}
				i++;
			}
			else if (strncmp(argv[i],"-f",2) == 0) {
				offset = atoi(argv[i + 1]);
				offsetExists = true;
			}
			else if (strncmp(argv[i],"-n",2) == 0) {
				pointsToReadCount = atoi(argv[i + 1]);
				if (pointsToReadCount == 0) {
					cerr << "Don't run the program with points to read arguments equals to ZERO!!!" << endl;
					exit(EXIT_FAILURE);
				}
				pointsExists = true;
			}
			i = i + 2;
		}
	}

	cout.precision(1);
	cout << "Arguments : inputBinaryFile : " << inputBinaryFile << endl << "outFile : " << outFile << endl << "utilityArgs : ";
	for (int j = 0 ; j < 3 ; j++)
		cout << fixed << utilityArgs[j] << " ";
	cout << "& cardinalDirection : " << cardinalDirection << endl;
	if (offsetExists == true) {
		cout << "offset : " << offset << endl;
	}
	if (pointsExists == true) {
		cout << "pointsToReadCount : " << pointsToReadCount << endl;
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
			float dx = utilityArgs[0] - tmpX;
			float dy = utilityArgs[1] - tmpY;
			if (dx * dx + dy * dy <= utilityArgs[2] * utilityArgs[2]) {
				switch ( cardinalDirection ) {
				case 'N' :
					if ( tmpY >= utilityArgs[1] )
						Fifo_write(tmpX,tmpY,fd,offset);
					break;
				case 'E' :
					if ( tmpX >= utilityArgs[0] )
						Fifo_write(tmpX,tmpY,fd,offset);
					break;
				case 'S' :
					if ( tmpY <= utilityArgs[1] )
						Fifo_write(tmpX,tmpY,fd,offset);
					break;
				case 'W' :
					if ( tmpX <= utilityArgs[0] )
						Fifo_write(tmpX,tmpY,fd,offset);
					break;
				}
			}
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








