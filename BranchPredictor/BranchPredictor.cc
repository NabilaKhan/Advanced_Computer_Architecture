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
string trace_file, Predictor_mode;

int Total_Predictions = 0, Wrong_Predictions = 0;
unsigned int Smith_Counter = 0;
unsigned int b, m, n, k, m_biomodal;
unsigned int Global_Branch_History_Register;

map < unsigned int, unsigned int > Bimodal_Table;
map < unsigned int, unsigned int > Gshare_Table;
map < unsigned int, unsigned int > Chooser_Table;


class Branch_Predictor{
    public:

        unsigned int Middle_Boundary, High_Saturation, Low_Saturation;

        Branch_Predictor(){}

        // Initialization for Smith N-bit Predictor
        Branch_Predictor(unsigned int number_of_bits){

            b = number_of_bits;
            Middle_Boundary = pow(2,b)/2;
            Low_Saturation = 0;
            High_Saturation = pow(2,b)-1;
            Smith_Counter = Middle_Boundary;
        }

        // Initialization for Gshare Predictor
        Branch_Predictor(unsigned int index_bits, unsigned int history_bits){
            m = index_bits;
            n = history_bits;

            Gshare_Table = map <unsigned int, unsigned int> ();
            int Gshare_table_height = pow(2, m);
            for(int i=0; i<Gshare_table_height; i++)
                Gshare_Table[i] = 4;

            Global_Branch_History_Register = 0;
        }

        // Initialization for Hybrid Predictor
        Branch_Predictor(unsigned int chooser_index_bits, unsigned int gshare_index_bits, unsigned int history_bits, unsigned int bimodal_index_bits){
            k = chooser_index_bits;
            m = gshare_index_bits;
            n = history_bits;
            m_biomodal = bimodal_index_bits;

            Gshare_Table = map < unsigned int, unsigned int > ();
            int Gshare_table_height = pow(2, m);
            for(int i=0; i < Gshare_table_height; i++)
                Gshare_Table[i] = 4;

            Bimodal_Table = map < unsigned int, unsigned int > ();
            int Bimodal_table_height = pow(2, m_biomodal);
            for(int i=0; i < Bimodal_table_height; i++)
                Bimodal_Table[i] = 4;

            Chooser_Table = map < unsigned int, unsigned int > ();
            int Chooser_table_height = pow(2, k);
            for(int i=0; i < Chooser_table_height; i++)
                Chooser_Table[i] = 1;

            Global_Branch_History_Register = 0;
        }
};


char Predict(Branch_Predictor P, unsigned int index);
void Update_Predictor(Branch_Predictor P, char actual_outcome, unsigned int index);
char Predict_Hybrid(Branch_Predictor P, unsigned int bimodal_index, unsigned int gshare_index, unsigned int chooser_index, char actual_outcome);
void Update_Global_History_Register(char actual_outcome);
void Print();


int hex_to_deci_conversion(string address){
    int dec_address;
    istringstream buffer(address);
    buffer >> hex >> dec_address;
    return dec_address;
}




