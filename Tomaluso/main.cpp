#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <conio.h>
#include <locale>
#include <algorithm>
#include <vector>


using namespace std;

int clk = 0;
int R[32], RAT_R[32], RAT_F[32]; 
float F[32], Mem[256];


string IntAdderInstrns[5] = {"beq", "bne", "add", "addi", "sub"};
string FPAdderInstrns[2] = {"add_d", "sub_d"};
string FPMulInstrns[2] = {"mult_d", "div_d"};
string LDInstrns[2] = {"ld", "sd"};
vector<string> code;										//Contains the lines of code

bool BelongsToFU(string operation, string FUInstrns[], int Num_FUInstrns)
{
	for (int i=0; i<Num_FUInstrns; i++)
		if (operation == FUInstrns[i])
			return true;
	return false;
}



struct TimingTable {										//Create the final IS-EX-MEM-WB-COMMIT table
	int ISSUE, EX0, EX1, MEM0, MEM1, WB, COMMIT;

	TimingTable() { ISSUE = EX0 = EX1 = MEM0 = MEM1 = WB = COMMIT = 0; }
public: void print()
		{
			cout<<"  ";
			if (ISSUE < 10) {cout<<" ";}
			cout<<ISSUE<<"  |  ";
			if (EX0 < 10) {cout<<" ";}
			cout<<EX0<<"-";
			if (EX1 < 10) {cout<<" ";}
			cout<<EX1<<"  | ";
			if (MEM0 == 0) {cout << "   - ";}
			else 
			{
				if (MEM0 < 10) {cout<<" ";}
				cout<<MEM0<<"-";
				if (MEM1 < 10) {cout<<" ";}
				cout<<MEM1;
			}
			cout<<"  |  ";
			if (WB < 10) {cout<<" ";}
			cout<<WB<<"  |   ";
			if (COMMIT < 10) {cout<<" ";}
			cout<<COMMIT<<"   |"<<endl;
		}
};
vector<TimingTable> CT, FT;



struct ReservationStation {										//Create the Reservation Station
	string Op, Qj, Qk;
	int Dst_Tag;
	float Vj, Vk;
	int code_cnt;

	ReservationStation() {Op = Qj = Qk = ""; Dst_Tag = code_cnt = -1; Vj = Vk = 0.0;}

public: bool isEmpty() 
	{
		if (Op == "") return true;
		else return false;
	}

public: void print(int maxVjCol, int maxIntPartVj) 
	{
		if (!(this->isEmpty()))
		{
			cout << Op;
			for (int i=Op.size(); i<6; i++)
				cout<<" ";
			if (Dst_Tag != -1)
			{
				cout<<" |  ROB" << Dst_Tag;
				if (Dst_Tag < 10)
					cout<<" ";
			}

			cout << "  |  ";
			if (Qj == "")
				cout<<"     ";
			else
			{
				if (Qj.size() == 5)
					cout<<Qj;
				else if (Qj.size() == 4)
					cout<<Qj<<" ";
			}
			cout<< "  |  " ;
			if (Qk == "")
				cout<<"     ";
			else
			{
				if (Qk.size() == 5)
					cout<<Qk;
				else if (Qk.size() == 4)
					cout<<Qk<<" ";
			}
			cout<< "  | " ;
			if (Qj == "")
			{
				stringstream ss (stringstream::in | stringstream::out);
				ss << Vj;
				int curIntPartVj, curFractPartVj;
				if (ss.str().find('.') != string::npos)
				{
					curIntPartVj = ss.str().find('.');
					curFractPartVj = ss.str().size()-ss.str().find('.')-1;
				}
				else
				{
					curIntPartVj = ss.str().size();
					curFractPartVj = -1;
				}
				
				for (int j=0; j<maxIntPartVj-curIntPartVj; j++)
					cout<<" ";
				cout << Vj;
				for (int j=0; j<maxVjCol-maxIntPartVj-curFractPartVj-1; j++)
					cout<<" ";
			}
			else
			{
				for (int j=0; j<maxVjCol; j++) cout << " ";
			}
			cout << " | ";

			if (Qk == "")
			{
				stringstream ss (stringstream::in | stringstream::out);
				ss << Vk;
				int curIntPartVk, curFractPartVk;
				if (ss.str().find('.') != string::npos)
				{
					curIntPartVk = ss.str().find('.');
					curFractPartVk = ss.str().size()-ss.str().find('.')-1;
				}
				else
				{
					curIntPartVk = ss.str().size();
					curFractPartVk = -1;
				}
				
				for (int j=0; j<maxIntPartVj-curIntPartVk; j++)
				cout<<" ";
				cout << Vk;
				for (int j=0; j<maxVjCol-maxIntPartVj-curFractPartVk-1; j++)
					cout<<" ";
			}
			else
				for (int j=0; j<maxVjCol; j++) cout << " ";

			cout<<" |"<<endl;
		}
		else
		{
			cout << "------ | ------- | ------- | ------- | ";
			for (int i=0; i<maxVjCol; i++) cout<<"-";
			cout<<" | ";
			for (int i=0; i<maxVjCol; i++) cout<<"-";
			cout<<" |" << endl;
		}
	}

public: void clear() { Op = Qj = Qk = ""; Dst_Tag = code_cnt = -1; Vj = Vk = 0.0; }
};

