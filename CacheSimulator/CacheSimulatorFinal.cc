#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <stdio.h>
#include <sstream>
#include <bitset>
#include <typeinfo>
#include <math.h>
#include <climits>
#include <vector>
#include <map>
#include <iomanip>

using namespace std;

ifstream trace_file_read;

int blocksize, timee = 0, L1_size, L1_associativity, L2_size, L2_associativity, L1_to_memo_inclusive = 0;
int L1_read_hit = 0, L1_read_miss = 0, L1_write_miss = 0, L1_write_hit = 0, L1_reads = 0, L1_writes = 0, L1_writeback = 0;
int L2_read_hit = 0, L2_read_miss = 0, L2_write_miss = 0, L2_write_hit = 0, L2_reads = 0, L2_writes = 0, L2_writeback = 0;

int L1_LRU_count_track[33000], L2_LRU_count_track[33000];
double hex_L1tag_max_len = 0, hex_L2tag_max_len = 0;
string trace_file, replacement_policy, inclusion_policy;

vector <string> rw_queue;
vector <string> address_queue;
map <unsigned int, vector<unsigned int> > optimal_address_track;


typedef struct Cache
{
    int tagbit;
    int LRU_count;
    int used_flag;
    int dirty_bit;
}Cache;


void initialize(int L1_sets, int L2_sets, vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree);
void L1Read(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree, string address, int exclusive);
void L1Write(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree, string address);
void L2Read(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, string victim, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree);
void L2Write(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, string victim, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree);
int replace_policy(vector<vector<Cache>>&L, int index, int assoc, int L_num_indexbits, int L_num_offsetbits, vector<vector<int>>&L_pseudo_LRU_binary_tree);
void access_pseudo_LRU(int assoc, int index, int L_associativity, vector<vector<int>>&L_pseudo_LRU_binary_tree);
int find_replacement_pseudo_LRU(int index, vector<vector<int>>&L_pseudo_LRU_binary_tree);
int optimal(vector<vector<Cache>>&L, int index, int L_associativity, int L_num_indexbits, int L_num_offsetbits);
void Print_output(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_sets, int L2_sets);


string deci_to_hexa_conversion(unsigned int deci){
    stringstream dec_string;
    dec_string << hex << deci;
    return dec_string.str();
}

int hex_to_deci_conversion(string address){
    int dec_address;
    istringstream buffer(address);
    buffer >> hex >> dec_address;
    return dec_address;
}

string deci_to_binary(int dec_address){
    bitset<32> bin_address(dec_address);
    string binary_address = bin_address.to_string();
    return binary_address;
}

int tag_index_to_address_conversion(int tag, int index, int L_num_indexbits, int L_num_offsetbits){
    int address = tag << L_num_indexbits;
    address = address | index;
    address = address << L_num_offsetbits;
    return address;
}





