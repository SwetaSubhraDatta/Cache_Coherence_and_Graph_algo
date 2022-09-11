/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <algorithm>
#include "cache.h"
using namespace std;

CPU* CPU::instance= nullptr;

Cache::Cache(int s,int a,int b,protocol p )
{
    ulong i, j;
    //*******************//
    //initialize your counters here//
    //*******************//

    reads = readMisses = writes = 0;
   writeMisses = writeBacks = currentCycle = 0;
   protocoltype=p;
   interventions=0;
   invalidations=0;
   cache2cache=0;
   flushes=0;
   mem_transactions=0;
   protocoltype=p;
   interventions=0;
   Bus_traffic=0;


   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  

   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/
   cache= vector<vector<cacheLine>>(sets,vector<cacheLine>(assoc));
   for(i=0; i<sets; i++)
   {
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

CPU *CPU::getinstance()
{
    if(!instance)
    {
        instance=new CPU;
    }
    return instance;
}

hit_miss_t Cache::check_for_hit_miss(cacheLine*& address,ulong adress,int cacheindex)
{
    CPU *cpu=cpu->getinstance();
    address=cpu->total_caches[cacheindex].findLine(adress);
    if(address== nullptr)
    {
        return MISS;
    }
    else
        return HIT;

}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op,int state)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
        newline->setvd_flags(valid_dirty_states::DIRTY);
   		newline->setFlags(state);
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
        line->setvd_flags(valid_dirty_states::DIRTY);
        line->setFlags(state) ;
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == MODIFIED &&victim->get_vd_flags()==valid_dirty_states::DIRTY) writeBacks++;
    if(victim->getFlags() == MODIFIED_DRAGON && victim->get_vd_flags()==valid_dirty_states::DIRTY)
    {
        writeBacks++;
    }
    if(victim->getFlags()==SM_DRAGON && victim->get_vd_flags()==valid_dirty_states::DIRTY)
    {
        writeBacks++;
    }
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setvd_flags(valid_dirty_states::VALID);
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::MSI_Snoop(int currentcache, int currentstate, int &nextstate, uchar op, ulong addr)
{
    int Cachetocache;
    CPU *cpu=cpu->getinstance();
    switch (currentstate) {
        case INVALID://if current state is invalid
        {
            if (op == 'r') {
                nextstate = SHARED;
                cpu->total_caches[currentcache].mem_transactions++;
            }
            if (op == 'w') {
                nextstate = MODIFIED;
                cpu->total_caches[currentcache].mem_transactions++;
            }
            for (int i = 0; i < cpu->no_of_processors; i++) {
                if (i != currentcache) {
                    if (op == 'r')
                        cpu->total_caches[currentcache].send_global_BusRd_to_receivers(addr,Cachetocache);
                    else
                        cpu->total_caches[currentcache].send_global_Busrdx_to_receivers(addr,Cachetocache);
                }
            }

            break;
        }

        case SHARED: {
            if (op == 'w') {
                for (int i = 0; i < cpu->no_of_processors; i++) {
                    if (i != currentcache)
                        cpu->total_caches[i].send_global_Busrdx_to_receivers(addr,Cachetocache);
                }
                nextstate = MODIFIED;
                cpu->total_caches[currentcache].mem_transactions++;
            }

            if(op=='r')
                    nextstate = currentstate;
            break;
        }

        case MODIFIED:
        {
            nextstate=currentstate;
            break;
        }

    }
}

void Cache::send_global_BusRd_to_receivers(ulong addr,int&c2ctransfer)
{
    int currentstate=INVALID;
    cacheLine *currentaddress= findLine(addr);//find the line in other processors
    if(currentaddress!= nullptr)
    {
         currentstate=currentaddress->getFlags();
    }
    if(protocoltype==MSI)
    {
        BusRd_MSI(currentaddress,currentstate);
    }
    if(protocoltype==MESI)
    {
        c2ctransfer= BusRd_MESI(currentaddress,currentstate);
    }
    if(protocoltype==DRAGON)
    {
        c2ctransfer=BusRd_DRAGON(currentaddress,currentstate);
    }
}

