#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

//declare global variables
ifstream inFile("input.txt");
ofstream outFile;
stringstream ss;
string stage[5] = {"nop", "nop", "nop", "nop", "nop"};
string predictTaken;
int reg[8][4] = {{16, 0, 0, 0}, {17, 0, 0, 0}, {18, 0, 0, 0}, {19, 0, 0, 0}, {20, 0, 0, 0}, {21, 0, 0, 0}, {22, 0, 0, 0}, {23, 0, 0, 0}}; //$16, $17, $18, $19, $20, $21, $22, $23
int counter = 1;

string deleteComma(string);
string deleteFirstSymbol(string);
void shiftStage(string, int);
void printStage(string);
void printRegisters(int);
void lw(string, string);
void sw(string, string);
void add(string, string, string);
void addi(string, string, string);
void sub(string, string, string);
void beq(string, string, string, string);
void branch(string);
void lock();
void forwarding();
bool stall();
void clearStage();
void unLock();
void shiftNPrint(string, int);
void execute(string);



string deleteComma(string input)
{
    string output = input.substr(0, input.length() - 1);
    return output;
}

string deleteFirstSymbol(string input)
{
    string output = input.erase(0, 1);
    return output;
}

//shift instructions into next stage
void shiftStage(string stage[5], int stop)
{
    for(int i = 3; i >= stop; --i)
        stage[i + 1] = stage[i];
}

void printStage(string stage[5])
{
    outFile << "===clock cycle " << counter << "===" << endl;
    outFile << "IF :" << stage[0] << endl;
    outFile << "ID :" << stage[1] << endl;
    outFile << "EX :" << stage[2] << endl;
    outFile << "MEM:" << stage[3] << endl;
    outFile << "WB :" << stage[4] << endl << endl;
}

void printRegisters(int reg[8][4])
{
    outFile << "$16~$23" << endl;
    outFile << reg[0][1] << " "
            << reg[1][1] << " "
            << reg[2][1] << " "
            << reg[3][1] << " "
            << reg[4][1] << " "
            << reg[5][1] << " "
            << reg[6][1] << " "
            << reg[7][1] << endl;
}

//load word from address to rd
void lw(string rd, string address)
{
    int intRd;
    ss << rd;
    ss >> intRd;
    ss.clear();

    //here we just simply update rd as 1
    for(int i = 0; i < 8; ++i)
    {
        if(intRd == reg[i][0])
            reg[i][1] = 1;
    }
}

//save word from rt to address,
void sw(string rt, string address)
{
     //here we just simply do nothing
}

//adds rs, rt and stores into rd
void add(string rd, string rs, string rt)
{
    int intRd, intRs, intRt;
    int sum = 0;

    //convert rs, rt and rd into int in order to compare
    ss << rd << " " << rs << " " << rt;
    ss >> intRd >> intRs >> intRt;
    ss.clear();

    //find rs, rt and add to sum
    for(int i = 0; i < 8; ++i)
    {
        if(intRs == reg[i][0])
            sum += reg[i][1];
        if(intRt == reg[i][0])
            sum += reg[i][1];
    }

    //store sum into rd
    for(int i = 0; i < 8; ++i)
    {
        if(intRd == reg[i][0])
            reg[i][1] = sum;
    }
}

//adds rs, imm and stores into rt
void addi(string rt, string rs, string imm)
{
    int intRt, intRs, intImm;
    int sum = 0;

    //convert rt, rs and imm into int in order to compare
    ss << rt << " " << rs << " " << imm;
    ss >> intRt >> intRs >> intImm;
    ss.clear();

    //find rs
    for(int i = 0; i < 8; ++i)
    {
        if(intRs == reg[i][0])
            sum += reg[i][1];
    }

    //add imm to sum
    sum += intImm;

    //store sum to rt
    for(int i = 0; i < 8; ++i)
    {
        if(intRt == reg[i][0])
            reg[i][1] = sum;
    }
}

//subtracts rt from rs and stores into rd
void sub(string rd, string rs, string rt)
{
    int intRd, intRs, intRt;
    int sum = 0;

    //convert rs, rt and rd into int in order to compare
    ss << rd << " " << rs << " " << rt;
    ss >> intRd >> intRs >> intRt;
    ss.clear();

    //find rs, rt and rd and add to sum
    for(int i = 0; i < 8; ++i)
    {
        if(intRs == reg[i][0])
            sum += reg[i][1];
        if(intRt == reg[i][0])
            sum -= reg[i][1];

    }

    //store sum into rd
    for(int i = 0; i < 8; ++i)
    {
        if(intRd == reg[i][0])
            reg[i][1] = sum;
    }
}