struct LS_Queue {
	string address, val, op;
	int code_cnt;

	LS_Queue () {address = ""; val = ""; op = ""; code_cnt = -1;}

public: void clear() { address = ""; val = ""; op = ""; code_cnt = -1; }
};


struct Integer_Adder {										//Create the Integer Adder
															//STATIC variables and functions are shared among the FUs
	static int num_RS, cycles_EX, num_FU;
	static const int cycles_MEM = 0;

	vector<ReservationStation> RS;
	int used_RS;
	
	Integer_Adder() {used_RS = 0;}

	void Initialize_RS() {
		for (int i=0; i<num_RS; i++)
		{
			ReservationStation temp;
			RS.push_back(temp);
		}
	}
};
int Integer_Adder::num_RS, Integer_Adder::cycles_EX, Integer_Adder::num_FU;


struct FP_Adder {										//Create the FP Adder
	static int num_RS, cycles_EX, num_FU;
	static const int cycles_MEM = 0;

	vector<ReservationStation> RS;
	int used_RS;

	FP_Adder() {used_RS = 0;}

	void Initialize_RS() {
		for (int i=0; i<num_RS; i++)
		{
			ReservationStation temp;
			RS.push_back(temp);
		}
	}
};
int FP_Adder::num_RS, FP_Adder::cycles_EX, FP_Adder::num_FU; 


struct FP_Multiplier {										//Create the FP Multiplier
	static int num_RS, cycles_EX, num_FU;
	static const int cycles_MEM = 0;

	vector<ReservationStation> RS;
	int used_RS;

	FP_Multiplier() {used_RS = 0;}

	void Initialize_RS() {
		for (int i=0; i<num_RS; i++)
		{
			ReservationStation temp;
			RS.push_back(temp);
		}
	}
};
int FP_Multiplier::num_RS, FP_Multiplier::cycles_EX, FP_Multiplier::num_FU;


struct LS_Unit {										//Create the Load/Store Unit
	static int num_LSQ, cycles_EX, cycles_MEM, num_FU;

	vector<LS_Queue> LSQ;
	int used_LSQ;

	LS_Unit() {used_LSQ = 0;}

	void Initialize_LSQ() {
		for (int i=0; i<num_LSQ; i++)
		{
			LS_Queue temp;
			LSQ.push_back(temp);
		}
	}
};
int LS_Unit::num_LSQ, LS_Unit::cycles_EX, LS_Unit::cycles_MEM, LS_Unit::num_FU;



struct ReOrderBuffer {										//Create the Ro-order Buffer
	string Type, Dst;
	float Val;
	bool Ready;
	int code_cnt;

	ReOrderBuffer() {Type = Dst = ""; Val = 0.0; Ready = false; code_cnt = -1;}

public: bool isEmpty() 
	{
		if (Type == "") return true;
		else return false;
	}

public: void print(int i, int maxThirdCol, int maxIntPart, int curIntPart, int curFractPart)
	{
		if (!(this->isEmpty())) { 
			cout << "ROB" << i;
			if (i < 100) cout << " ";
			if (i < 10) cout << " ";
			cout << " | " << Type << "\t | " << Dst << "\t | ";
			for (int j=0; j<maxIntPart-curIntPart; j++)
				cout<<" ";
			cout << Val;
			for (int j=0; j<maxThirdCol-maxIntPart-curFractPart-1; j++)
				cout<<" ";
			cout << " |   " << Ready << "   |" << endl; 
		}
	}

public: void clear() { Type = Dst = ""; Val = 0.0; Ready = false; code_cnt = -1;}
};
int used_ROB = 0;
int ROB_entries = 0;




