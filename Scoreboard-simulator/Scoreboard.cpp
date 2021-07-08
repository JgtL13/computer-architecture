#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
using namespace std;

#define LDCLOCK 1  //L.D execution stage cycles
#define ADDCLOCK 2  //ADD.D and SUB.D execution stage cycles
#define MULTCLOCK 10  //MUL.D execution stage cycles
#define DIVCLOCK 40  //DIV.D execution stage cycles

//Scoreboard variables
int InstructionStatus[10][4] = {{0}};
bool Busy[5] = {false, false, false, false, false};
string Op[5] = {};
string Fi[5] = {};
string Fj[5] = {};
string Fk[5] = {};
string Qj[5] = {};
string Qk[5] = {};
bool Rj[5] = {true, true, true, true, true};
bool Rk[5] = {true, true, true, true, true};
string RegisterResultStatus[16][2] = {{"F0", }, {"F2", }, {"F4", }, {"F6", }, {"F8", }, {"F10", }, {"F12", }, {"F14", }, {"F16", }, {"F18", }, {"F20", }, {"F22", }, {"F24", }, {"F26", }, {"F28", }, {"F30", }};

//Other variables for this project
ifstream inFile("input.txt");
stringstream ss;
string op[10];
string Rd[10];
string Rs[10];
string Rt[10];
int countDown[10] = {0};
bool roundComplete[10] = {false};
int cycle = 1;
int linesOfInstruction = 0;

//Functions
string deleteFirstSymbol(string);
string deleteLastSymbol(string);
void instructionPreProccess(string);  //將指輸入的指令拆解並且存入op[], Rd[], Rs[], Rt中
void printInstructionStatus();
void printFunctionUnitStatus();
void issue(int);
void rdOperands(int);
void exeComplete(int);
void wrResult(int);
void printRegResult();

string deleteFirstSymbol(string input)
{
    string output = input.erase(0, 1);
    return output;
}

string deleteLastSymbol(string input)
{
    string output = input.substr(0, input.length() - 1);
    return output;
}

void instructionPreProccess(string line)
{
    int i = 0;
    while(Rd[i] != "")
        ++i;
    string opcode, rd, rs, rt;
    ss << line;
    ss >> opcode;
    if(opcode == "L.D")
    {
        ss >> rd >> rs;
        ss.clear();
        rd = deleteLastSymbol(rd);
        while(isdigit(rs[0]))
        {
            rs = deleteFirstSymbol(rs);
        }
        rs = deleteFirstSymbol(rs);
        rs = deleteLastSymbol(rs);
        op[i] = opcode;
        Rd[i] = rd;
        Rt[i] = rs;
    }
    else if(opcode == "ADD.D" || opcode == "SUB.D" || opcode == "MUL.D" || opcode == "DIV.D")
    {
        ss >> rd >> rs >> rt;
        ss.clear();
        rd = deleteLastSymbol(rd);
        rs = deleteLastSymbol(rs);
        op[i] = opcode;
        Rd[i] = rd;
        Rs[i] = rs;
        Rt[i] = rt;
    }
}

void printInstructionStatus()
{
    int width = 12;
    cout << left << setw(width) << ""
                    << setw(width) << ""
                    << setw(width) << ""
                    << setw(width) << ""
                    << setw(width) << ""
                    << setw(width) << "Read"
                    << setw(width) << "Execution"
                    << setw(width) << "Write"
                    << endl;
    cout << left << setw(width) << "Instruction"
                    << setw(width) << ""
                    << setw(width) << "j"
                    << setw(width) << "k"
                    << setw(width) << "Issue"
                    << setw(width) << "operands"
                    << setw(width) << "complete"
                    << setw(width) << "Result"
                    << endl;
    for(int i = 0; i < linesOfInstruction; ++i)
    {
        cout << left << setw(width) << op[i]
                        << setw(width) << Rd[i]
                        << setw(width) << Rs[i]
                        << setw(width) << Rt[i];
        for(int j = 0; j < 4; ++j)
        {
            if(InstructionStatus[i][j] != 0)
                cout << left << setw(width) << InstructionStatus[i][j];
            else if(InstructionStatus[i][j] == 0)
                cout << left << setw(width) << "";
        }
        cout << endl;
    }
}

