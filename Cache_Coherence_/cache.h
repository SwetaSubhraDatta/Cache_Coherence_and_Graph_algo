/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>
#include <vector>
#include <iomanip>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;
using namespace std;

/****add new states, based on the protocol****/
enum hit_miss_t
{
    HIT,
    MISS
};
enum valid_dirty_states
{
    NULLPTR,
    VALID,
    DIRTY
};
enum {
	INVALID = 0,
    SHARED,
    MODIFIED,
    EXCLUSIVE,
    SE_DRAGON,
    EXCLUSIVE_DRAGON,
    SM_DRAGON,
    MODIFIED_DRAGON,
    SC_DRAGON

};


enum protocol
{
    MSI=0,
    MESI=1,
    DRAGON=2
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq;
   valid_dirty_states vdstates;
 
public:
   cacheLine()
   {
       tag = 0; Flags = 0; vdstates=NULLPTR;
   }

   ulong getTag()
   {
       return tag;
   }

   ulong getFlags()
   {
       return Flags;
   }
   valid_dirty_states get_vd_flags()
   {
       return vdstates;
   }

   ulong getSeq()
   {
       return seq;
   }

   void setSeq(ulong Seq)
   {
       seq = Seq;
   }

   void setFlags(ulong flags)
   {
       Flags = flags;
   }
   void setvd_flags(valid_dirty_states states)
   {
       vdstates=states;
   }

   void setTag(ulong a)
   {
       tag = a;
   }

   void invalidate()
   {
       tag = 0;
       Flags = INVALID;
   }//useful function

   bool isValid()
   {
       return
       ((Flags) != INVALID);
   }
};

class Cache
{
public:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;
   int cache2cache,flushes,mem_transactions,interventions,invalidations,Bus_traffic;
   protocol protocoltype;

   //******///
   //add coherence counters here///
   //******///

   vector<vector<cacheLine>>cache;
   ulong calcTag(ulong addr)
   {
       return (addr >> (log2Blk) );
   }
   ulong calcIndex(ulong addr)
   { return ((addr >> log2Blk) & tagMask);
   }
   ulong calcAddr4Tag(ulong tag)
   {
       return (tag << (log2Blk));
   }
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int,protocol);
   ~Cache()
   {
       cache.clear();
   }
public:
    void MSI_Snoop(int currentcache,int currentsate,int& nextstate,uchar op,ulong addr);
    void MESI_Snoop(int currentcache,int currentstate,int &nextstate,uchar op,ulong addr,int cachetocachetransfer,bool flush);
    void DRAGON_Snoop(int currentcache,int currentstate,int &nextstate,uchar op,ulong addr,int flag,bool flush);
    void MSI_Calculate_BusTraffic(cacheLine*,int cacheindex,uchar op,int);
    void MSI_Calculate_BusTraffic(hit_miss_t status,int cacheindex,uchar op,int);

   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);

private:
void send_global_BusRd_to_receivers(ulong,int&);
void send_global_Busrdx_to_receivers(ulong,int&);
void BusRdx_MSI(cacheLine*,int);
void BusRd_MSI(cacheLine*,int );
int  BusRd_MESI(cacheLine*,int );
int  BusRdx_MESI(cacheLine*,int);
void BusUpgr_MESI(ulong addr);
int BusRd_DRAGON(cacheLine*,int);
int BusUpdt_DRAGON(ulong addr);

public:
    float get_missrate()
    {
        return (float)((float)(readMisses+writeMisses)/(float)(reads+writes))*100;
    }

private:


    void updateLRU(cacheLine *);


public:
    void Access(ulong,uchar,int);
    hit_miss_t check_for_hit_miss(cacheLine*&,ulong address,int cacheindex);




   //******///
   //add other functions to handle bus transactions///
   //******///

};

class CPU
{
    int data;
public:
    static CPU* getinstance();
private:
    CPU()
    {
        data=0;
    }

    static CPU* instance;
public:
   int no_of_processors;
   vector<Cache>total_caches;
   void load_address_into_memory();
    void printStats(protocol p);
    void print_personal_info(int cache_size,int assoc,int blocksize,int processors,protocol p,string tracefile);
    void get_cache_config();

};

#endif