//Function used in FormatLine(string)
bool BothAreSpaces(char lhs, char rhs) {					
	return ((lhs == rhs) && ((lhs == ' ') || (lhs == '\t'))) || ((lhs == ' ') && (rhs == '\t')) || ((lhs == '\t') && (rhs == ' ')); 
}

//Function to remove extra tabs and spaces and convert to lowercase
string FormatLine (string line) {
	//Replace tabs with spaces
	std::replace( line.begin(), line.end(), '\t', ' ');

	//Remove leading and trailing blanks
	string::size_type begin = line.find_first_not_of(" \t");
	string::size_type end   = line.find_last_not_of(" \t");
	line = line.substr(begin, end + 1);

	//Remove multiple instances of spaces
	string::iterator new_end = std::unique(line.begin(), line.end(), BothAreSpaces);
	line.erase(new_end, line.end()); 

	//Convert to lowercase
	std::locale loc;
	for (std::string::size_type i=0; i<line.length(); ++i)
		line[i] = std::tolower(line[i],loc);
	return line;
}

//Function to break a line into it's words
vector<string> BreakLine (string line) {
	string delimiter = " ";								//Delimiter to seperate words on a line
	char extras[] = ",";								//Extra chars that have to be removed from each word
	string token = line;								//This will be looped and cut in the while loop
	string temp;
	vector<string> string_list;		
	int count = 0;
			
	while (token.find(delimiter) != string::npos)			//Loop to seperate a line at ", " - This will seperate the operands except the first one
	{
		temp = token.substr(0, token.find(delimiter));		 //Get the first word

		for (unsigned int i = 0; i < strlen(extras); ++i)   //Remove the extras
			temp.erase (std::remove(temp.begin(), temp.end(), extras[i]), temp.end());
					
		string_list.push_back(temp);
		token = token.erase(0, token.find(delimiter)+delimiter.length());  //Cut the front of the token for the next iteration
		count++;
	}
	string_list.push_back(token);
	return string_list;
}

//Function to parse the line
void ParseLine(vector<string> string_list, string line) {
	if (string_list[0] == "rob")
	{
		ROB_entries = atoi(string_list[3].c_str());
	}
	else if (string_list[0] == "integer" && string_list[1] == "adder")
	{
		Integer_Adder::num_FU = atoi(string_list[4].c_str());
		Integer_Adder::cycles_EX = atoi(string_list[3].c_str());
		Integer_Adder::num_RS = atoi(string_list[2].c_str());
	}
	else if (string_list[0] == "fp" && string_list[1] == "adder")
	{
		FP_Adder::num_FU = atoi(string_list[4].c_str());
		FP_Adder::cycles_EX = atoi(string_list[3].c_str());
		FP_Adder::num_RS = atoi(string_list[2].c_str());
	}
	else if (string_list[0] == "fp" && string_list[1] == "multiplier")
	{
		FP_Multiplier::num_FU = atoi(string_list[4].c_str());
		FP_Multiplier::cycles_EX = atoi(string_list[3].c_str());
		FP_Multiplier::num_RS = atoi(string_list[2].c_str());
	}
	else if (string_list[0] == "load/store" && string_list[1] == "unit")
	{
		LS_Unit::num_FU = atoi(string_list[5].c_str());
		LS_Unit::cycles_MEM = atoi(string_list[4].c_str());
		LS_Unit::cycles_EX = atoi(string_list[3].c_str());
		LS_Unit::num_LSQ = atoi(string_list[2].c_str());
	}
	else if (string_list[0].find('=') != string::npos)
	{
		for (int i=0; i < string_list.size(); i++)
		{
			if (string_list[i].at(0) == 'r')
			{
				int reg_num = atoi(string_list[i].substr(1,string_list[i].find('=')-1).c_str());
				int reg_val = atoi(string_list[i].substr(string_list[i].find('=')+1,string::npos).c_str());
				R[reg_num] = reg_val;
			}
			else if (string_list[i].at(0) == 'f')
			{
				int reg_num = atoi(string_list[i].substr(1,string_list[i].find('=')-1).c_str());
				float reg_val = std::stof(string_list[i].substr(string_list[i].find('=')+1,string::npos));
				F[reg_num] = reg_val;
			}
			else if (string_list[i].at(0) == 'm')
			{
				int mem_add = atoi(string_list[i].substr(4,string_list[i].find(']')-1).c_str());
				float mem_val = atof(string_list[i].substr(string_list[i].find('=')+1,string::npos).c_str());
				Mem[mem_add] = mem_val;
			}
		}
	}
	else if (!string_list.empty() && (string_list.size() < 5))
		code.push_back(line);
}





