#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>

#define CACHE_SETS 16
#define SA_WAYS 4
#define MEM_SIZE 2048
#define CACHE_WAYS 1
#define BLOCK_SIZE 1 // bytes per block
#define DM 0
#define FA 1
#define SA 2

using namespace std;
struct cache_set
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for FA only
	int data; // the actual data stored in the cache/memory
	int sa_index; //the index if its a 4-way set associative, since we have to do different stuff to it
	// add more things here if needed
};

struct trace
{
	bool MemR;
	bool MemW;
	int adr;
	int data;
};

/*
Either implement your memory_controller here or use a separate .cpp/.c file for memory_controller and all the other functions inside it (e.g., LW, SW, Search, Evict, etc.)
*/

//implementation of update function
//we update the lru positions of myCache by getting the index value of the cache
//every lru position less than it stays the same
//every lru position greater than it decrements by 1
//then the lru position of the info at that index turns into 15
void update(int lru_counter, cache_set* myCache, int type)
{
	//cout << "updating" << endl;

	if (type == 0)
	{
		return;
	}
	//we ignore the index, since its only for SA
	else if (type == FA)
	{
		int lru_counter_position = myCache[lru_counter].lru_position;
		for (int i = 0; i < CACHE_SETS; i++)
		{
			if (myCache[i].lru_position > lru_counter_position)
			{
				myCache[i].lru_position -= 1;
			}
			else if (i == lru_counter)
			{
				myCache[i].lru_position = 15;
			}
		}
	}
	else //type == SA
	{
		int index_position = myCache[lru_counter].sa_index;
		int lru_counter_position = myCache[lru_counter].lru_position;
		for (int i = 0; i < CACHE_SETS; i++)
		{
			if (myCache[i].lru_position > lru_counter_position && myCache[i].sa_index == index_position)
			{
				myCache[i].lru_position -= 1;
			}
			else if (i == lru_counter && myCache[i].sa_index == index_position)
			{
				myCache[i].lru_position = 3;
			}
		}
	}
}

//implementation of evict function
//this needs to do 3 things
// first - find a candidate for eviction, i.e lru_position = 0
// second - update the counters by calling update()
// third - update ht etag and data store values with new values
//
void evict(int* cur_data, int* cur_adr, cache_set* myCache, int mm_value, int type)
{
	//cout << "evicting" << endl;

	//first do DM: there is no LRU for a direct map, so therefore just replace stuff via index

	//first iterate through the entire cache
	//if there is lru_position = 0, break the loop
	//our lru_counter keeps track of which index we plan to place in the new data value
	//we are also assuming there will always be an lru_position = 0
	int lru_loc;
	if (type == DM)
	{
		update(lru_loc, myCache, type);
	}
	else if (type == FA)
	{
		for (int i = 0; i < CACHE_SETS; i++)
		{
			if (myCache[i].lru_position == 0)
			{
				lru_loc = i;
				break;
			}
		}
		//update the tag and data_stores with the new values here
		update(lru_loc, myCache, type);
	}
	//we need the index for SA, so i'll put that off until the index has been calculated down below
	

	//update the tag and data_stores with the new values here
	//update(lru_loc, myCache, type);

	//basically just copy/paste the divying code from the search function

	//first depending on the type we extract the offest, index, and tag from the address
	int offset_size, index_size;
	offset_size = log2(BLOCK_SIZE); //the number of bits that the address has of offset

	//if we have a DM, that means our index size is log2(cache size / block size)
	//
	if (type == DM)
	{
		index_size = log2(CACHE_SETS / BLOCK_SIZE);
	}
	else if (type == FA)
	{
		index_size = 0;
	}
	else //type == SA
	{
		index_size = log2(CACHE_SETS / BLOCK_SIZE / SA_WAYS);
	}

	//tag size is the log2 of our main memory size - index - offest size
	int tag_size = log2(MEM_SIZE) - index_size - offset_size;

	//Theoretically for a DM, offset = 0 bits, index = 4 bits, which means tag = 11 - 4 - 0 = 7 bits
	int temp = *cur_adr;
	//cout << temp << endl;

	//now we calculate actual offset, index, and tag
	int offset = temp & int(pow(2, offset_size) - 1);
	int index = (temp >> offset_size) & int(pow(2, index_size) - 1);
	int tag = ((temp >> offset_size) >> index_size) & int(pow(2, tag_size) - 1);

	//this is where we get the lru for our SA 
	//since it required the index to see which way we are working with
	if (type == SA)
	{
		for (int j = 0; j < CACHE_SETS; j++)
		{
			if (myCache[j].sa_index == index && myCache[j].lru_position == 0)
			{
				lru_loc = j;
				break;
			}
		}
		update(lru_loc, myCache, type);
	}

	//update the tag and data stores with the new values from  mm
	if (type == DM)
	{
		myCache[index].tag = tag;
		myCache[index].data = mm_value;
	}
	else if (type == FA)
	{
		myCache[lru_loc].data = mm_value;
		myCache[lru_loc].tag = tag;
	}
	else if (type == SA)
	{
		myCache[lru_loc].data = mm_value;
		myCache[lru_loc].tag = tag;
	}
}