int main(int argc , char *argv[]) {

    blocksize = atoi(argv[1]);
    L1_size = atoi(argv[2]);
    L1_associativity = atoi(argv[3]);
    L2_size = atoi(argv[4]);
    L2_associativity = atoi(argv[5]);

    if(atoi(argv[6]) == 0) replacement_policy = "LRU";
    else if(atoi(argv[6]) == 1) replacement_policy = "Pseudo-LRU";
    else if(atoi(argv[6]) == 2) replacement_policy = "Optimal";
    else cout<< "Replacement Policy Parameter not Correct!" << endl;

    if(atoi(argv[7]) == 0) inclusion_policy = "non-inclusive";
    else if(atoi(argv[7]) == 1) inclusion_policy = "inclusive";
    else cout<< "Inclusion Policy Parameter not Correct!" << endl;

    trace_file = argv[8];


    vector < vector < Cache >> L1;
    vector < vector < Cache >> L2;

    // *********************************** Cache 1 ***********************************//
    int L1_sets = L1_size/(blocksize*L1_associativity), L2_sets;


    // ***** Calculating #sets, tag bits, offset bits, index bits of L1
    int L1_num_indexbits = log2(L1_sets);
    int L1_num_offsetbits = log2(blocksize);

    int dividend = (32 - L1_num_indexbits - L1_num_offsetbits)%4;
    int tag_bit_length = 32 - L1_num_indexbits - L1_num_offsetbits;
    if(dividend > 0) hex_L1tag_max_len = (tag_bit_length + 4 - dividend)/4;
    else hex_L1tag_max_len = floor(tag_bit_length/4);


    // ********************* Cache 2 **************************//
    int L2_num_indexbits, L2_num_offsetbits;
    if(L2_size != 0){
        // ***** Calculating #sets, offset bits, index bits of L2
        L2_sets = L2_size/(blocksize*L2_associativity);
        L2_num_indexbits = log2(L2_sets);
        L2_num_offsetbits = log2(blocksize);
        dividend = (32 - L2_num_indexbits - L2_num_offsetbits)%4;
        tag_bit_length = 32 - L2_num_indexbits - L2_num_offsetbits;
        if(dividend > 0) hex_L2tag_max_len = (tag_bit_length + 4 - dividend)/4;
        else hex_L2tag_max_len = floor(tag_bit_length/4);
    }
    else L2_sets = 0;


    int address_position = 0;
    vector < vector < int >> L1_pseudo_LRU_binary_tree;
    vector < vector < int >> L2_pseudo_LRU_binary_tree;

    initialize(L1_sets, L2_sets, L1, L2, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree);

    //***** Read all address and store in vector
    string RW, address;
    trace_file_read.open(trace_file);


    while (!trace_file_read.eof()) {
        // ***** Reading mode and address from file
        trace_file_read >> RW >> address;
        if(RW.length() > 1) RW = RW[RW.length()-1];
        rw_queue.push_back(RW);
        address_queue.push_back(address);

        address_position += 1;

        unsigned int dec_address, tag_index_address;
        dec_address = hex_to_deci_conversion(address);
        tag_index_address = dec_address >> L1_num_offsetbits;

        if (optimal_address_track.count(tag_index_address) == 0) {
            optimal_address_track[tag_index_address] = vector <unsigned int> ();
        }
        optimal_address_track[tag_index_address].push_back(address_position);
    }
    trace_file_read.close();


    for(int v = 0; v < rw_queue.size(); v++){

        timee += 1;
        long long int dec_address;

        RW = rw_queue[v];
        address = address_queue[v];

        int address_len = address.length();
        if(address_len<8){
            for(int bit_add = 0; bit_add < 8-address_len; bit_add++){
                address = '0'+address;
            }
        }

        //  ***** L1 cache Tag bit generate *****//
        // ***** Converting hex address to decimal
        dec_address = hex_to_deci_conversion(address);

        // ***** Converting dec address to binary string
        string binary_address = deci_to_binary(dec_address);

        if(RW == "r") L1Read(L1, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree, address, 0);
        else if(RW == "w") L1Write(L1, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree, address);
        else cout<<"Instruction mode not correct."<<endl;
    }
    Print_output(L1, L2, L1_sets, L2_sets);
    return 0;
}


void initialize(int L1_sets, int L2_sets, vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree){

    // ***** Initialize cache 1
    for(int i = 0; i < L1_sets; i++){
        vector <Cache> v;
        L1.push_back(v);
        Cache b;
        b.tagbit = 0;
        b.LRU_count = 0;
        b.used_flag = 0;
        b.dirty_bit = 0;
        for(int j = 0; j < L1_associativity; j++){
           L1[i].push_back(b);
        }
    }

    // ***** Initialize cache 2
    for(int i = 0; i < L2_sets; i++){
        vector <Cache> v;
        L2.push_back(v);
        Cache b;
        b.tagbit = 0;
        b.LRU_count = 0;
        b.used_flag = 0;
        b.dirty_bit = 0;
        for(int j = 0; j < L2_associativity; j++){
           L2[i].push_back(b);
        }
    }

    // ***** Initialize binary tree for pseudo-LRU of L1
    for(int i = 0; i < L1_sets; i++){
        vector <int> v;
        L1_pseudo_LRU_binary_tree.push_back(v);
        for(int j = 0; j < L1_associativity; j++){
           L1_pseudo_LRU_binary_tree[i].push_back(0);
        }
    }


    // ***** Initialize binary tree for pseudo-LRU of L2
    for(int i = 0; i < L2_sets; i++){
        vector <int> v;
        L2_pseudo_LRU_binary_tree.push_back(v);
        for(int j = 0; j < L2_associativity; j++){
           L2_pseudo_LRU_binary_tree[i].push_back(0);
        }
    }

    // ***** Initialize LRU count for each set of L1
    for(int i = 0; i < L1_sets; i++){
        L1_LRU_count_track[i] = 0;
    }

    // ***** Initialize LRU count for each set of L2
    for(int i = 0; i < L2_sets; i++){
        L2_LRU_count_track[i] = 0;
    }
}