void printFunctionUnitStatus()
{
    int width = 8;
    cout << "Functional unit status" << endl;
    cout << left << setw(width) << "Time"
                    << setw(width) << "Name"
                    << setw(width) << "Busy"
                    << setw(width) << "Op"
                    << setw(width) << "Fi"
                    << setw(width) << "Fj"
                    << setw(width) << "Fk"
                    << setw(width) << "Qj"
                    << setw(width) << "Qk"
                    << setw(width) << "Rj"
                    << setw(width) << "Rk"
                    << endl;
    for(int i = 0; i < 5; ++i)
    {
        cout << left << setw(width) << countDown[i];
        if(i == 0)
            cout << left << setw(width) << "Integer";
        else if(i == 1)
            cout << left << setw(width) << "Mult1";
        else if(i == 2)
            cout << left << setw(width) << "Mult2";
        else if(i == 3)
            cout << left << setw(width) << "Add";
        else if(i == 4)
            cout << left << setw(width) << "Divide";
        cout << left << boolalpha << setw(width) << Busy[i];
        if(Op[i] == "L.D")
            cout << left << setw(width) << "Load";
        else if(Op[i] == "ADD.D")
            cout << left << setw(width) << "Add";
        else if(Op[i] == "SUB.D")
            cout << left << setw(width) << "Sub";
        else if(Op[i] == "DIV.D")
            cout << left << setw(width) << "Div";
        else if(Op[i] == "MUL.D")
            cout << left << setw(width) << "Mult";
        else
            cout << left << setw(width) << "";

        cout << left << boolalpha << setw(width) << Fi[i]
                                  << setw(width) << Fj[i]
                                  << setw(width) << Fk[i];

        if(Qj[i] == "L.D")
            cout << left << setw(width) << "Integer";
        else if(Qj[i] == "ADD.D" || Qj[i] == "SUB.D")
            cout << left << setw(width) << "Add";
        else if(Qj[i] == "DIV.D")
            cout << left << setw(width) << "Divide";
        else if(Qj[i] == "MUL.D")
        {
            if(Fj[i] == Fi[1])
                cout << left << setw(width) << "Mult1";
            else if(Fj[i] == Fi[2])
                cout << left << setw(width) << "Mult2";
        }
        else
            cout << left << setw(width) << "";

        if(Qk[i] == "L.D")
            cout << left << setw(width) << "Integer";
        else if(Qk[i] == "ADD.D" || Qk[i] == "SUB.D")
            cout << left << setw(width) << "Add";
        else if(Qk[i] == "DIV.D")
            cout << left << setw(width) << "Divide";
        else if(Qk[i] == "MUL.D")
        {
            if(Fk[i] == Fi[1])
                cout << left << setw(width) << "Mult1";
            else if(Fk[i] == Fi[2])
                cout << left << setw(width) << "Mult2";
        }
        else
            cout << left << setw(width) << "";

        if(Busy[i] == false && Rj[i] == true)
            cout << left << setw(width) << "";
        else
            cout << left << setw(width) << Rj[i];
        if(Busy[i] == false && Rk[i] == true)
            cout << left << setw(width) << "";
        else
            cout << left << setw(width) << Rk[i];
        cout << endl;
    }
}