//compares rs and rt. If equal, jump to label
void beq(string rs, string rt, string label, string predictTaken)
{
    int intRs, intRt, a, b;
    ss << rs << " " << rt;
    ss >> intRs >> intRt;
    ss.clear();

    //get value of rs and rt in order to compare
    for(int i = 0; i < 8; ++i)
    {
        if(intRs == reg[i][0])
            a = reg[i][1];
        if(intRt == reg[i][0])
            b = reg[i][1];
    }

    //predict wrong
    if(((a != b) && (predictTaken == "T")) || ((a == b) && (predictTaken == "NT")))
    {
        //bubble
        ++counter;
        shiftNPrint("Bubble(nop)", 0);
        unLock();
        clearStage();
    }
    //predict right do nothing
    //if equal, branch
    if(a == b)
        branch(label);
}

//jumps to label
void branch(string label)
{
    //open another file to execute separate from main branch
    ifstream inFileBranch("input.txt");
    string lineBranch, opBranch;
    int found = 0;
    while(getline(inFileBranch, lineBranch))
    {
        if(found == 1)
        {
            execute(lineBranch); //execute branch
            continue;
        }

        //find label line by line
        for(int i = 0; i < label.length(); ++i) //found
        {
            if(i == label.length() - 1)
            {
                found = 1;
                break;
            }
            if((lineBranch[i] != label[i]) && (found == 0)) //not found
                break;
        }
    }
}

//lock the registers that are not ready until a certain stage
void lock()
{
    string op, rd, rs, rt, address, dontCare;
    ss << stage[2];  //we only check stage 3 since only from stage 3 there will be hazards
    ss >> op;
    if(op == "lw" || op == "sw" || op == "add" || op == "sub" || op == "addi" || op == "beq")
    {
        if(op == "lw" || op == "sw")
        {
            int intRd, intAddress;
            ss >> rd >> address;
            ss.clear();
            rd = deleteComma(rd);
            rd = deleteFirstSymbol(rd);
            while(address[0] != '$')
                address = deleteFirstSymbol(address);
            address = deleteComma(address);
            address = deleteFirstSymbol(address);
            ss << rd << " " << address;
            ss >> intRd >> intAddress;
            ss.clear();
            for(int i = 0; i < 8; ++i)
            {
                if(intRd == reg[i][0])
                {
                    if(op == "lw")
                    {
                        reg[i][2] += 1;  //lock rd until WB
                        reg[i][3] = 3; //this register is now in stage 3
                    }
                }
                if(intAddress == reg[i][0])
                {
                    if(op == "sw")
                    {
                        reg[i][2] += 1;
                        reg[i][3] = 3;
                    }
                }
            }
        }
        else if(op == "add" || op == "sub")
        {
            int intRd, intRs, intRt;
            ss >> rd >> rs >> rt;
            ss.clear();
            rd = deleteComma(rd);
            rd = deleteFirstSymbol(rd);
            rs = deleteComma(rs);
            rs = deleteFirstSymbol(rs);
            rt = deleteFirstSymbol(rt);
            ss << rd << " " << rs << " " << rt;
            ss >> intRd >> intRs >> intRt;
            ss.clear();
            for(int i = 0; i < 8; ++i)
            {
                if(intRd == reg[i][0])
                {
                    reg[i][2] += 1;
                    reg[i][3] = 3;
                }
            }
        }
        else if(op == "addi")
        {
            int intRt, intRs;
            ss >> rt >> rs >> dontCare;
            ss.clear();
            rt = deleteComma(rt);
            rt = deleteFirstSymbol(rt);
            rs = deleteComma(rs);
            rs = deleteFirstSymbol(rs);
            ss << rt << " " << rs;
            ss >> intRt >> intRs;
            ss.clear();
            for(int i = 0; i < 8; ++i)
            {
                if(intRt == reg[i][0])
                {
                    reg[i][2] += 1;
                    reg[i][3] = 3;
                }
            }
        }
    }
    else
    {
        ss.clear();
        ss.str("");
    }
}