void Cache::send_global_Busrdx_to_receivers(ulong addr,int&c2c)
{
    int currentstate=INVALID;
    cacheLine *currentaddress= findLine(addr);
    if(currentaddress!= nullptr)
    {
        currentstate=currentaddress->getFlags();
    }
    if(protocoltype==MSI)
    {
        BusRdx_MSI(currentaddress,currentstate);
    }
    if(protocoltype==MESI)
    {
        c2c= BusRdx_MESI(currentaddress,currentstate);

    }
    if(protocoltype==DRAGON)
    {
        cerr<<"Wrong call Dragon only does busupdates "<<endl;
        exit(1);
    }

}

void Cache::BusRdx_MSI(cacheLine * currentaddress, int current_state)
{
    if(currentaddress== nullptr)
    {
        current_state=INVALID;
    }
    switch(current_state)
    {
        case INVALID:
            break;
        case SHARED:
            currentaddress->setFlags(INVALID);
            invalidations++;
            break;
        case MODIFIED:
            currentaddress->setFlags(INVALID);
            invalidations++;
            writeBacks++;
            flushes++;
            break;
    }

}

void Cache::BusRd_MSI(cacheLine *currentaddress,int current_state)
{
    if(currentaddress== nullptr)
    {
        current_state=INVALID;
    }
    switch (current_state)
    {
        case INVALID:

            break;
        case SHARED:
            currentaddress->setFlags(SHARED);
            break;
        case MODIFIED:
            currentaddress->setFlags(SHARED);
            writeBacks++;
            interventions++;
            flushes ++;
            break;
    }
}


void Cache::MSI_Calculate_BusTraffic(hit_miss_t status,int cacheindex,uchar op,int current_state)
{
    CPU *cpu=cpu->getinstance();
    //if there is a miss
    if(status==MISS)
    {
        if(op=='w')
            cpu->total_caches[cacheindex].Bus_traffic++;
    }
        //if there is a hit
    else {
        if(op == 'w' && current_state == SHARED) cpu->total_caches[cacheindex].Bus_traffic++;
    }

}

/*****FOR MESI***/
int Cache::BusRd_MESI(cacheLine *current, int currenState)
{
    switch (currenState) {
        case INVALID:
            return 0;
        case SHARED:
            return 1;
        case EXCLUSIVE:
            current->setFlags(SHARED);
            interventions++;
            return 1;
        case MODIFIED:
            current->setFlags(SHARED);
            writeBacks++;
            flushes++;
            interventions++;
            return 1;
    }
    return 0x33;//error code
}
int Cache::BusRdx_MESI(cacheLine * current, int currentstate)
{
    switch (currentstate) {
        case INVALID:
            return 0;
        case SHARED:
            current->setFlags(INVALID);
            invalidations++;
            return 1;
        case EXCLUSIVE:
            current->setFlags(INVALID);
            invalidations++;
            return 1;
        case MODIFIED:
            current->setFlags(INVALID);
            writeBacks++;
            flushes++;
            invalidations++;
            return 1;
        default:
            return 0x33;//error code
    }


}

void Cache::BusUpgr_MESI(ulong addr)
{
    int currentState = INVALID;
    cacheLine *current = findLine(addr);
    if (current != nullptr)
        currentState = current->getFlags();

        switch (currentState)
        {
            case SHARED: {
                current->setFlags(INVALID);
                invalidations++;
                return;
            }


    }
}
void Cache::MESI_Snoop(int currentcache, int currentstate, int &nextstate, uchar op,
                                                      ulong addr, int cache_to_cache_transfers,bool flush)

