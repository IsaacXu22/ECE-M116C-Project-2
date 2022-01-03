/*
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>



#define CACHE_SETS 16
#define CACHE_SETS_SA 4
#define MEM_SIZE 2048 //i.e 12 bits total
#define CACHE_WAYS 1
#define BLOCK_SIZE 1 // bytes per block, i.e block size is 1 byte
#define DM 0
#define FA 1
#define SA 2


using namespace std;
struct cache_set
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for FA only
	int data; // the actual data stored in the cache/memory
	// add more things here if needed
};

struct trace
{
	bool MemR; 
	bool MemW; 
	int adr; 
	int data; 
};


Either implement your memory_controller here or use a separate .cpp/.c file for memory_controller and all the other functions inside it (e.g., LW, SW, Search, Evict, etc.)


Sets up the entire memory_controller function
It has inputs of current memR and memW, pointer to data, current address, current status, pointer to miss counter, pointer to our cache set, and our int array of memory


//our update function, which updates the LRU
void update()
{

}

//our evict function, which calls for a candidate to evict
//and also updates the counters, which is done by update()
//what we need to do is
//1 find a candidate for eviction
//2 update the counters by calling update
//3 update the tag and data stores with new value, probably from cache
void evict(int address, int* data, cache_set* myCache, int myMem[])
{

	update();
}

//our cache_miss function, which accesses the main memory and calls evict
//we read the data from main memory
void cache_miss(int address, int* data, cache_set* myCache, int myMem[])
{
	//first read the data from main memory
	int temp = myMem[address];

	//calls evict
	evict(address, data, myCache, myMem);
}

// our search function, which happens when it is called through load or through store
// both are essentially the same
// int instr teels us if its one or or the other
// if instr == 0, then its a load
// if instr == 1, then its a store
// our data is then updated with regards to instruction type
//
bool search(int address, int* data, int type, cache_set* myCache, int myMem[], int instr)
{
	//cout << "searching" << endl;
	cout << instr << endl;
	bool hit = 0;
	//first depending on the type we extract the offest, index, and tag from the address
	int offset_size, index_size;
	offset_size = log2(BLOCK_SIZE); //the number of bits that the address has of offset
	
	//if we have a DM, that means our index size is log2(cache size / block size)
	//
	if (type == DM)
	{
		index_size = log2(CACHE_SETS/BLOCK_SIZE);
	}
	int temp = address;

	//calculate the offset, index, and tag from the address inputted
	//theoretically for a DM it have 0 bit offset, 4 bit index, and 7 bit tag, since the main memory size is 2048 -> 11 bits
	int offset = temp & offset_size;
	int index = (temp >> offset_size) & int((pow(2, index_size)-1)); 
	int tag = ((temp >> offset_size) >> index_size) & int((pow(2, 11-offset_size-index_size))-1);

	int cache_tag = myCache[index].tag;
	//cout << cache_tag << endl;

	//for LW, we read the value from the data store
	//for SW, the data store is updated with the new value
	if (tag == cache_tag)
	{
		hit = true;

		//if its lw, we read the value from data store
		if (instr == 0)
		{
			*data = myCache[index].data;
		}
		//if its sw, then we update the data store with the new value
		else //instr == 1
		{
			myCache[index].data = *data;
			cout << "this is the data " << * data << endl;
		}
		//call update here
		update();
	}
	else
	{
		hit = false;
	}

	return hit;
}

//The load function happens when the status == 1 and cur_memR == 1
//what we do is call search, which requires inputs of current address, current data, and type
//Inputs:
// address: current address that we need to divy up
// data: either update or input it into address in cache
// type: type of cache we are working with
// myCache: our cache that we are working with
// myMem: our main memory that we are working with
void load(int address, int* data, int type, cache_set* myCache, int myMem[], int *status)
{
	//need to do *data to get the actual data and not the address
	//cout << "loading" << endl;

	bool hit = search(address, data, type, myCache, myMem, 0);

	// if we get a hit, assign status = 1
	if (hit) //hit == 1
	{
		*status = 1;
	}
	//if we get a miss, assign status = -3
	//also call cache_miss
	else //hit == 0
	{
		*status = -3;
		cache_miss(address, data, myCache, myMem);
	}
	//cout << status << " " << *status << endl;
}

void store(int address, int* data, int type, cache_set* myCache, int myMem[], int *status)
{
	bool hit = search(address, data, type, myCache, myMem, 1);

	//if we get a hit, the main memory is updated
	//if we get a miss, the main memory is updated, but  don't allocate a new line in the cache
	if (hit)
	{
		myMem[address] = *data;
		//allocate a new line in cache
		//this is done by finding the first tag with -1, and replacing that with everything 
	}
	else if (!hit)
	{
		myMem[address] = *data;
		//do not allocate a new line in cache
	}

	//assign status = 1 and data = 0
	*status = 1;
	*data = 0;
}

//our main memory_controller function
int memory_controller(bool cur_MemR,bool cur_MemW,int *cur_data,int cur_adr,int status,int *miss,int type,cache_set* myCache,int myMem[])
{
	//if status isn't 0 or 1, that means we are still "reading" from main memory, and thus can't do anything
	if (status != 0 and status != 1)
	{
		//cout << "status is not 0 or 1" << endl;
		return status;
	}
	//if status == 1 that means we have to do the finite machine, and do load/store, etc.
	//comments down below that describe each step roughly
	else if (status == 1)
	{
		//first we check the control signal, i.e whether or not its a load or a store
		//if its a load then memR = 1, if store then memW = 1
		if (cur_MemR == 1)
		{
			//call the load function here
			//cout << "status = load" << endl;
			load(cur_adr, cur_data, type, myCache, myMem, &status);
		}
		else if (cur_MemW == 1)
		{
			//cout << "status = store" << endl;
			//do store here
			store(cur_adr, cur_data, type, myCache, myMem, &status);

			//return from search

		}

	}
	//if status == 0 we output data=MM[adr] and let status = 1
	else if (status == 0)
	{
		//cout << "status = 0" << endl;
		//data = myMem[cur_adr]; do something about this later
		status = 1;
	}
	return status;
}



//first work with the assumption that we have a DM, i.e type = 0

int main (int argc, char* argv[]) // the program runs like this: ./program <filename> <mode>
{
	// input file (i.e., test.txt)
	string filename = argv[1];

	// mode for replacement policy
	int type;
	
	ifstream fin;

	// opening file
	fin.open(filename.c_str());
	if (!fin){ // making sure the file is correctly opened
		cout << "Error opening " << filename << endl;
		exit(1);
	}
	
	if (argc > 2)  
		type = stoi(argv[2]); // the input could be either 0 or 1 or 2 (for DM and FA and SA)
	else type = 0;// the default is DM.
	

	// reading the text file
	string line;
	vector<trace> myTrace;
	int TraceSize = 0;
	string s1,s2,s3,s4;
    while( getline(fin,line) )
      	{
            stringstream ss(line);
            getline(ss,s1,','); 
            getline(ss,s2,','); 
            getline(ss,s3,','); 
            getline(ss,s4,',');
            myTrace.push_back(trace()); 
            myTrace[TraceSize].MemR = stoi(s1);
            myTrace[TraceSize].MemW = stoi(s2);
            myTrace[TraceSize].adr = stoi(s3);
            myTrace[TraceSize].data = stoi(s4);
            //cout<<myTrace[TraceSize].MemW << endl;
            TraceSize+=1;
        }


	// defining a fully associative or direct-mapped cache
	cache_set myCache[CACHE_SETS]; // 1 set per line. 1B per Block
	int myMem [MEM_SIZE]; //main memory

	// initializing
	for (int i=0; i<CACHE_SETS; i++)
	{
		myCache[i].tag = -1; // -1 indicates that the tag value is invalid. We don't use a separate VALID bit. 
		myCache[i].lru_position = 0;  // 0 means lowest position
		myCache[i].data = 0;
	}

	// counters for miss rate
	int accessL = 0; //////
	int accessS = 0; 
	int miss = 0; // this has to be updated inside your memory_controller
	int status = 1; 
	int clock = 0;
	int traceCounter = 0;
	bool cur_MemR; 
	bool cur_MemW; 
	int cur_adr;
	int cur_data;
	bool hit; 
	// this is the main loop of the code
	while(traceCounter < TraceSize){
		if(status == 1)
		{
			cur_MemR = myTrace[traceCounter].MemR;
			cur_MemW = myTrace[traceCounter].MemW;
			cur_data = myTrace[traceCounter].data;
			cur_adr = myTrace[traceCounter].adr;
			hit = false; 
			traceCounter += 1;
			if (cur_MemR == 1)
				accessL += 1;
			else if (cur_MemW == 1)
				accessS += 1;
		}
		else if (status == 0)
		{
			cout << "true" << endl;
		}
		if (status < 0) //models the read delay in MM -> should take 4 clock cycles total
		{
			status += 1;
		}
		// YOUR CODE
		status = memory_controller (cur_MemR, cur_MemW, &cur_data, cur_adr, status, &miss, type, myCache, myMem); // in your memory controller you need to implement your FSM, LW, SW, and MM. 
		// those & values are pass by reference
		////////////
		clock += 1; 
	}
	// to make sure that the last access is also done
	while (status < 1) 
	{
		// YOUR CODE
		//status = memory_controller (cur_MemR, cur_MemW, &cur_data, cur_adr, status, &miss, type, myCache, myMem); // in your memory controller you need to implement your FSM, LW, SW, and MM. 
		////////////
		clock += 1;
	}
	float miss_rate = miss / (float)accessL; 
	
	// printing the final result

	// closing the file
	fin.close();

	//print out all the cache and cache values
	for (int i = 0; i < 16; i++)
	{
		cout << i << " " << myCache[i].tag << " " << myCache[i].data << endl;
	}

	return 0;
}
*/