int main() 
{
	R[0] = 0;
	for (int i=0; i<32; i++) {RAT_R[i] = RAT_F[i] = -1;}
	ifstream myfile("\\\\psf\\Home\\Desktop\\test2.txt");		//Open the input file
	
	if (myfile.is_open())										//If file can be opened, start reading line by line and parse the data
	{
		string line;											//Contains current line to be parsed
		while (getline(myfile, line))							//Loop to get next line
		{
			if (!line.empty())
			{
				line = FormatLine(line);						//Function to remove extra tabs and spaces and convert to lowercase
				vector<string> string_list = BreakLine(line);	//Function to break a line into it's words		
				ParseLine(string_list, line);					//Function to parse the line
			}
		}
		myfile.close();											//Close the input file
	} 
	else														//If file cannot be opened, show error
		cout << "Error opening file.";


	//Print the initial state before starting the algorithm to confirm that data is parsed correctly
	cout<<endl<<"                # of rs    Cycles in Ex    Cycles in Mem    # of FUs"<<endl;
	cout<<"Integer Adder\t  "<<Integer_Adder::num_RS<<"\t\t"<<Integer_Adder::cycles_EX<<"\t\t"<<Integer_Adder::cycles_MEM<<"\t\t"<<Integer_Adder::num_FU<<endl;
	cout<<"FP Adder\t  "<<FP_Adder::num_RS<<"\t\t"<<FP_Adder::cycles_EX<<"\t\t"<<FP_Adder::cycles_MEM<<"\t\t"<<FP_Adder::num_FU<<endl;
	cout<<"FP Multiplier\t  "<<FP_Multiplier::num_RS<<"\t\t"<<FP_Multiplier::cycles_EX<<"\t\t"<<FP_Multiplier::cycles_MEM<<"\t\t"<<FP_Multiplier::num_FU<<endl;
	cout<<"L/S Unit\t  "<<LS_Unit::num_LSQ<<"\t\t"<<LS_Unit::cycles_EX<<"\t\t"<<LS_Unit::cycles_MEM<<"\t\t"<<LS_Unit::num_FU<<endl<<endl;
	cout<<"ROB entries = "<<ROB_entries<<endl<<endl;
	for (int i=0; i<32; i++)
		if (R[i] != 0)
			cout << "R" << i <<" = " << R[i] << "\n";
	cout<<endl;

	for (int i=0; i<32; i++)
		if (F[i] != 0)
			cout << "F" << i <<" = " << F[i] << "\n";
	cout<<endl;

	for (int i=0; i<256; i++)
		if (Mem[i] != 0)
			cout << "Mem[" << i <<"] = " << Mem[i] << "\n";
	cout << endl << "Continue? (y/n)";

	char input;
	cin >> input;
	if (input != 'y' && input != 'Y')
		exit(0);


	ReOrderBuffer* ROB = new ReOrderBuffer[ROB_entries];

	Integer_Adder* IntAddFU = new Integer_Adder[Integer_Adder::num_FU];
	for (int i=0; i<Integer_Adder::num_FU; i++)
		IntAddFU[i].Initialize_RS();

	FP_Adder* FPAddFU = new FP_Adder[FP_Adder::num_FU];
	for (int i=0; i<FP_Adder::num_FU; i++)
		FPAddFU[i].Initialize_RS();

	FP_Multiplier* FPMulFU = new FP_Multiplier[FP_Multiplier::num_FU];
	for (int i=0; i<FP_Multiplier::num_FU; i++)
		FPMulFU[i].Initialize_RS();

	LS_Unit* LSFU = new LS_Unit[LS_Unit::num_LSQ];
	for (int i=0; i<LS_Unit::num_FU; i++)
		LSFU[i].Initialize_LSQ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////NOW WE HAVE THE DATA////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////





	getch();
	return 0;
}