{
    int cache_tocache=0;
    CPU *cpu=cpu->getinstance();
    switch(currentstate)
    {
        case INVALID:
            for (int i = 0; i < cpu->no_of_processors; i++) {
                if (i != currentcache) {
                    if (op == 'r') {
                        cpu->total_caches[i].send_global_BusRd_to_receivers(addr, cache_to_cache_transfers);
                        cache_tocache += cache_to_cache_transfers;
                    }
                    else {
                        cpu->total_caches[i].send_global_Busrdx_to_receivers(addr, cache_to_cache_transfers);
                        flush = flush | cache_to_cache_transfers;
                    }
                }
            }

            if (op == 'r') {
                if (cache_tocache != 0) {
                    nextstate = SHARED;
                    cpu->total_caches[currentcache].cache2cache++;
                } else {
                    nextstate = EXCLUSIVE;
                    cpu->total_caches[currentcache].mem_transactions++;
                }

            } else {
                nextstate = MODIFIED;
                //caches[proc_no].mem_transac++;
                if (flush){
                    cpu->total_caches[currentcache].cache2cache++;

                }
                else
                {
                    cpu->total_caches[currentcache].mem_transactions++;
                }
                cpu->total_caches[currentcache].Bus_traffic++;
            }
            break;

        case EXCLUSIVE:
            if (op == 'w') {
                nextstate= MODIFIED;
            } else
                nextstate= currentstate;
            break;
        case SHARED:
            if (op == 'w') {
                for (int i = 0; i < cpu->no_of_processors; i++) {
                    if (i != currentcache)
                        cpu->total_caches[i].BusUpgr_MESI(addr);
                }
                nextstate = MODIFIED;
            } else
                nextstate=currentstate;
            break;

        case MODIFIED:
            nextstate = currentstate;
            break;
    }
}

int Cache::BusRd_DRAGON(cacheLine * current ,int currentstate)
{

        switch (currentstate)
        {
            case INVALID:
                return 0;
            case EXCLUSIVE_DRAGON:
                current->setFlags(SC_DRAGON);
                interventions++;
                return 1;

            case SC_DRAGON:
                return 1;

            case SM_DRAGON:
                flushes++;
                return 1;

            case MODIFIED_DRAGON:
                current->setFlags(SM_DRAGON);
                interventions++;
                mem_transactions++;
                flushes++;
                return 1;
        }
    return 0x33;// error code


    }

int Cache::BusUpdt_DRAGON(ulong addr)
{
    CPU *cpu=cpu->getinstance();
    int currentstate=INVALID;
    cacheLine * currentaddress= findLine(addr);
    if(currentaddress!= nullptr)
    {
        currentstate=currentaddress->getFlags();
    }
    switch(currentstate)
    {
        case SC_DRAGON:
            return 1;
        case SM_DRAGON:
            currentaddress->setFlags(SC_DRAGON);
            return 1;
        default:
            return 0;
    }
}



void Cache::DRAGON_Snoop(int currentcache, int currentstate, int &nextstate, uchar op,ulong addr,int flag,bool flush)
{
    int statefill=0;
    CPU *cpu=cpu->getinstance();
    switch(currentstate)
    {
        case INVALID: {
            for (int i = 0; i < cpu->no_of_processors; i++) {
                if (i != currentcache) {
                    cpu->total_caches[i].send_global_BusRd_to_receivers(addr, statefill);
                    flag = flag + statefill;
                }
            }

            if (op == 'r') {
                if (flag != 0) {
                    nextstate = SC_DRAGON;
                } else {
                    nextstate = EXCLUSIVE_DRAGON;
                }

            }
            if (op == 'w') {
                if (flag != 0) {
                    nextstate = SM_DRAGON;

                    for (int i = 0; i < cpu->no_of_processors; i++) {
                        if (i != currentcache) {
                            cpu->total_caches[i].BusUpdt_DRAGON(addr);
                        }
                    }

                } else {
                    nextstate = MODIFIED_DRAGON;
                }
            }
            break;
        }
        case EXCLUSIVE_DRAGON:
        {
            if (op == 'w') {
                nextstate = MODIFIED_DRAGON;
            } else
                nextstate = currentstate;
            break;
        }
        case SC_DRAGON:
        {
            if (op == 'w') {
                for (int i = 0; i < cpu->no_of_processors; i++) {
                    if (i != currentcache)
                        flush = flush | cpu->total_caches[i].BusUpdt_DRAGON(addr);
                }
                if (flush)
                    nextstate = SM_DRAGON;
                else {
                    nextstate = MODIFIED_DRAGON;
                }
            } else
                nextstate = currentstate;
            break;
        }
        case MODIFIED_DRAGON:
                    {
            nextstate=currentstate;
            break;
                    }
        case SM_DRAGON:
        {
            if(op == 'w')
            {

                for(int i=0; i < cpu->no_of_processors; i++){
                    if(i != currentcache)
                        flush = flush | cpu->total_caches[i].BusUpdt_DRAGON(addr);
                }
                if(flush)
                {
                    nextstate = currentstate;
                }
                else
                {
                    nextstate = MODIFIED_DRAGON;
                }
            }

            else
            {
                nextstate = currentstate;
            }
            break;
        }

    }

}





