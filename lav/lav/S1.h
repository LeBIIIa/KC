
#ifndef S1_H
#define S1_H

// X = 17 - ����� �������
// Y = S + D = 83 + 68 = 151 - ���� ASCII ����� ����� �� � ���

const int X = 17;
const int Y = 151;

SC_MODULE(S1) {
    sc_out<long> o1, o2;  //output 1
    sc_in<bool>    clk;  //clock

    void mainFunc();       //method implementing functionality

    //Counstructor
    SC_CTOR(S1) {
        SC_METHOD(mainFunc);   //Declare addsub as SC_METHOD and
        sensitive_pos << clk;  //make it sensitive to positive clock edge
    }
};

#endif