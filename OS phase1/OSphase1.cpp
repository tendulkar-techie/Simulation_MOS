#include<iostream> 
#include<fstream> 
#include<cstring> 
#include<cstdlib> 
using namespace std; 
char M[100][4]; 
char IR[4];          
char R[4];           
int IC;              
int SI;              
bool C;              
int JobID; 
int TTL, TLL; 
int TTC = 0;         
int LLC = 0;         
ifstream fin("input.txt"); 
ofstream fout("output.txt"); 
char buffer[40]; 
void INIT() 
{ 
    for(int i=0;i<100;i++) 
        for(int j=0;j<4;j++) 
            M[i][j]=' '; 
    for(int i=0;i<4;i++)           
    { 
        IR[i]=' '; 
        R[i]=' '; 
    } 
 
    IC = 0; 
    C = false; 
    TTC = 0; 
    LLC = 0; 
} 
void READ() 
{ 
    fin.getline(buffer,40); 
    int loc = (IR[2]-'0')*10 + (IR[3]-'0'); 
    int k = 0; 
    for(int i=0;i<10;i++) 
    { 
        for(int j=0;j<4;j++) 
        { 
            if(buffer[k] != '\0') 
                M[loc][j] = buffer[k++]; 
        } 
        loc++; 
    } 
} 
void WRITE() 
{ 
    int loc = (IR[2]-'0')*10 + (IR[3]-'0'); 
    for(int i=0;i<10;i++) 
    { 
        for(int j=0;j<4;j++) 
            fout << M[loc][j]; 
        loc++; 
    } 
    fout << endl; 
    LLC++; 
    if(LLC > TLL) 
    { 
        fout<<"Line Limit Exceeded"<<endl; 
        fout<<endl<<endl; 
    } 
} 
void TERMINATE() 
{ 
    fout<<endl<<endl; 
} 
 
 
void MOS() 
{ 
    switch(SI) 
    { 
        case 1: 
            READ(); 
            break; 
        case 2: 
            WRITE(); 
            break; 
        case 3: 
            TERMINATE(); 
            break; 
    } 
    SI = 0; 
} 
void EXECUTEUSERPROGRAM() 
{ 
    while(true) 
    { 
        TTC++; 
        if(TTC > TTL) 
        { 
            fout<<"Time Limit Exceeded"<<endl; 
            TERMINATE(); 
            break; 
        } 
        for(int i=0;i<4;i++) 
            IR[i] = M[IC][i]; 
        IC++; 
        int loc = (IR[2]-'0')*10 + (IR[3]-'0'); 
        if(IR[0]=='L' && IR[1]=='R') 
        { 
            for(int i=0;i<4;i++) 
                R[i] = M[loc][i]; 
        } 
        else if(IR[0]=='S' && IR[1]=='R') 
        { 
            for(int i=0;i<4;i++) 
                M[loc][i] = R[i]; 
        } 
 
 
        else if(IR[0]=='C' && IR[1]=='R') 
        { 
            C = true; 
 
            for(int i=0;i<4;i++) 
                if(R[i] != M[loc][i]) 
                    C = false; 
        } 
        else if(IR[0]=='B' && IR[1]=='T') 
        { 
            if(C == true) 
                IC = loc; 
        } 
        else if(IR[0]=='G' && IR[1]=='D') 
        { 
            SI = 1; 
            MOS(); 
        } 
        else if(IR[0]=='P' && IR[1]=='D') 
        { 
            SI = 2; 
            MOS(); 
        } 
        else if(IR[0]=='H') 
        { 
            SI = 3; 
            MOS(); 
            break; 
        } 
    } 
} 
void STARTEXECUTION() 
{ 
    IC = 0; 
    EXECUTEUSERPROGRAM(); 
} 
void LOAD() 
{ 
    int m = 0; 
    while(!fin.eof()) 
    { 
        fin.getline(buffer,40); 
 
        if(strncmp(buffer,"$AMJ",4)==0) 
        { 
            INIT(); 
            char temp[5]; 
            strncpy(temp, buffer+4,4); 
            temp[4]='\0'; 
            JobID = atoi(temp); 
            strncpy(temp, buffer+8,4); 
            temp[4]='\0'; 
            TTL = atoi(temp); 
            strncpy(temp, buffer+12,4); 
            temp[4]='\0'; 
            TLL = atoi(temp); 
        } 
        else if(strncmp(buffer,"$DTA",4)==0) 
        { 
            STARTEXECUTION(); 
        } 
        else if(strncmp(buffer,"$END",4)==0) 
        { 
            m = 0; 
            continue; 
        } 
        else 
        { 
            int k = 0; 
            for(int i=0;i<strlen(buffer);i++) 
            { 
                M[m][k] = buffer[i]; 
                k++; 
                if(k == 4) 
                { 
                    m++; 
                    k = 0; 
                } 
            } 
            if(k != 0) 
                m++; 
        } 
    } 
} 
 
 
int main() 
{    
    LOAD(); 
    fin.close(); 
    fout.close(); 
    cout<<"Execution Finished"<<endl; 
    return 0; 
}