//check the need to forward
void forwarding()
{
    string op, rd, rs, rt, address, dontCare;
    ss << stage[2]; //only check stage 3 since forwarding will only happen in stage 3
    ss >> op;
    if(op == "lw" || op == "sw" || op == "add" || op == "sub" || op == "addi" || op == "beq")
    {
        if(op == "lw" || op == "sw")
        {
            int intRd, intAddress;
            ss >> rd >> address;
            ss.clear();
            rd = deleteComma(rd);
            rd = deleteFirstSymbol(rd);
            while(address[0] != '$')
                address = deleteFirstSymbol(address);
            address = deleteComma(address);
            address = deleteFirstSymbol(address);
            ss << rd << " " << address;
            ss >> intRd >> intAddress;
            ss.clear();
            for(int i = 0; i < 8; ++i)
            {
                if(intRd == reg[i][0])
                    if(op == "sw")
                        if(reg[i][2] != 0)  //check if register is available, if not, forward.
                            outFile << endl << "Forwarding from stage " << reg[i][3] << " to stage 3." << endl;
                if(intAddress == reg[i][0])
                    if(op == "lw")
                        if(reg[i][2] != 0)
                            outFile << endl << "Forwarding from stage " << reg[i][3] << " to stage 3." << endl;
            }
        }
        else if(op == "add" || op == "sub")
        {
            int intRd, intRs, intRt;
            ss >> rd >> rs >> rt;
            ss.clear();
            rd = deleteComma(rd);
            rd = deleteFirstSymbol(rd);
            rs = deleteComma(rs);
            rs = deleteFirstSymbol(rs);
            rt = deleteFirstSymbol(rt);
            ss << rd << " " << rs << " " << rt;
            ss >> intRd >> intRs >> intRt;
            ss.clear();
            for(int i = 0; i < 8; ++i)
            {
                if(intRs == reg[i][0])
                    if(reg[i][2] != 0)
                        outFile << endl << "Forwarding from stage " << reg[i][3] << " to stage 3." << endl;
                if(intRt == reg[i][0])
                    if(reg[i][2] != 0)
                        outFile << endl << "Forwarding from stage " << reg[i][3] << " to stage 3." << endl;
            }
        }
        else if(op == "addi")
        {
            int intRt, intRs;
            ss >> rt >> rs >> dontCare;
            ss.clear();
            rt = deleteComma(rt);
            rt = deleteFirstSymbol(rt);
            rs = deleteComma(rs);
            rs = deleteFirstSymbol(rs);
            ss << rt << " " << rs;
            ss >> intRt >> intRs;
            ss.clear();
            for(int i = 0; i < 8; ++i)
                if(intRs == reg[i][0])
                    if(reg[i][2] != 0)
                        outFile << endl << "Forwarding from stage " << reg[i][3] << " to stage 3." << endl;
        }
        else if(op == "beq")
        {
            int intRt, intRs;
            ss >> rs >> rt >> dontCare;
            ss.clear();
            rt = deleteComma(rt);
            rt = deleteFirstSymbol(rt);
            rs = deleteComma(rs);
            rs = deleteFirstSymbol(rs);
            ss << rt << " " << rs;
            ss >> intRt >> intRs;
            ss.clear();
            for(int i = 0; i < 8; ++i)
            {
                if(intRt == reg[i][0])
                    if(reg[i][2] != 0)
                        outFile << endl << "Forwarding from stage " << reg[i][3] << " to stage 3." << endl;
                if(intRs == reg[i][0])
                    if(reg[i][2] != 0)
                        outFile << endl << "Forwarding from stage " << reg[i][3] << " to stage 3." << endl;
            }
        }
    }
    else
        ss.clear();
}

//check if there is a need to stall
bool stall()
{
    bool forwarding = false;
    string idOp, idRd, idRt, idRs;
    string exOp, exRd, address;
    ss << stage[2];  //only check stage 3 since only stage 3 will there be stalling
    ss >> exOp;
    if(exOp == "lw")
    {
        ss >> exRd >> address;
        exRd = deleteComma(exRd);
        ss.clear();
        ss << stage[1];
        ss >> idOp;
        if(idOp == "add" || idOp == "sub")
        {
            ss >> idRd >> idRt >> idRs;
            ss.clear();
            idRt = deleteComma(idRt);
            if(exRd == idRt || exRd == idRs) //(ID/EX.RegisterRt = IF/ID.RegisterRs) or (ID/EX.RegisterRt = IF/ID.RegisterRt)
            {
                //Bubble will shift stages and print stages individually
                unLock();
                clearStage();
                shiftNPrint("Bubble(nop)", 2);
                lock();
                ++counter;
                forwarding = true;
            }
        }
        else
        {
            ss.clear();
            ss.str("");
        }
    }
    else
    {
        ss.clear();
        ss.str("");
    }
    return forwarding;
}

//set the register status that are in stage six to 0, which means such register is not locked by any instruction
void clearStage()
{
    for(int i = 0; i < 8; ++i)
        if(reg[i][3] >= 6)
            reg[i][3] = 0;
}

