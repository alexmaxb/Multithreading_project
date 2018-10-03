# Multithreading_project
This is code I wrote for class that uses two thread pools. One pool read from a files, 
then transfers names of websites to a buffer, then the other pool takes these names, 
uses DNS services to find the IP address, then prints this into an output file.


In order to build the program, the command "make" needs to be run in the terminal in 
the same directory as the c files. Once this has been done, the program can be run with 
"./multi-lookup <number of requesters> <number of resolvers> <requester log file, not 
serviced.txt> <resolver log file, not results.txt> [<datafile>...]". The program will 
create the files serviced.txt, with information about how many files each requester 
thread serviced, and results.txt, with the matches of hostnames and IP addresses. To 
remove the program file created and serviced.txt and results.txt, "make clean" can be 
run and will delete these files.