int main(int argc , char *argv[]) {

    Predictor_mode = argv[1];
    Branch_Predictor P = Branch_Predictor();
    if(Predictor_mode == "smith") {
        trace_file = argv[3];
        P = Branch_Predictor(atoi(argv[2]));
    }
    else if(Predictor_mode == "bimodal"){
        trace_file = argv[3];
        P = Branch_Predictor(atoi(argv[2]), 0);
    }
    else if(Predictor_mode == "gshare"){
        trace_file = argv[4];
        P = Branch_Predictor(atoi(argv[2]), atoi(argv[3]));
    }
    else if(Predictor_mode == "hybrid"){
        trace_file = argv[6];
        P = Branch_Predictor(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    }


    string hexa_address;
    char actual_outcome, predicted_outcome;
    trace_file_read.open(trace_file);

    while (!trace_file_read.eof() && trace_file_read >> hexa_address >> actual_outcome) {

        unsigned int deci_address, index = 0, bimodal_index = 0, gshare_index = 0, chooser_index = 0;
        deci_address = hex_to_deci_conversion(hexa_address);

        index = deci_address << (32 - m - 2) >> (32 - m);   // *** bimodal index ***//
        gshare_index = index ^ Global_Branch_History_Register;

        if(Predictor_mode == "smith" || Predictor_mode == "bimodal" || Predictor_mode == "gshare"){

            if(Predictor_mode == "gshare") index = index ^ Global_Branch_History_Register;

            predicted_outcome = Predict(P, index);
            Update_Predictor(P, actual_outcome, index);

            if(Predictor_mode == "gshare") Update_Global_History_Register(actual_outcome);
        }
        else if(Predictor_mode == "hybrid"){

            bimodal_index = deci_address << (32 - m_biomodal - 2) >> (32 - m_biomodal);
            chooser_index = deci_address << (32 - k - 2) >> (32 - k); // *** chooser index ***//

            predicted_outcome = Predict_Hybrid(P, bimodal_index, gshare_index, chooser_index, actual_outcome);
        }
        else{
            cout<< "Unknown Branch Predictor! Select from the following options:" <<endl;
            cout<< "'smith' or 'bimodal' or 'gshare' or 'hybrid'" << endl;
        }

        Total_Predictions += 1;
        if(predicted_outcome != actual_outcome) Wrong_Predictions+= 1;
    }

    trace_file_read.close();
    Print();
    return 0;
}

char Predict(Branch_Predictor P, unsigned int index){
    char predicted_outcome;

    // *** Smith N-bit Predictor
    if(Predictor_mode == "smith"){
        // Predicted not taken
        if(Smith_Counter < P.Middle_Boundary) predicted_outcome = 'n';
        // Predicted taken
        else predicted_outcome = 't';
    }

    // *** Bimodal or Gshare Predictor
    else if(Predictor_mode == "bimodal" || Predictor_mode == "gshare"){
        // Predicted not taken
        if(Gshare_Table[index] < 4) predicted_outcome = 'n';
        // Predicted taken
        else predicted_outcome = 't';
    }
    return predicted_outcome;
}

void Update_Predictor(Branch_Predictor P, char actual_outcome, unsigned int index){

    // *** Smith N-bit Predictor
    if(Predictor_mode == "smith"){
        // If actual outcome not taken and counter not in low saturation, decrement counter by 1
        if(actual_outcome == 'n' && Smith_Counter > P.Low_Saturation) Smith_Counter -= 1;
        // If actual outcome taken and counter not in high saturation, increment counter by 1
        else if(actual_outcome == 't' && Smith_Counter < P.High_Saturation) Smith_Counter += 1;
    }
    // *** Bimodal or Gshare Predictor
    else if(Predictor_mode == "bimodal" || Predictor_mode == "gshare"){
        // If actual outcome not taken and counter not in low saturation, decrement counter by 1
        if(actual_outcome == 'n' && Gshare_Table[index] > 0) Gshare_Table[index] -= 1;
        // If actual outcome taken and counter not in high saturation, increment counter by 1
        else if(actual_outcome == 't' && Gshare_Table[index] < 7) Gshare_Table[index] += 1;
    }
}

char Predict_Hybrid(Branch_Predictor P, unsigned int bimodal_index, unsigned int gshare_index, unsigned int chooser_index, char actual_outcome){

    char predicted_outcome;

    // ********************* Predicting Outcome ********************* //
    char bimodal_outcome, gshare_outcome;

    //***** Bimodal Predictor *****//
    if(Bimodal_Table[bimodal_index] < 4) bimodal_outcome = 'n';  // Predicted "Not Taken" for Bimodal
    else bimodal_outcome = 't';  // Predicted "Taken" for Bimodal


    //***** Gshare Predictor *****//
    if(Gshare_Table[gshare_index] < 4) gshare_outcome = 'n';  // Predicted "Not Taken" for Gshare
    else gshare_outcome = 't';  // Predicted "Taken" for Gshare


    //*** Chooser ***//
    if(Chooser_Table[chooser_index] < 2) { // Select Bimodal
        predicted_outcome = bimodal_outcome;
        // ********************* Updating Bimodal Predictor ********************* //
        if(actual_outcome == 'n' && Bimodal_Table[bimodal_index] > 0) Bimodal_Table[bimodal_index] -= 1;    //If actual outcome not taken and counter not in low saturation, decrement counter by 1
        else if(actual_outcome == 't' && Bimodal_Table[bimodal_index] < 7) Bimodal_Table[bimodal_index] += 1; //If actual outcome taken and counter not in high saturation, increment counter by 1
    }
    else{ // Select Gshare
        predicted_outcome = gshare_outcome;
        // ********************* Updating Gshare Predictor ********************* //
        if(actual_outcome == 'n' && Gshare_Table[gshare_index] > 0) Gshare_Table[gshare_index] -= 1; //If actual outcome not taken and counter not in low saturation, decrement counter by 1
        else if(actual_outcome == 't' && Gshare_Table[gshare_index] < 7) Gshare_Table[gshare_index] += 1; //If actual outcome taken and counter not in high saturation, increment counter by 1
    }

    // ********************* Updating Global History Register ********************* //
    Update_Global_History_Register(actual_outcome);

    // ********************* Updating Chooser ********************* //
    if(gshare_outcome == actual_outcome && bimodal_outcome != actual_outcome){
        if(Chooser_Table[chooser_index] < 3) Chooser_Table[chooser_index] += 1;
    }
    else if(gshare_outcome != actual_outcome && bimodal_outcome == actual_outcome){
        if(Chooser_Table[chooser_index] > 0) Chooser_Table[chooser_index] -= 1;
    }

    return predicted_outcome;
}

void Update_Global_History_Register(char actual_outcome){

    Global_Branch_History_Register = Global_Branch_History_Register >> 1;
    if(actual_outcome == 't')Global_Branch_History_Register = Global_Branch_History_Register | (1 << n - 1);
}

void Print(){

    double rate = (Wrong_Predictions/(double)Total_Predictions)*100;

    std::cout << std::setprecision(2) << std::fixed;

    cout << "COMMAND" << endl;

    if(Predictor_mode == "smith") cout<< "./sim " << Predictor_mode << " " << b <<" " << trace_file << endl;
    else if(Predictor_mode == "bimodal") cout<< "./sim " << Predictor_mode << " " << m <<" " << trace_file << endl;
    else if(Predictor_mode == "gshare") cout<< "./sim " << Predictor_mode << " " << m << " " << n <<" " << trace_file << endl;
    else if(Predictor_mode == "hybrid") cout<< "./sim " << Predictor_mode << " " << k << " " << m << " " << n << " " << m_biomodal << " " << trace_file << endl;

    cout << "OUTPUT" << endl;
    cout << "number of predictions:   \t" << Total_Predictions << endl;
    cout << "number of mispredictions:\t" << Wrong_Predictions << endl;
    cout << "misprediction rate:      \t" << rate <<"%" << endl;
    if(Predictor_mode == "smith"){
        cout << "FINAL COUNTER CONTENT:   \t" << Smith_Counter << endl;
    }
    else if(Predictor_mode == "bimodal"){
        cout << "FINAL BIMODAL CONTENTS" << endl;
        int Gshare_table_height = pow(2, m);
        for(int i=0; i<Gshare_table_height; i++)
            cout << i << "\t" << Gshare_Table[i] << endl;
    }
    else if(Predictor_mode == "gshare"){
        cout << "FINAL GSHARE CONTENTS" << endl;
        int Gshare_table_height = pow(2, m);
        for(int i=0; i<Gshare_table_height; i++)
            cout << i << "\t" << Gshare_Table[i] << endl;
    }
    else if(Predictor_mode == "hybrid"){
        cout << "FINAL CHOOSER CONTENTS" << endl;
        int Chooser_table_height = pow(2, k);
        for(int i=0; i<Chooser_table_height; i++)
            cout << i << "\t" << Chooser_Table[i] << endl;

        cout << "FINAL GSHARE CONTENTS" << endl;
        int Gshare_table_height = pow(2, m);
        for(int i=0; i<Gshare_table_height; i++)
            cout << i << "\t" << Gshare_Table[i] << endl;

        cout << "FINAL BIMODAL CONTENTS" << endl;
        int Bimodal_table_height = pow(2, m_biomodal);
        for(int i=0; i<Bimodal_table_height; i++)
            cout << i << "\t" << Bimodal_Table[i] << endl;
    }
}