void CPU::printStats(protocol p)
{
    ulong mem_transactions=0;
CPU *cpu=cpu->getinstance();
for(int i=0;i<cpu->no_of_processors;i++)
{
    if(p==MSI || p==MESI)
    {
        mem_transactions=cpu->total_caches[i].mem_transactions+cpu->total_caches[i].writeBacks;
    }
    if(p == DRAGON)
    {
        mem_transactions = total_caches[i].readMisses + total_caches[i].writeMisses+ total_caches[i].writeBacks;
    }
    cout<<"============ Simulation results ("<<"Cache "<<to_string(i)<<")"<<" ==============="<<endl;
    cout<<" 01. number of reads:"<<setw(30)<<cpu->total_caches[i].reads<<endl;
    cout<<" 02. number of read misses:"<<setw(24)<<cpu->total_caches[i].readMisses<<endl;
    cout<<" 03. number of writes:"<<setw(29)<<cpu->total_caches[i].writes<<endl;
    cout<<" 04. number of write misses:"<<setw(23)<<cpu->total_caches[i].writeMisses<<endl;
    cout<<" 05. total miss rate:"<<setw(30)<<fixed<<setprecision(2)<<cpu->total_caches[i].get_missrate()<<"%"<<endl;
    cout<<" 06. number of writebacks: "<<setw(24)<<cpu->total_caches[i].writeBacks<<endl;
    cout<<" 07. number of cache-to-cache transfers:"<<setw(11)<<cpu->total_caches[i].cache2cache<<endl;
    cout<<" 08. number of memory transactions:"<<setw(16)<<mem_transactions<<endl;
    cout<<" 09. number of interventions:"<<setw(22)<<cpu->total_caches[i].interventions<<endl;
    cout<<" 10. number of invalidations:"<<setw(22)<<cpu->total_caches[i].invalidations<<endl;
    cout<<" 11. number of flushes"<<setw(29)<<cpu->total_caches[i].flushes<<endl;
    cout <<" 12. number of BusRdX:"<<setw(29)<<cpu->total_caches[i].Bus_traffic<<endl;

}
}

void CPU::print_personal_info(int cache_size, int assoc,int blocksize, int processors, protocol p, string tracefile)
{
    cout<<"===== 506 Personal information ====="<<endl;
    cout<<"Sweta Subhra Datta"<<endl;
    cout<<"ssdatta"<<endl;
    cout<<"ECE 406 Student::NO"<<endl;
    cout<<"===== 506 SMP Simulator configuration ====="<<endl;
    cout<<"L1_SIZE: "<<cache_size<<endl;
    cout<<"L1_ASSOC: "<<assoc<<endl;
    cout<<"L1_BLOCKSIZE: "<<blocksize<<endl;
    cout<<"NUMBER OF PROCESSORS:"<<processors<<endl;
    if(p== MSI)
    {
        cout<<"COHERENCE PROTOCOL:"<<"MSI"<<endl;

    }
    else if(p==MESI)
    {
        cout<<"COHERENCE PROTOCOL :"<<"MESI"<<endl;
    }
    else if(p==DRAGON)
    {
        cout<<"COHERENCE PROTOCOL :"<<"DRAGON"<<endl;
    }
    else
    {
        cerr<<"Wrong protocol chosen"<<endl;
        exit(0);
    }
    cout<<"TRACE FILE: "<<tracefile<<endl;

}