//implementation of cache miss function
//this happens right after we miss and also are trying to do a load function
// in a cache miss we read the data from main memory
// then call the evict function
//
void cache_miss(int* cur_data, int* cur_adr, cache_set* myCache, int myMem[], int type)
{
	//cout << "cache missing" << endl;
	//first read the data from main memory
	int mm_value = myMem[*cur_adr];
	//cout << temp << endl;
	evict(cur_data, cur_adr, myCache, mm_value, type);
}

//implementation of search function
//there are at least two inputs, and a boolean output
//using the address, we divy up the offset, index, and tag
//then we access the cache
//how we access changes with what kind of cache we are working with
bool search(int* cur_data, int* cur_adr, int* miss, int type, cache_set* myCache, int myMem[], int instruction)
{
	//cout << "searching" << endl;
	bool hit;

	//first depending on the type we extract the offest, index, and tag from the address
	int offset_size, index_size;
	offset_size = log2(BLOCK_SIZE); //the number of bits that the address has of offset

	//if we have a DM, that means our index size is log2(cache size / block size)
	//if we have a fully associated cache, we don't have an index, and therefore its just the tag
	//if we have a SA, that means our index size is log2(cache size / block size / # of sets)
	if (type == DM)
	{
		index_size = log2(CACHE_SETS / BLOCK_SIZE);
	}
	else if (type == FA)
	{
		index_size = 0;
	}
	else if (type == SA)
	{
		index_size = log2(CACHE_SETS / BLOCK_SIZE / SA_WAYS);
	}

	//tag size is the log2 of our main memory size - index - offest size
	int tag_size = log2(MEM_SIZE) - index_size - offset_size;

	//Theoretically for a DM, offset = 0 bits, index = 4 bits, which means tag = 11 - 4 - 0 = 7 bits
	//for FA it should be tag = 11 bits, everything else = 0 bits
	//for SA it should be offset = 0 bits, index = 2 bits, which means tag = 11 - 2 - 0 bits
	int temp = *cur_adr;
	//cout << temp << endl;

	//now we calculate actual offset, index, and tag
	int offset = temp & int(pow(2, offset_size) - 1);
	int index = (temp >> offset_size) & int(pow(2, index_size) - 1);
	int tag = ((temp >> offset_size) >> index_size) & int(pow(2, tag_size) - 1);

	//cout << temp << " " << offset << " " << index << " " << tag << endl;

	//if its a DM, we search for the cache's index
	//and look for the tags
	if (type == DM)
	{
		//now we access the cache
		int stored_tag = myCache[index].tag;

		//compare our stored tag and our computed tag
		//if they match, its a hit
		//if not, then its a miss
		if (tag == stored_tag)
		{
			hit = true;

			//do different stuff
			//if its a load, our data is replaced by the cache data, i.e instr = 0
			//if its a store, the cache data is replaced by our data, i.e instr = 1
			if (instruction == 1) //store
			{
				myCache[index].data = *cur_data;
			}
			else if (instruction == 0) //instruction == 0, i.e load
			{
				*cur_data = myCache[index].data;
				update(index, myCache, type);
			}
		}
		else
		{
			hit = false;
			//cout << *miss << endl;
		}
	}
	//if its a FA, we search through all the sets
	else if (type == FA)
	{
		//the counter tells us where exactly in the FA the tag is located
		hit = false;
		int loc;

		//iterate through all sets in the FA
		//if we get matching tags, update the counter to that location, and it's a hit
		//if we don't, it's a miss
		for (int i = 0; i < CACHE_SETS; i++)
		{
			if (myCache[i].tag == tag)
			{
				loc = i;
				hit = true;
			}
		}

		//if we did get a hit
		if (hit)
		{
			//sw: update the data store with the new value
			if (instruction == 1)
			{
				myCache[loc].data = *cur_data;
			}
			//lw: update *data with the value from cache
			else if (instruction == 0)
			{
				*cur_data = myCache[loc].data;
				update(loc, myCache, type);
			}
		}
		//if we didn't get a hit, we do nothing, since hit is already false
	}
	//the remaining possible type is SA
	//since its 4 way we get the index, and search through all of the indecies that share the same value
	//we search for the tags with the same index
	else if (type == SA)
	{
		//the counter tells us where exactly in the FA the tag is located
		hit = false;
		int loc;

		//iterate through all sets in the FA
		//if we get matching tags, update the counter to that location, and it's a hit
		//if we don't, it's a miss
		for (int i = 0; i < CACHE_SETS; i++)
		{
			if (myCache[i].tag == tag && myCache[i].sa_index == index)
			{
				loc = i;
				hit = true;
			}
		}

		//if we got a hit
		if (hit)
		{
			//sw: update our cache with the data
			if (instruction == 1)
			{
				myCache[loc].data = *cur_data;
			}
			//lw: update our data with cache data, and update lru position
			else if (instruction == 0)
			{
				*cur_data = myCache[loc].data;
				update(loc, myCache, type);
			}
		}
		//if we didn't get a hit, we do nothing, since hit is already false
	}
	return hit;
}

