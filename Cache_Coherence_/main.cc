/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <algorithm>


using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{

	CPU *cpu=cpu->getinstance();

	ifstream fin;
	//FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }


    int cache_size = atoi(argv[1]);
    int cache_assoc= atoi(argv[2]);
    int blk_size   = atoi(argv[3]);
    int num_processors = atoi(argv[4]);
    cpu->no_of_processors=num_processors;
    int protocol_i=atoi(argv[5]);
	auto type_of_protocol   = (protocol)(protocol_i);
    int processor_index=-1;
    uchar read_or_write;
    ulong addr;
 	string filename= (const char*)(argv[6]);
     string line;

    cpu->print_personal_info(cache_size,cache_assoc,blk_size,num_processors,type_of_protocol,filename);
    for(int i=0;i<cpu->no_of_processors;i++)
    {
        cpu->total_caches.emplace_back(Cache(cache_size,cache_assoc,blk_size,type_of_protocol));
    }

	fin.open(filename);
	if(!fin.is_open())
	{   
		printf("Trace file problem\n");
		exit(0);
	}

//Reading the trace file now
 while(getline(fin,line))
 {
     cacheLine *currentaddress= nullptr;
     line.erase(std::remove(line.begin(), line.end(), ' '), line.end());//Erase Spaces
     processor_index = (line[0]) - '0';//Get the cachenumber or the cache which is attached to the processor
     read_or_write = line[1];//get read_or_write
     addr = stoul(line.substr(2, line.length() - 1), nullptr, 16);//get address and convert string to base 16
     int current_state = INVALID;
     int next_state = INVALID;
     int cacheToCacheTransfers=0;
     int flag=0;
     bool flush=false;
     hit_miss_t status=cpu->total_caches[processor_index].check_for_hit_miss(currentaddress,addr,processor_index);
     if (status== HIT) { //if HIT
         current_state = currentaddress->getFlags(); //get the current state
     }
    switch(type_of_protocol)
    {
            case MSI: {
                cpu->total_caches[processor_index].MSI_Snoop(processor_index, current_state,next_state, read_or_write,addr);
                cpu->total_caches[processor_index].MSI_Calculate_BusTraffic(status, processor_index,read_or_write, current_state);
                cpu->total_caches[processor_index].Access(addr, read_or_write, next_state);
                break;
            }
            case MESI:
            {
                cpu->total_caches[processor_index].MESI_Snoop(processor_index,current_state,next_state,read_or_write,addr,cacheToCacheTransfers,flush);
                cpu->total_caches[processor_index].Access(addr,read_or_write,next_state);
                break;
            }

            case DRAGON:
            {
                cpu->total_caches[processor_index].DRAGON_Snoop(processor_index,current_state,next_state,read_or_write,addr,flag,flush);
                cpu->total_caches[processor_index].Access(addr,read_or_write,next_state);
                break;

            }
            default:
            {
                cerr<<"Protocol not found or implemented "<<endl;
                exit(EXIT_FAILURE);
            }

    }

 }



 cpu->printStats(type_of_protocol);
	fin.close();
    delete cpu;

	//********************************//
	//print out all caches' statistics //
	//********************************//
	
}
