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
#include <algorithm>
#include <map>
#include <iomanip>

#define REG_SIZE 128


using namespace std;

ifstream trace_file_read;
ofstream debug_file_write;
string trace_file;


int S, N;
int cycle = 0, instr_count = 0;

struct Trace_file_input{
    string pc;
    int op;
    int des;
    int src1;
    int src2;
};

struct Queue_entry{
    int tag;
    int pos;
};

bool operator<(const Queue_entry& a, const Queue_entry& b)
{
    return a.tag < b.tag;
}

struct Execute_unit{
    int tag;
    int pos;
    int op;
    int time;
};

struct Fake_ROB_entry{

    int TAG;
    int OP;
    int DES[2], SRC1[3], SRC2[3];
    string current_stage;
    int IF[2], ID[2], IS[2], EX[2], WB[2];
};

struct Register{
    int tag;
    int ready_flag;
};

vector <Trace_file_input> tc;
vector <Fake_ROB_entry> ROB;
vector <Queue_entry> DISPATCH_QUEUE;
vector <Queue_entry> ISSUE_QUEUE;
vector <Execute_unit> E;
struct Register R[REG_SIZE];



void initialize_ROB(int temp_tag, int temp_op, int temp_des, int temp_src1, int temp_src2);
int Fetch(int inst);
void Dispatch();
void Issue();
void Execute();
void Fake_Retire();
void increment_cycle();



int main(int argc , char *argv[]) {

    S = atoi(argv[1]), N = atoi(argv[2]), trace_file = argv[3];

    string PC;
    int op, des, src1, src2;
    trace_file_read.open(trace_file);

    //Initialize all register entries to "Ready" state (flag value 1)
    for (int reg = 0; reg < REG_SIZE; reg++){
        R[reg].ready_flag = 1;
        R[reg].tag = -1;
    }

    // Reading the trace file and storing in an vector
    while (!trace_file_read.eof() && trace_file_read >> PC >> op >> des >> src1 >> src2) {

        tc.push_back(Trace_file_input());
        tc[instr_count].pc = PC;
        tc[instr_count].op = op;
        tc[instr_count].des = des;
        tc[instr_count].src1 = src1;
        tc[instr_count].src2 = src2;

        instr_count += 1;
    }

    trace_file_read.close();

    int inst = 0;
    do{
        Fake_Retire();
        Execute();
        Issue();
        Dispatch();
        int parallel_count = Fetch(inst);
        inst += parallel_count;

        // Increment cycle latency for each stage of instructions
        increment_cycle();
        cycle += 1;

    }while(ROB.size() > 0);

    cout << "number of instructions = " << inst << endl;
    cout << "number of cycles       = " << cycle - 1 << endl;
    float IPC = float(inst) / (cycle - 1);
    cout << "IPC                    = " << IPC << endl;

    return 0;
}


void initialize_ROB(int temp_tag, int temp_op, int temp_des, int temp_src1, int temp_src2){

    int cur_len = ROB.size();
    ROB.push_back(Fake_ROB_entry());
    ROB[cur_len].TAG = temp_tag;
    ROB[cur_len].OP = temp_op;
    ROB[cur_len].DES[0] = temp_des;
    ROB[cur_len].SRC1[0] = temp_src1;
    ROB[cur_len].SRC1[2] = -1;
    ROB[cur_len].SRC2[0] = temp_src2;
    ROB[cur_len].SRC2[2] = -1;
    ROB[cur_len].current_stage = "IF";

    ROB[cur_len].IF[0] = cycle;
    ROB[cur_len].IF[1] = 0;
}


int Fetch(int inst){

    int parallel_count = 0;

    while(parallel_count < N){

        int temp_tag = inst + parallel_count;
        if(temp_tag == tc.size() || DISPATCH_QUEUE.size() == 2 * N) break;

        // Push instruction to fake-ROB
        initialize_ROB(temp_tag, tc[temp_tag].op, tc[temp_tag].des, tc[temp_tag].src1, tc[temp_tag].src2);

        // Insert instruction in Dispatch Queue
        int no_instr_Dispatch_queue = DISPATCH_QUEUE.size();
        DISPATCH_QUEUE.push_back(Queue_entry());
        DISPATCH_QUEUE[no_instr_Dispatch_queue].tag = temp_tag;
        DISPATCH_QUEUE[no_instr_Dispatch_queue].pos = ROB.size() - 1;

        parallel_count += 1;
    }
    return parallel_count;
}