void L1Read(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree, string address, int exclusive){


    string tag_bits, index_bits, block_offset_bits;
    unsigned long long tag, index;

    // ***** Converting hexa address to deci
    long long int dec_address = hex_to_deci_conversion(address);

    // ***** Converting dec address to binary string
    string binary_address = deci_to_binary(dec_address);

    // ***** Storing tab bits, index bits, offset bits for L1 cache
    int L1_num_tagbits = 32 - L1_num_indexbits - L1_num_offsetbits;
    for(int b = 0; b<L1_num_tagbits; b++) tag_bits = tag_bits + binary_address[b];
    for(int b = L1_num_tagbits; b<L1_num_tagbits+L1_num_indexbits; b++)index_bits = index_bits + binary_address[b];
    tag = bitset<32>(tag_bits).to_ullong();
    index = bitset<32>(index_bits).to_ullong();


    if(exclusive == 1){
        for(int slot = 0; slot < L1_associativity ; slot++){

            if(L1[index][slot].tagbit == tag){  // ***** Cache Hit

                L1[index][slot].used_flag = 0;
                if(L1[index][slot].dirty_bit == 1) {
                    L1_to_memo_inclusive += 1;
                    L1[index][slot].dirty_bit = 0;
                }
                break;
            }
        }
    }
    else{

    int victim_tag;
    string victim_dirty, victim_string;

    L1_reads = L1_reads + 1;

    // ***** Checking Hit or miss in L1 cache
    int slot_to_replace, L1_hit_flag = 0, slot_empty_flag = 0;

    for(int slot = 0; slot < L1_associativity ; slot++){

        if(L1[index][slot].tagbit == tag){  // ***** Cache Hit

            L1_read_hit += 1;
            L1[index][slot].LRU_count = L1_LRU_count_track[index];
            L1_LRU_count_track[index] += 1;
            if(replacement_policy == "Pseudo-LRU") access_pseudo_LRU(slot, index, L1_associativity, L1_pseudo_LRU_binary_tree);
            L1_hit_flag = 1;
            break;

        }

        if(L1[index][slot].used_flag == 0 && slot_empty_flag == 0){ //***** Cache Miss without requirement of replace
            slot_to_replace = slot;
            slot_empty_flag = 1;
            L1[index][slot_to_replace].used_flag = 1;
        }
    }


    if(L1_hit_flag == 0){  //If L1 Cache Miss Occurs, place tag value to L1

        if(slot_empty_flag == 0){   //***** Cache Miss with replace
            slot_to_replace = replace_policy(L1, index, L1_associativity, L1_num_indexbits, L1_num_offsetbits,  L1_pseudo_LRU_binary_tree);

            victim_tag = L1[index][slot_to_replace].tagbit;
            int victim = victim_tag << L1_num_indexbits;
            victim = victim | index;
            victim = victim << L1_num_offsetbits;

            stringstream temp;
            temp<<deci_to_hexa_conversion(victim);
            victim_string = temp.str();
        }

        L1_read_miss += 1;
        if(L1[index][slot_to_replace].dirty_bit == 1){
            if(L2_size !=0) L2Write(L1, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, victim_string, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree); //Write back
            L1_writeback += 1;
            L1[index][slot_to_replace].dirty_bit = 0;
        }
        L1[index][slot_to_replace].tagbit = tag;
        L1[index][slot_to_replace].LRU_count = L1_LRU_count_track[index];
        L1_LRU_count_track[index] += 1;

        if(L2_size != 0){
            if(L2_size !=0) L2Read(L2, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, victim_string, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree);
        }
    }

    }
}