void issue(int cycleNum)
{
    int rd, rs, rt;
    bool firstCycle = false;
    if(cycleNum == 1)
        firstCycle = true;

    for(int i = 0; i < linesOfInstruction; ++i)
    {
        for(int j = 0; j < 16; ++j)
        {
            if(Rd[i] == RegisterResultStatus[j][0])
                rd = j;
            if(Rs[i] == RegisterResultStatus[j][0])
                rs = j;
            if(Rt[i] == RegisterResultStatus[j][0])
                rt = j;
        }

        //Not busy[FU] and not result[D]
        if(op[i] == "L.D")
        {
            if ((Busy[0] == false
                && RegisterResultStatus[rd][1] == ""
                && InstructionStatus[i - 1][0] != 0
                && InstructionStatus[i][0] == 0
                && roundComplete[i] == false)
                || (firstCycle == true))
            {
                InstructionStatus[i][0] = cycleNum;
                Busy[0] = true;
                Op[0] = op[i];
                Fi[0] = Rd[i];
                Fk[0] = Rt[i];
                if(RegisterResultStatus[rs][1] != "")
                    Qk[0] = RegisterResultStatus[rs][1];

                RegisterResultStatus[rd][1] = Op[0];

                if(Qj[0] == "")
                    Rj[0] = true;
                else
                    Rj[0] = false;
                if(Qk[0] == "")
                    Rk[0] = true;
                else
                    Rk[0] = false;
                roundComplete[i] = true;
                break;
            }
        }
        else if(op[i] == "ADD.D" || op[i] == "SUB.D")
        {
            if ((Busy[3] == false
                && RegisterResultStatus[rd][1] == ""
                && InstructionStatus[i - 1][0] != 0
                && InstructionStatus[i][0] == 0
                && roundComplete[i] == false)
                || firstCycle == true)
            {
                InstructionStatus[i][0] = cycleNum;
                Busy[3] = true;
                Op[3] = op[i];
                Fi[3] = Rd[i];
                Fj[3] = Rs[i];
                Fk[3] = Rt[i];

                if(RegisterResultStatus[rs][1] != "")
                    Qj[3] = RegisterResultStatus[rs][1];
                if(RegisterResultStatus[rt][1] != "")
                    Qk[3] = RegisterResultStatus[rt][1];

                RegisterResultStatus[rd][1] = Op[3];

                if(Qj[3] == "")
                    Rj[3] = true;
                else
                    Rj[3] = false;
                if(Qk[3] == "")
                    Rk[3] = true;
                else
                    Rk[3] = false;
                roundComplete[i] = true;
                break;
            }
        }
        else if(op[i] == "MUL.D")
        {
            if(((Busy[1] == false || Busy[2] == false)
                && RegisterResultStatus[rd][1] == ""
                && InstructionStatus[i - 1][0] != 0
                && InstructionStatus[i][0] == 0
                && roundComplete[i] == false)
                || (firstCycle == true))
            {
                if(Busy[1] == false)
                {
                    InstructionStatus[i][0] = cycleNum;
                    Busy[1] = true;
                    Op[1] = op[i];
                    Fi[1] = Rd[i];
                    Fj[1] = Rs[i];
                    Fk[1] = Rt[i];

                    if(RegisterResultStatus[rs][1] != "")
                        Qj[1] = RegisterResultStatus[rs][1];
                    if(RegisterResultStatus[rt][1] != "")
                        Qk[1] = RegisterResultStatus[rt][1];

                    RegisterResultStatus[rd][1] = Op[1];

                    if(Qj[1] == "")
                        Rj[1] = true;
                    else
                        Rj[1] = false;
                    if(Qk[1] == "")
                        Rk[1] = true;
                    else
                        Rk[1] = false;
                    roundComplete[i] = true;
                    break;
                }
                else if(Busy[2] == false)
                {
                    InstructionStatus[i][0] = cycleNum;
                    Busy[2] = true;
                    Op[2] = op[i];
                    Fi[2] = Rd[i];
                    Fj[2] = Rs[i];
                    Fk[2] = Rt[i];

                    if(RegisterResultStatus[rs][1] != "")
                        Qj[2] = RegisterResultStatus[rs][1];
                    if(RegisterResultStatus[rt][1] != "")
                        Qk[2] = RegisterResultStatus[rt][1];

                    RegisterResultStatus[rd][1] = Op[2];

                    if(Qj[2] == "")
                        Rj[2] = true;
                    else
                        Rj[2] = false;
                    if(Qk[2] == "")
                        Rk[2] = true;
                    else
                        Rk[2] = false;
                    roundComplete[i] = true;
                    break;
                }
            }
        }
        else if(op[i] == "DIV.D")
        {
            if((Busy[4] == false
                && RegisterResultStatus[rd][1] == ""
                && InstructionStatus[i - 1][0] != 0
                && InstructionStatus[i][0] == 0
                && roundComplete[i] == false)
                || (firstCycle == true))
            {
                InstructionStatus[i][0] = cycleNum;
                Busy[4] = true;
                Op[4] = op[i];
                Fi[4] = Rd[i];
                Fj[4] = Rs[i];
                Fk[4] = Rt[i];


                if(RegisterResultStatus[rs][1] != "")
                    Qj[4] = RegisterResultStatus[rs][1];
                if(RegisterResultStatus[rt][1] != "")
                    Qk[4] = RegisterResultStatus[rt][1];

                RegisterResultStatus[rd][1] = Op[4];

                if(Qj[4] == "")
                    Rj[4] = true;
                else
                    Rj[4] = false;
                if(Qk[4] == "")
                    Rk[4] = true;
                else
                    Rk[4] = false;
                roundComplete[i] = true;
                break;
            }
        }
    }
}