//unlock registers that are released by instructions when finished
void unLock()
{
    string op, rd, rs, rt, address;
    ss << stage[3];  //stage 4
    ss >> op;
    if(op == "sw")
    {
        ss >> rd >> address;
        ss.clear();
        while(address[0] != '$')
            address = deleteFirstSymbol(address);
        address = deleteComma(address);
        address = deleteFirstSymbol(address);
        int intAddress;
        ss << address;
        ss >> intAddress;
        ss.clear();
        for(int i = 0; i < 8; ++i)
            if(intAddress == reg[i][0])
                reg[i][2] -= 1;
    }
    else
    {
        ss.clear();
        ss.str("");
    }

    ss << stage[4];  //stage 5
    ss >> op;
    if(op == "add" || op == "sub" || op == "addi")
    {
        ss >> rd >> rs >> rt;
        ss.clear();
        rd = deleteComma(rd);
        rd = deleteFirstSymbol(rd);
        int intRd;
        ss << rd;
        ss >> intRd;
        ss.clear();
        for(int i = 0; i < 8; ++i)
            if(intRd == reg[i][0])
                reg[i][2] -= 1;
    }
    else if(op == "lw")
    {
        ss >> rd >> address;
        ss.clear();
        rd = deleteComma(rd);
        rd = deleteFirstSymbol(rd);
        int intRd;
        ss << rd;
        ss >> intRd;
        ss.clear();
        for(int i = 0; i < 8; ++i)
            if(intRd == reg[i][0])
                reg[i][2] -= 1;
    }
    else
    {
        ss.clear();
        ss.str("");
    }
}

//shift stages and print the stages
void shiftNPrint(string input, int stop)
{
    shiftStage(stage, stop);
    for(int i = 0; i < 8; ++i)
        if(reg[i][3] != 0)
            reg[i][3] += 1;
    stage[stop] = input;
    printStage(stage);
}

//executes the instructions
void execute(string line)
{
    string op, rs, rt, rd, address, imm, label;
    ss << line;
    ss >> op;
    if((op == "lw") || (op == "sw") || (op == "add") || (op == "addi") || (op == "sub") || (op == "beq") || (op == "nop"))
    {
        shiftNPrint(line, 0);
        if(op == "lw")
        {
            ss >> rd >> address;
            ss.clear();
            rd = deleteComma(rd);
            rd = deleteFirstSymbol(rd);
            lw(rd, address);
        }
        else if(op == "sw")
        {
            ss >> rt >> address;
            ss.clear();
            rt = deleteComma(rt);
            rt = deleteFirstSymbol(rt);
            sw(rt, address);
        }
        else if(op == "add")
        {
            ss >> rd >> rs >> rt;
            ss.clear();
            rd = deleteComma(rd);
            rs = deleteComma(rs);
            rd = deleteFirstSymbol(rd);
            rs = deleteFirstSymbol(rs);
            rt = deleteFirstSymbol(rt);
            add(rd, rs, rt);
        }
        else if(op == "addi")
        {
            ss >> rt >> rs >> imm;
            ss.clear();
            rt = deleteComma(rt);
            rs = deleteComma(rs);
            rt = deleteFirstSymbol(rt);
            rs = deleteFirstSymbol(rs);
            addi(rt, rs, imm);
        }
        else if(op == "sub")
        {
            ss >> rd >> rs >> rt;
            ss.clear();
            rd = deleteComma(rd);
            rs = deleteComma(rs);
            rd = deleteFirstSymbol(rd);
            rs = deleteFirstSymbol(rs);
            rt = deleteFirstSymbol(rt);
            sub(rd, rs, rt);
        }
        else if(op == "beq")
        {
            ss >> rs >> rt >> label;
            ss.clear();
            rs = deleteComma(rs);
            rt = deleteComma(rt);
            rs = deleteFirstSymbol(rs);
            rt = deleteFirstSymbol(rt);
            beq(rs, rt, label, predictTaken);
        }
        ++counter;
        ss.clear();
        ss.str("");
    }
    else
        ss.clear();
}

int main()
{
    outFile.open("output.txt");
    string line;

    //get predict taken or not
    getline(inFile, line);
    ss << line;
    ss >> predictTaken;
    ss.clear();

    //get register value
    getline(inFile, line);
    ss << line;
    ss >> reg[0][1] >> reg[1][1] >> reg[2][1] >> reg[3][1] >> reg[4][1] >> reg[5][1] >> reg[6][1] >> reg[7][1];
    ss.clear();

    while(getline(inFile, line))
    {
        string op;
        ss << line;
        ss >> op;
        ss.str("");
        if(op == "lw" || op == "sw" || op == "add" || op == "sub" || op == "addi" || op == "beq")
        {
            execute(line);
            lock();  //lock the registers that are in use
            if(stall() == false)  //check for stall
                forwarding();  //if not stalled then check for forwarding
            unLock();  //unlock the registers that are no more in use
            clearStage();  //clear the register status which instructions are no more in the MIPS
        }
        else
            continue;
    }

    while(stage[3] != "nop")
    {
        execute("nop");
        lock();
        if(stall() == false)
            forwarding();
        unLock();
        clearStage();
    }
    printRegisters(reg);  //print register one last time
    cout << "Done! Results saved to output.txt" << endl;
}