void L1Write(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree, string address){

    string tag_bits, index_bits, block_offset_bits;
    unsigned long long tag, index;

    // ***** Converting hexa address to deci
    long long int dec_address = hex_to_deci_conversion(address);

    // ***** Converting dec address to binary string
    string binary_address = deci_to_binary(dec_address);

    // ***** Storing tab bits, index bits, offset bits for L1 cache
    int L1_num_tagbits = 32 - L1_num_indexbits - L1_num_offsetbits;
    for(int b = 0; b<L1_num_tagbits; b++) tag_bits = tag_bits + binary_address[b];
    for(int b = L1_num_tagbits; b<L1_num_tagbits+L1_num_indexbits; b++)index_bits = index_bits + binary_address[b];
    tag = std::bitset<32>(tag_bits).to_ullong();
    index = std::bitset<32>(index_bits).to_ullong();

    int victim_tag;
    string victim_dirty, victim_string;


    L1_writes = L1_writes + 1;

    // ***** Checking Hit or miss in L1 cache
    int slot_to_replace, L1_hit_flag = 0, slot_empty_flag = 0;

    for(int slot = 0; slot < L1_associativity ; slot++){

        if(L1[index][slot].tagbit == tag){  // ***** Cache Hit

            L1_write_hit += 1;
            L1[index][slot].LRU_count = L1_LRU_count_track[index];
            L1_LRU_count_track[index] += 1;
            if(replacement_policy == "Pseudo-LRU") access_pseudo_LRU(slot, index, L1_associativity, L1_pseudo_LRU_binary_tree);
            L1_hit_flag = 1;
            L1[index][slot].dirty_bit = 1;
            break;
        }

        if(L1[index][slot].used_flag == 0 && slot_empty_flag == 0){ //***** Cache Miss without replace policy
            slot_to_replace = slot;
            L1[index][slot_to_replace].used_flag = 1;
            slot_empty_flag = 1;
        }
    }

    if(L1_hit_flag == 0){  //If L1 Cache Miss Occurs, place tag value to L1
        L1_write_miss += 1;

        if(slot_empty_flag == 0){   //***** Cache Miss with replace
            slot_to_replace = replace_policy(L1, index, L1_associativity, L1_num_indexbits, L1_num_offsetbits, L1_pseudo_LRU_binary_tree);

            victim_tag = L1[index][slot_to_replace].tagbit;
            int victim = victim_tag << L1_num_indexbits;
            victim = victim | index;
            victim = victim << L1_num_offsetbits;

            stringstream temp;
            temp<<deci_to_hexa_conversion(victim);
            victim_string = temp.str();
        }

        if(L1[index][slot_to_replace].dirty_bit == 1){
            if(L2_size !=0)  L2Write(L1, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, victim_string, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree);  // Writeback
            L1_writeback += 1;
        }

        L1[index][slot_to_replace].tagbit = tag;
        L1[index][slot_to_replace].LRU_count = L1_LRU_count_track[index];
        L1_LRU_count_track[index] += 1;
        L1[index][slot_to_replace].dirty_bit = 1;

        if(L2_size != 0){
            L2Read(L2, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, victim_string, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree);
        }
    }

}


