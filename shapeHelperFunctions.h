/*
 * shapeHelperFunctions.h
 *
 *  Created on: Nov 14, 2016
 *      Author: alex
 */

#ifndef SHAPEHELPERFUNCTIONS_H_
#define SHAPEHELPERFUNCTIONS_H_

struct WorkersID {
	pid_t workerPID;
	int workerNum;
};

int ReturnWorkerID(WorkersID* , int , pid_t );
// duplicate c-string
char *Strduplicate (const char *source);
// Check if directory exists , if not create it.
void Check_directory(char *);
// Getting the current working directory and returning the path to utility
std::string Get_cwd(std::string);
// Returning how many arguments(for the -a parameter) the utility has
int GetArgs(std::string);

int Open_fifo(const char *);

// Creates the arguments-array for execv.
void Create_ExecArgv(char *[],char*,const char*,int,const char*,int,std::stringstream&);

std::string Create_GnuScript(char *, int);
void Append_GnuScript(std::string , std::string ,pid_t,char *,int);
void Exec_GnuScript(int, char *);
// Removing everything in tmpDirectory
void Remove_all(char* );

void Fifo_write(float tmpX, float tmpY, int fd ,int offset);

#endif /* SHAPEHELPERFUNCTIONS_H_ */