void rdOperands(int cycleNum)
{
    //count down
    for(int i = 0; i < linesOfInstruction; ++i)
    {
        if(countDown[i] > 0)
        {
            countDown[i] -= 1;
        }
    }
    for(int i = 0; i < linesOfInstruction; ++i)
    {
        if(op[i] == "L.D")
        {
            if(Rj[0] == true
                && Rk[0] == true
                && InstructionStatus[i][0] != 0
                && InstructionStatus[i][1] == 0
                && roundComplete[i] == false)
            {
                InstructionStatus[i][1] = cycleNum;
                countDown[i] = LDCLOCK;
                roundComplete[i] = true;
                break;
            }
        }
        else if(op[i] == "ADD.D" || op[i] == "SUB.D")
        {
            if (Rj[3] == true
                && Rk[3] == true
                && InstructionStatus[i][0] != 0
                && InstructionStatus[i][1] == 0
                && roundComplete[i] == false)
            {
                InstructionStatus[i][1] = cycleNum;
                countDown[i] = ADDCLOCK;
                roundComplete[i] = true;
                break;
            }
        }
        else if(op[i] == "MUL.D")
        {
            //特別處理 有兩個mul
            for(int j = 1; j <= 2; ++j)
            {
                if(Busy[j] == true)
                {
                    if (Rj[j] == true
                        && Rk[j] == true
                        && InstructionStatus[i][0] != 0
                        && InstructionStatus[i][1] == 0
                        && roundComplete[i] == false)
                    {
                        if(Fi[j] == Rd[i] && Fj[j] == Rs[i] && Fk[j] == Rt[i])
                        {
                            InstructionStatus[i][1] = cycleNum;
                            countDown[i] = MULTCLOCK;
                            roundComplete[i] = true;
                        }
                    }
                }
            }
        }
        else if(op[i] == "DIV.D")
        {
            if (Rj[4] == true
                && Rk[4] == true
                && InstructionStatus[i][0] != 0
                && InstructionStatus[i][1] == 0
                && roundComplete[i] == false)
            {
                InstructionStatus[i][1] = cycleNum;
                countDown[i] = DIVCLOCK;
                roundComplete[i] = true;
                break;
            }
        }
    }

}

void exeComplete(int cycleNum)
{
    for(int i = 0; i < linesOfInstruction; ++i)
    {
        if(InstructionStatus[i][1] != 0  && InstructionStatus[i][2] == 0 && roundComplete[i] == false && countDown[i] == 0)
        {
            InstructionStatus[i][2] = cycleNum;
            roundComplete[i] = true;
        }
    }
}