void L2Read(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, string victim, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree){

    L2_reads = L2_reads + 1;

    long long int dec_address;
    int L2_num_tagbits = 32 - (L2_num_indexbits + L2_num_offsetbits);
    unsigned long long tag, index;
    string tag_bits, index_bits, block_offset_bits, address = address_queue[timee - 1];

    int vic = 0, victim_tag;
    string victim_dirty, victim_string, L2_victim;

    //***** L2 cache Tag bit generate *****//
    // ***** Converting hex address to decimal
    dec_address = hex_to_deci_conversion(address);

    // ***** Converting hex address to binary
    string binary_address = deci_to_binary(dec_address);

    // ***** Storing tab bits, index bits, offset bits for L1 cache
    for(int b = 0; b<L2_num_tagbits; b++) tag_bits = tag_bits + binary_address[b];
    for(int b = L2_num_tagbits; b<L2_num_tagbits+L2_num_indexbits; b++) index_bits = index_bits + binary_address[b];
    tag = std::bitset<32>(tag_bits).to_ullong();
    index = std::bitset<32>(index_bits).to_ullong();

    // ***** Checking Hit or miss in L1 cache
    int slot_to_replace, L2_hit_flag = 0, slot_empty_flag = 0;
    for(int slot = 0; slot < L2_associativity ; slot++){

        if(L2[index][slot].tagbit == tag){  // ***** Cache Hit

            L2_read_hit += 1;
            L2[index][slot].LRU_count = L2_LRU_count_track[index];
            L2_LRU_count_track[index] += 1;
            if(replacement_policy == "Pseudo-LRU") access_pseudo_LRU(slot, index, L2_associativity, L2_pseudo_LRU_binary_tree);
            L2_hit_flag = 1;
            break;
        }

        if(L2[index][slot].used_flag == 0 && slot_empty_flag == 0){ //***** Cache Miss without requirement of replace
            slot_to_replace = slot;
            L2[index][slot_to_replace].used_flag = 1;
            slot_empty_flag = 1;
        }
    }


    if(L2_hit_flag == 0){  //If L2 Cache Miss Occurs, place tag value to L2

        L2_read_miss += 1;

        if(slot_empty_flag == 0){   //***** Cache Miss with replace
        slot_to_replace = replace_policy(L2, index, L2_associativity, L2_num_indexbits, L2_num_offsetbits,  L2_pseudo_LRU_binary_tree);

        victim_tag = L2[index][slot_to_replace].tagbit;
        int victim = victim_tag << L2_num_indexbits;
        victim = victim | index;
        victim = victim << L2_num_offsetbits;
        L2_victim = deci_to_hexa_conversion(victim);
        }

        if(L2[index][slot_to_replace].dirty_bit == 1){
            L2_writeback += 1;
            L2[index][slot_to_replace].dirty_bit = 0;
        }
        L2[index][slot_to_replace].tagbit = tag;
        L2[index][slot_to_replace].LRU_count = L2_LRU_count_track[index];
        L2_LRU_count_track[index] += 1;

        if(slot_empty_flag == 0 && inclusion_policy == "inclusive"){   //***** Cache Miss with replace
            L1Read(L1, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree, L2_victim, 1);
        }
    }
}


void L2Write(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_num_indexbits, int L1_num_offsetbits, int L2_num_indexbits, int L2_num_offsetbits, string victim, vector<vector<int>>&L1_pseudo_LRU_binary_tree, vector<vector<int>>&L2_pseudo_LRU_binary_tree){

    long long int dec_address;
    int L2_num_tagbits = 32 - (L2_num_indexbits + L2_num_offsetbits);
    unsigned long long tag, index;
    string tag_bits, index_bits, block_offset_bits, address, L2_victim;

    int victim_tag;
    string victim_dirty, victim_string;

    L2_writes = L2_writes + 1;
    address = victim;

    //***** L2 cache Tag bit generate *****//
    // ***** Converting hex address to decimal
    dec_address = hex_to_deci_conversion(address);

    // ***** Converting hex address to binary
    string binary_address = deci_to_binary(dec_address);

    // ***** Storing tab bits, index bits, offset bits for L1 cache
    for(int b = 0; b<L2_num_tagbits; b++) tag_bits = tag_bits + binary_address[b];
    for(int b = L2_num_tagbits; b<L2_num_tagbits+L2_num_indexbits; b++) index_bits = index_bits + binary_address[b];
    tag = std::bitset<32>(tag_bits).to_ullong();
    index = std::bitset<32>(index_bits).to_ullong();

    // ***** Checking Hit or miss in L2 cache
    int slot_to_replace, L2_hit_flag = 0, slot_empty_flag = 0;

    for(int slot = 0; slot < L2_associativity ; slot++){

        if(L2[index][slot].tagbit == tag){  // ***** Cache Hit

            L2_write_hit += 1;
            L2[index][slot].LRU_count = L2_LRU_count_track[index];
            L2_LRU_count_track[index] += 1;
            if(replacement_policy == "Pseudo-LRU") access_pseudo_LRU(slot, index, L2_associativity, L2_pseudo_LRU_binary_tree);
            L2_hit_flag = 1;
            L2[index][slot].dirty_bit = 1;
            break;
        }

        if(L2[index][slot].used_flag == 0 && slot_empty_flag == 0){ //***** Cache Miss without replace policy
            slot_to_replace = slot;
            L2[index][slot_to_replace].used_flag = 1;
            slot_empty_flag = 1;
        }
    }


   if(L2_hit_flag == 0){  //If L2 Cache Miss Occurs, place tag value to L2
        L2_write_miss += 1;

        if(slot_empty_flag == 0){   //***** Cache Miss with replace
            slot_to_replace = replace_policy(L2, index, L2_associativity, L2_num_indexbits, L2_num_offsetbits, L2_pseudo_LRU_binary_tree);

            victim_tag = L2[index][slot_to_replace].tagbit;
            int victim = victim_tag << L2_num_indexbits;
            victim = victim | index;
            victim = victim << L2_num_offsetbits;
            L2_victim = deci_to_hexa_conversion(victim);
        }

        if(L2[index][slot_to_replace].dirty_bit == 1){
            L2_writeback += 1;
        }

        L2[index][slot_to_replace].tagbit = tag;
        L2[index][slot_to_replace].LRU_count = L2_LRU_count_track[index];
        L2_LRU_count_track[index] += 1;
        L2[index][slot_to_replace].dirty_bit = 1;

        //***** If inclusive *****
        if(slot_empty_flag == 0 && inclusion_policy == "inclusive"){   //***** Cache Miss with replace
            L1Read(L1, L2, L1_num_indexbits, L1_num_offsetbits, L2_num_indexbits, L2_num_offsetbits, L1_pseudo_LRU_binary_tree, L2_pseudo_LRU_binary_tree, L2_victim, 1);
        }
    }
}