void Dispatch(){

    vector <Queue_entry> temp_dispatch;

    for(int i = 0; i < DISPATCH_QUEUE.size(); i++){
        int current_pos = DISPATCH_QUEUE[i].pos;
        string current_state = ROB[current_pos].current_stage;

        if(current_state == "IF") {
                ROB[current_pos].current_stage = "ID";
                ROB[current_pos].ID[0] = cycle;
                ROB[current_pos].ID[1] = 0;
        }
        else if(current_state == "ID"){
            // construct a temp list of dispatch instructions
            int temp_pos = temp_dispatch.size();
            temp_dispatch.push_back(Queue_entry());
            temp_dispatch[temp_pos].tag = DISPATCH_QUEUE[i].tag;
            temp_dispatch[temp_pos].pos = DISPATCH_QUEUE[i].pos;
        }
    }

    // Sort temp dispatch queue according to tag value
    std::sort(temp_dispatch.begin(), temp_dispatch.end());


    // If Scheduling Queue not full
    for(int j = 0; j < temp_dispatch.size() ; j++){

        if(ISSUE_QUEUE.size() == S) break;  //Scheduling queue full
        // Enter instruction into Issue list
        int temp_pos = ISSUE_QUEUE.size();
        ISSUE_QUEUE.push_back(Queue_entry());
        ISSUE_QUEUE[temp_pos].tag = temp_dispatch[j].tag;
        ISSUE_QUEUE[temp_pos].pos = temp_dispatch[j].pos;

        // Change State from ID to IS
        ROB[temp_dispatch[j].pos].current_stage = "IS";
        ROB[temp_dispatch[j].pos].IS[0] = cycle;
        ROB[temp_dispatch[j].pos].IS[1] = 0;

        //Update destination position of instruction in Register to "not Ready", also update status of source1 and source2 in ROB based on their status in register
        int cur_des = ROB[temp_dispatch[j].pos].DES[0];
        int cur_src1 = ROB[temp_dispatch[j].pos].SRC1[0];
        int cur_src2 = ROB[temp_dispatch[j].pos].SRC2[0];

        if(cur_src1 != -1) {
                ROB[temp_dispatch[j].pos].SRC1[1] = R[cur_src1].ready_flag;
                ROB[temp_dispatch[j].pos].SRC1[2] = R[cur_src1].tag;
        }
        if(cur_src2 != -1) {
                ROB[temp_dispatch[j].pos].SRC2[1] = R[cur_src2].ready_flag;
                ROB[temp_dispatch[j].pos].SRC2[2] = R[cur_src2].tag;
        }
        if(cur_des != -1) {
                R[cur_des].ready_flag = 0;
                R[cur_des].tag = ISSUE_QUEUE[temp_pos].tag;
        }

        // Delete instruction from Dispatch list
        for(int k = 0; k < DISPATCH_QUEUE.size(); k ++){
            if(DISPATCH_QUEUE[k].tag == temp_dispatch[j].tag){
                DISPATCH_QUEUE.erase(DISPATCH_QUEUE.begin() + k);
                break;
            }
        }
    }
}


void Issue(){
    vector <Queue_entry> temp_issue;

    for(int i = 0; i < ISSUE_QUEUE.size(); i++){
        int current_pos = ISSUE_QUEUE[i].pos;
        int cur_src1_status = ROB[current_pos].SRC1[1];
        int cur_src2_status = ROB[current_pos].SRC2[1];

        // if all sources ready then add instruction to temp issue list
        if((cur_src1_status == 1 && cur_src2_status == 1) || (cur_src1_status == 1 && ROB[current_pos].SRC2[0] == -1) || (ROB[current_pos].SRC1[0] == -1 && cur_src2_status == 1) || (ROB[current_pos].SRC1[0] == -1 && ROB[current_pos].SRC2[0] == -1)){

            int temp_pos = temp_issue.size();
            temp_issue.push_back(Queue_entry());
            temp_issue[temp_pos].tag = ISSUE_QUEUE[i].tag;
            temp_issue[temp_pos].pos = ISSUE_QUEUE[i].pos;
        }
    }

    // Sort temp issue queue according to tag value
    std::sort(temp_issue.begin(), temp_issue.end());

    int execution_count = 0;
    for(int j = 0; j < temp_issue.size() ; j++){

        if(execution_count == N+1) break; // At a time maximum N+1 instructions can be passed to execution unit

        // Delete instruction from Issue list
        for(int k = 0; k < ISSUE_QUEUE.size(); k ++){


            if(ISSUE_QUEUE[k].tag == temp_issue[j].tag){

                //Add instruction to Execution list
                int ex = E.size();
                E.push_back(Execute_unit());
                E[ex].tag = temp_issue[j].tag;
                E[ex].pos = temp_issue[j].pos;
                E[ex].op = ROB[E[ex].pos].OP;
                E[ex].time = 1;
                ROB[E[ex].pos].current_stage = "EX";
                ROB[E[ex].pos].EX[0] = cycle;
                ROB[E[ex].pos].EX[1] = 0;
                // remove instruction from issue list
                ISSUE_QUEUE.erase(ISSUE_QUEUE.begin() + k);
                break;
            }
        }
        execution_count += 1;
    }
}


