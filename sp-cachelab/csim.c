#include "cachelab.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"

struct cache_line {
    int valid_bit; // cache line is valid or not
    int tag; // tag
    int lru_ctr; // if this is 0 the cache line is most recently referenced. 
                 // Larger lru_ctr means longer time passed after being referenced
};

int main(int argc, char *argv[])
{
    if(argc<9){
        return 0;
    }
    
    /*--------arguments--------*/
    int s = *argv[2] - '0';
    int E = *argv[4] - '0';
    int b = *argv[6] - '0';
    const char* t = argv[8];

    /*-------file manager-------*/
    FILE* tracePtr = fopen(t, "r");

    /*---------cache----------*/

    struct cache_line cache[(int)pow(2, s)][E];
    int hit_count = 0;
    int miss_count = 0;
    int evict_count = 0;

    for(int i=0; i<(int)pow(2,s); i++){
        for(int j=0; j<E; j++){
            cache[i][j].lru_ctr = -1;
            cache[i][j].valid_bit = 0;
        }
    }

    /*------------------------*/

    
    while(1){
        char buffer[255];
        if(feof(tracePtr)!=0)
            break;
        if(fgets(buffer, sizeof(buffer), tracePtr)==NULL)
            break;
        if(buffer[0] == ' '){
            //1. parse the operation, address, size
            char operation = *strtok(buffer, " ");
            int address = (int)strtol(strtok(NULL, ","), NULL, 16);

            //find cache line info
            int block_address = address >> b;
            int set_index = block_address & (0x7fffffff >> (63-s));
            int address_tag = block_address>>s;

            int hit = 0;
            int miss = 0;
            int eviction = 0;
            for(int i=0; i<E; i++){
                if(cache[set_index][i].tag == address_tag){
                    if(cache[set_index][i].valid_bit==1){
                        hit = 1;
                        for(int j=i+1; j<E; j++){
                            cache[set_index][j].lru_ctr++;
                        }
                        cache[set_index][i].lru_ctr =0;
                        break;
                    }
                    else{
                        cache[set_index][i].lru_ctr++;
                    }
                }
            }

            if(!hit){
                miss = 1;
                eviction = 1;
                for(int i=0; i<E; i++){
                    if(cache[set_index][i].valid_bit==0){
                        cache[set_index][i].valid_bit  = 1;
                        cache[set_index][i].tag = address_tag;
                        for(int j=i+1; j<E; j++){
                            cache[set_index][j].lru_ctr++;
                        }
                        cache[set_index][i].lru_ctr = 0;
                        eviction = 0;
                        break;
                    }
                    else{
                        cache[set_index][i].lru_ctr++;
                    }
                }
                if(eviction){
                    int victim = 0;
                    for(int i=1; i<E; i++){
                        if(cache[set_index][i].lru_ctr > cache[set_index][victim].lru_ctr){
                            victim =i;
                        }
                    }                    
                    cache[set_index][victim].valid_bit = 1;
                    cache[set_index][victim].tag = address_tag;
                    cache[set_index][victim].lru_ctr = 0;
                }
            }

            switch (operation)
            {
            case 'L':
                /* code */
                hit_count += hit;
                miss_count += miss;
                evict_count += eviction;
                break;
            
            case 'M':
                /* code */
                if(!hit){
                    hit_count++;
                    miss_count++;
                    evict_count += eviction;
                }
                else{
                    hit_count += 2;
                }
                break;
            
            case 'S':
                /* code */
                hit_count += hit;
                miss_count += miss;
                evict_count += eviction;
                break;
            
            default:
                break;
            }
            
        }
    }



    printSummary(hit_count, miss_count, evict_count);
    return 0;
}