int replace_policy(vector<vector<Cache>>&L, int index, int assoc, int L_num_indexbits, int L_num_offsetbits, vector<vector<int>>&L_pseudo_LRU_binary_tree){

    int slot_to_replace;

    if(replacement_policy == "LRU"){
        int min_count = INT_MAX, min_index;
        for(int i=0; i<assoc; i++){
            if(L[index][i].LRU_count < min_count){
                min_count = L[index][i].LRU_count;
                min_index = i;
            }
        }
        slot_to_replace = min_index;
    }
    else if(replacement_policy == "Pseudo-LRU"){
        slot_to_replace = find_replacement_pseudo_LRU(index, L_pseudo_LRU_binary_tree);
    }
    else if(replacement_policy == "Optimal"){
        slot_to_replace = optimal(L, index, assoc, L_num_indexbits, L_num_offsetbits);
    }
    return slot_to_replace;
}


void access_pseudo_LRU(int assoc, int index, int L_associativity, vector<vector<int>>&L_pseudo_LRU_binary_tree){

    assoc += 1;
    int current_node = 1;
    int left_boundary = 1, right_boundary = L_associativity, middle;

    while(left_boundary < right_boundary){

        middle = floor((left_boundary+right_boundary)/2);

        if(assoc <= middle){
           // goes to left, set current_node to 0
           L_pseudo_LRU_binary_tree[index][current_node - 1] = 1;
           current_node = current_node * 2;
           right_boundary = middle;
        }
        else{
           // goes to right, set current_node to 1
           L_pseudo_LRU_binary_tree[index][current_node - 1] = 0;
           current_node = current_node * 2 + 1;
           left_boundary = middle + 1;
        }
    }
}


int find_replacement_pseudo_LRU(int index, vector<vector<int>>&L_pseudo_LRU_binary_tree){

    int current_node = 1;
    int left_boundary = 1, right_boundary = L1_associativity, middle;

    while(left_boundary < right_boundary){

        //leaf_no = leaf_no / 2;
        middle = floor((left_boundary+right_boundary)/2);

        if(L_pseudo_LRU_binary_tree[index][current_node - 1] == 0){
           // go to left
           L_pseudo_LRU_binary_tree[index][current_node - 1] = 1;
           current_node = current_node * 2;
           right_boundary = middle;
        }
        else{
           // goes to right, set current_node to 1
           L_pseudo_LRU_binary_tree[index][current_node - 1] = 0;
           current_node = current_node * 2 + 1;
           left_boundary = middle + 1;
        }
    }

    return left_boundary - 1;
}


int optimal(vector<vector<Cache>>&L, int index, int L_associativity, int L_num_indexbits, int L_num_offsetbits){

    unsigned int current_slot, future_access = 0, slot_to_replace;
    for(int i = 0; i < L_associativity; i++){
       current_slot = L[index][i].tagbit;
       current_slot = current_slot << L_num_indexbits;
       current_slot = current_slot | index;
       unsigned int current_slot_access = 0;

       for(int j = 0; j < optimal_address_track[current_slot].size(); j++){
            int temp_access = optimal_address_track[current_slot][j];
            if(temp_access >= timee) {
                current_slot_access = temp_access;
                break;
            }
       }

       if(current_slot_access == 0) current_slot_access = INT_MAX;

       if(future_access < current_slot_access){
            future_access = current_slot_access;
            slot_to_replace = i;
       }
    }
   return slot_to_replace;
}