void Execute(){

    vector <Execute_unit> Execute_temp;

    for(int ex = 0; ex < E.size(); ex++){

        // If instruction has completed then remove from execution list and send it to WB stage
        if((E[ex].op == 0 && E[ex].time == 1) || (E[ex].op == 1 && E[ex].time == 2) || (E[ex].op == 2 && E[ex].time == 5)){

            int cur_pos = E[ex].pos;
            int cur_des = ROB[cur_pos].DES[0];

            int temp_size = Execute_temp.size();
            Execute_temp.push_back(Execute_unit());
            Execute_temp[temp_size].tag = E[ex].tag;
            Execute_temp[temp_size].pos = E[ex].pos;

            if(cur_des != -1 && R[cur_des].tag == E[ex].tag) {
                    R[cur_des].ready_flag = 1;
                    R[cur_des].tag = -1;
            }
            for(int r = 0; r < ROB.size(); r++){
                if(ROB[r].SRC1[0] == cur_des && ROB[r].SRC1[2] == E[ex].tag) {
                    ROB[r].SRC1[1] = 1;
                }
                if(ROB[r].SRC2[0] == cur_des && ROB[r].SRC2[2] == E[ex].tag) {
                    ROB[r].SRC2[1] = 1;
                }
            }

            ROB[E[ex].pos].current_stage = "WB";
            ROB[E[ex].pos].WB[0] = cycle;
            ROB[E[ex].pos].WB[1] = 1;
        }
        else E[ex].time += 1;
    }

    for(int ex = 0; ex < Execute_temp.size(); ex++){
        for(int k = 0; k < E.size(); k++){
            if(Execute_temp[ex].tag == E[k].tag){
                E.erase(E.begin() + k);
                break;
            }
        }
    }
}


void Fake_Retire(){

    vector <Fake_ROB_entry> temp_ROB;

    // Delete already written instructions from fake ROB
    for(int r = 0; r < ROB.size(); r++){
        if(ROB[r].current_stage != "WB") break;

        int temp_size = temp_ROB.size();
        temp_ROB.push_back(Fake_ROB_entry());
        temp_ROB[temp_size].TAG = ROB[r].TAG;
    }

    int pos_change = temp_ROB.size();

    for(int tr = 0; tr < temp_ROB.size(); tr++){
        for(int k =0; k < ROB.size(); k++){
            if(ROB[k].TAG == temp_ROB[tr].TAG){
                cout << ROB[k].TAG << " fu{" << ROB[k].OP << "} src{" << ROB[k].SRC1[0] << "," << ROB[k].SRC2[0]
                << "} dst{" << ROB[k].DES[0] << "} IF{" << ROB[k].IF[0] << "," << ROB[k].IF[1] << "} ID{" << ROB[k].ID[0] << "," << ROB[k].ID[1] << "} IS{"
                << ROB[k].IS[0] << "," << ROB[k].IS[1] << "} EX{" << ROB[k].EX[0] << "," << ROB[k].EX[1] << "} WB{" << ROB[k].WB[0] << "," << ROB[k].WB[1] << "}" << endl;
                ROB.erase(ROB.begin() + k);
                break;
            }
        }
    }

    // Update position of instructions in each list with the current position of instruction in the fake ROB
    for(int d = 0; d < DISPATCH_QUEUE.size(); d++) DISPATCH_QUEUE[d].pos = DISPATCH_QUEUE[d].pos - pos_change;
    for(int is = 0; is < ISSUE_QUEUE.size(); is++) ISSUE_QUEUE[is].pos = ISSUE_QUEUE[is].pos - pos_change;
    for(int e = 0; e < E.size(); e++) E[e].pos = E[e].pos - pos_change;
}

void increment_cycle(){

    for(int inst = 0; inst < ROB.size(); inst++){
        if(ROB[inst].current_stage == "IF") ROB[inst].IF[1] += 1;
        else if(ROB[inst].current_stage == "ID") ROB[inst].ID[1] += 1;
        else if(ROB[inst].current_stage == "IS") ROB[inst].IS[1] += 1;
        else if(ROB[inst].current_stage == "EX") ROB[inst].EX[1] += 1;
    }
}