//implementation of store function
//if cur_memW = 1, then we are working with a store
//we call the search function, and depending on the hit return value
//we update the data store with the new value
int store(int* cur_data, int* cur_adr, int* miss, int type, cache_set* myCache, int myMem[])
{
	int status = 1;
	//cout << "storing" << endl;
	int instruction = 1;

	//call the search function
	bool hit = search(cur_data, cur_adr, miss, type, myCache, myMem, instruction);

	//at this point, our cur_data or myCache as been updated, depending on the hit
	//now we update the main memory depending if it was a hit or not
	if (hit)
	{
		//write-through
		//i.e update both cache and memory
		myMem[*cur_adr] = *cur_data;
	}
	else if (!hit)
	{
		//write-no-allocate
		//i.e written directly to memory, not cache
		myMem[*cur_adr] = *cur_data;
	}
	*cur_data = 0;
	return status;
}

//implementation of the load function
//if cur_memR = 1, then we are working with a load
//
int load(int* cur_data, int* cur_adr, int* miss, int type, cache_set* myCache, int myMem[])
{
	int status;
	//cout << "loading" << endl;
	int instruction = 0;
	
	//call the search function
	bool hit = search(cur_data, cur_adr, miss, type, myCache, myMem, instruction);

	//if there was a hit then we assign status = 1
	if (hit)
	{
		status = 1;
	}
	//if there was no hit then we assign status = -3 and cache_miss
	else
	{
		status = -3;
		*miss += 1;
		cache_miss(cur_data, cur_adr, myCache, myMem, type);
	}
	return status;
}
/*
* Implementation of Memory controller
*/
int memory_controller(int cur_MemR, int cur_MemW, int* cur_data, int cur_adr, int status, int* miss, int type, cache_set* myCache, int myMem[])
{
	//cout << "controlling" << endl;
	//if cur_memR = 1, then its load
	//if cur_memW = 1, then its store
	//if status < 0, we just increment by 1
	if (status < 0)
	{
		status += 1;
		return status;
	}
	//if status = 0, we make status = 1 and output data - myMem[address]
	else if (status == 0)
	{
		*cur_data = myMem[cur_adr];
		status = 1;
	}
	else //assume that status == 1
	{
		if (cur_MemR == 1)
		{
			status = load(cur_data, &cur_adr, miss, type, myCache, myMem);
		}
		else if (cur_MemW == 1)
		{
			status = store(cur_data, &cur_adr, miss, type, myCache, myMem);
		}
	}
	return status;
}

int main (int argc, char* argv[]) // the program runs like this: ./program <filename> <mode>
{
	//cout << "starting up" << endl;
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
	//cout<<TraceSize<<endl;

	// defining a fully associative or direct-mapped cache or a 4-way set associative cache
	cache_set myCache[CACHE_SETS]; // 1 set per line. 1B per Block

	int myMem [MEM_SIZE];

	// later on define a 4-way SA cache

	// initializing
	// myCache holds the tag, lru position, and data
	for (int i=0; i<CACHE_SETS; i++)
	{
		myCache[i].tag = -1; // -1 indicates that the tag value is invalid. We don't use a separate VALID bit. 
		myCache[i].lru_position = 0;  // 0 means lowest position
		myCache[i].data = 0;
		myCache[i].sa_index = i % 4; //this indicates what set this is in in a 4-way SA. It won't be used for FA
	}

	/*
	for (int j = 0; j < CACHE_SETS; j++)
	{
		cout << myCache[j].sa_index << endl;
	}
	*/

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
		// YOUR CODE
		////////////
		status = memory_controller(cur_MemR, cur_MemW, &cur_data, cur_adr, status, &miss, type, myCache, myMem); // in your memory controller you need to implement your FSM, LW, SW, and MM.
		clock += 1; 
		//cout << clock << endl;
	}
	// to make sure that the last access is also done
	while (status < 1) 
	{
		// YOUR CODE
		status = memory_controller (cur_MemR, cur_MemW, &cur_data, cur_adr, status, &miss, type, myCache, myMem); // in your memory controller you need to implement your FSM, LW, SW, and MM. 
		////////////
		clock += 1;
	}
	float miss_rate = miss / (float)accessL; 
	// printing the final result
	
	cout << "(" << clock << ", " << miss_rate << ")" << endl;

	// closing the file
	fin.close();

	//print out the entire cache
	
	/*
	for (int i = 0; i < 16; i++)
	{
		cout << i << " " << myCache[i].tag << " " << myCache[i].data << " " << myCache[i].lru_position << " " << myCache[i].sa_index << endl;
	}
	*/

	/*
	//print out the entire main memory
	for (int j = 0; j < 24; j++)
	{
		cout << j << " " << myMem[j] << endl;
	}
	*/

	//cout << "ending code" << endl;
	return 0;
}