void Print_output(vector<vector<Cache>>&L1, vector<vector<Cache>>&L2, int L1_sets, int L2_sets){

    std::cout << std::setprecision(6) << std::fixed;
    double L1_miss_rate = double(L1_read_miss + L1_write_miss) / (L1_reads + L1_writes);
    double L2_miss_rate;
    if(L2_size != 0) L2_miss_rate = double(L2_read_miss) / (L2_reads);


    cout << "===== Simulator configuration =====" << endl;
    cout << "BLOCKSIZE:             " << blocksize << endl;
    cout << "L1_SIZE:               " << L1_size << endl;
    cout << "L1_ASSOC:              " << L1_associativity << endl;
    cout << "L2_SIZE:               " << L2_size << endl;
    cout << "L2_ASSOC:              " << L2_associativity << endl;
    cout << "REPLACEMENT POLICY:    " << replacement_policy << endl;
    cout << "INCLUSION PROPERTY:    " << inclusion_policy << endl;
    cout << "trace_file:            " << trace_file << endl;


    cout<< "===== L1 contents =====" << endl;

    //***** Print Status of L1
    for(int i= 0; i<L1_sets; i++){
        cout<<"Set\t"<<i<<":\t";
        for(int j=0; j<L1_associativity; j++){
            string Dirty_bit = " ";
            string hex_st = deci_to_hexa_conversion(L1[i][j].tagbit);
            string space;
            for(int sp = 0; sp < hex_L1tag_max_len-hex_st.length(); sp++) space = space + " ";
            if(L1[i][j].dirty_bit == 1) Dirty_bit = "D";
            cout<<hex_st<<space<<" "<<Dirty_bit<<" ";
        }
        cout<<endl;
    }

    if(L2_size != 0){

        cout<< "===== L2 contents =====" << endl;
        //***** Print Status of L1
        for(int i= 0; i<L2_sets; i++){
            cout<<"Set\t"<<i<<":\t";
            for(int j=0; j<L2_associativity; j++){
                string Dirty_bit = " ";
                string hex_st = deci_to_hexa_conversion(L2[i][j].tagbit);
                string space;
                for(int sp = 0; sp < hex_L2tag_max_len-hex_st.length(); sp++) space = space + " ";
                if(L2[i][j].dirty_bit == 1) Dirty_bit = "D";
                cout<<hex_st<<space<<" "<<Dirty_bit<<" ";
            }
            cout<<endl;
        }
    }

    cout<< "===== Simulation results (raw) =====" << endl;
    cout<<"a. number of L1 reads:        " << L1_reads<< endl;
    cout<<"b. number of L1 read misses:  " << L1_read_miss<< endl;
    cout<<"c. number of L1 writes:       " << L1_writes<< endl;
    cout<<"d. number of L1 write misses: " << L1_write_miss<< endl;

    cout<<"e. L1 miss rate:              " << setprecision(6) << L1_miss_rate << endl;
    cout<<"f. number of L1 writebacks:   " << L1_writeback << endl;
    cout<<"g. number of L2 reads:        " << L2_reads<< endl;
    cout<<"h. number of L2 read misses:  " << L2_read_miss<< endl;
    cout<<"i. number of L2 writes:       " << L2_writes<< endl;
    cout<<"j. number of L2 write misses: " << L2_write_miss<< endl;

    if(L2_size != 0) cout<<"k. L2 miss rate:              " << setprecision(6) << L2_miss_rate << endl;
    else cout<<"k. L2 miss rate:              " << "0" << endl;
    cout<<"l. number of L2 writebacks:   " << L2_writeback << endl;


    if(L2_size != 0 && inclusion_policy == "inclusive") cout<<"m. total memory traffic:      " << L2_read_miss + L2_write_miss + L2_writeback + L1_to_memo_inclusive << endl;
    else if(L2_size != 0 && inclusion_policy == "non-inclusive") cout<<"m. total memory traffic:      " << L2_read_miss + L2_write_miss + L2_writeback << endl;
    else cout<<"m. total memory traffic:      " << L1_read_miss + L1_write_miss + L1_writeback << endl;
}