void wrResult(int cycleNum)
{
    //我的Rd不能是別人的Rs或Rt 我的Rj和Rk要等於true
    for(int i = 0; i < linesOfInstruction; ++i)
    {
        bool ready = true;
        if(InstructionStatus[i][2] != 0 && InstructionStatus[i][3] == 0 && roundComplete[i] == false)
        {
            for(int j = 0; j < 5; ++j)
            {
                if(op[i] == Op[j] && Busy[j] == true && Rd[i] == Fi[j]) //found in scoreboard, j
                {
                    if(Rj[j] == false || Rk[j] == false)
                    {
                        ready = false;

                        break;
                    }
                    for(int k = 0; k < linesOfInstruction; ++k)
                    {
                        for(int a = 0; a < i; ++a)
                        {
                            if(Fi[j] == Rs[a] || Fi[j] == Rt[a])
                            {
                                if(InstructionStatus[a][1] == 0 || roundComplete[a] == true)
                                {
                                    ready = false;
                                }
                            }
                        }
                    }
                }
            }

            if(ready == true)
            {
                InstructionStatus[i][3] = cycleNum;
                for(int j = 0; j < 5; ++j)
                {
                    if(Qj[j] == op[i])
                    {
                        Qj[j] = "";
                        Rj[j] = true;
                    }
                    if(Qk[j] == op[i])
                    {
                        Qk[j] = "";
                        Rk[j] = true;
                    }
                }
                for(int j = 0; j < 5; ++j)
                {
                    if(Busy[j] == true && Op[j] == op[i] && Fi[j] == Rd[i])
                    {
                        for(int k = 0; k < 16; ++k)
                        {
                            if(RegisterResultStatus[k][0] == Fi[j])
                            {
                                RegisterResultStatus[k][1] = "";
                                break;
                            }
                        }
                        Busy[j] = false;
                        Op[j] = "";
                        Fi[j] = "";
                        Fj[j] = "";
                        Fk[j] = "";
                        Qj[j] = "";
                        Qk[j] = "";
                        Rj[j] = true;
                        Rk[j] = true;
                        break;
                    }
                }

                roundComplete[i] = true;
            }
        }
    }
}

void printRegResult()
{
    int width = 8;
    cout << "Register result status" << endl;
    for(int i = 0; i < 16; ++i)
    {
        if(RegisterResultStatus[i][1] != "")
        {
            cout << left << setw(width) << RegisterResultStatus[i][0];
        }
    }
    cout << endl;
    for(int i = 0; i < 16; ++i)
    {
        if(RegisterResultStatus[i][1] != "")
        {
            if(RegisterResultStatus[i][1] == "L.D")
                cout << left << setw(width) << "Integer";
            else if(RegisterResultStatus[i][1] == "ADD.D" || RegisterResultStatus[i][1] == "SUB.D")
                cout << left << setw(width) << "Add";
            else if(RegisterResultStatus[i][1] == "DIV.D")
                cout << left << setw(width) << "Divide";
            else if(RegisterResultStatus[i][1] == "MUL.D")
            {
                if(RegisterResultStatus[i][0] == Fi[1])
                    cout << left << setw(width) << "Mult1";
                else if(RegisterResultStatus[i][0] == Fi[2])
                    cout << left << setw(width) << "Mult2";
            }
        }
    }
    cout << endl;
}

int main()
{
    string line;
    while(getline(inFile, line))
    {
        ++linesOfInstruction;
        instructionPreProccess(line);
    }

    while(1)
    {
        cout << "=====cycle " << cycle << "=====" << endl;
        for(int j = 0; j < linesOfInstruction; ++j)
        {
            roundComplete[j] = false;
        }

        issue(cycle);
        rdOperands(cycle);
        exeComplete(cycle);
        wrResult(cycle);
        ++cycle;

        printInstructionStatus();
        cout << endl;
        printFunctionUnitStatus();
        cout << endl;
        printRegResult();
        cout << endl;

        bool finish = true;
        for(int j = 0; j < linesOfInstruction; ++j)
        {
            if(InstructionStatus[j][0] == 0 || InstructionStatus[j][1] == 0 || InstructionStatus[j][2] == 0 || InstructionStatus[j][3] == 0)
            {
                finish = false;
            }
        }
        if(finish == true)
        {
            break;
        }
    }
    cout << "=====FINISH=====" << endl;
    system("pause");
}
