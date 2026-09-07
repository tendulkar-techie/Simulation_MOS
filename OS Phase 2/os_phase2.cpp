#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
using namespace std;

//  GLOBAL DATA STRUCTURES 

char M[300][4];          // Main memory: 300 words of 4 characters each
char IR[4];              // Instruction Register
char R[4];               // General Purpose Register
short IC;                // Instruction Counter (virtual address) - 2 bytes
bool C;                  // Toggle (comparison flag)
char PTR[4];             // Page Table Register (4 bytes) - stores base address of page table in memory

int SI;                  // Service Interrupt (1=GD, 2=PD, 3=H)
int PI;                  // Program Interrupt (1=Opcode err, 2=Operand err, 3=Page fault)
int TI;                  // Timer Interrupt (0=OK, 2=Time limit exceeded)

// Process Control Block (PCB)
struct PCB {
    int  jobID;
    int  TTL;            // Total Time Limit
    int  TLL;            // Total Line Limit
    int  TTC;            // Total Time Counter
    int  LLC;            // Line Limit Counter
} pcb;

// Page Table: index = virtual page, value = frame number (-1 = unallocated)
int pageTable[10];

// Frame allocation table: true = occupied
bool frameOccupied[30]; // 30 frames (0-29), each frame = 10 words

ifstream fin;
ofstream fout;
char buffer[200];
bool jobTerminated;      // Flag to indicate current job has been terminated

//  UTILITY FUNCTIONS 

// Randomly allocate a free frame in main memory (frames 0-29)
int allocate() {
    int freeFrames[30];
    int count = 0;
    for (int i = 0; i < 30; i++) {
        if (!frameOccupied[i]) {
            freeFrames[count++] = i;
        }
    }
    if (count == 0) return -1;
    int idx = rand() % count;
    int frame = freeFrames[idx];
    frameOccupied[frame] = true;
    return frame;
}

//  INIT 

void init() {
    // Clear entire memory
    for (int i = 0; i < 300; i++)
        for (int j = 0; j < 4; j++)
            M[i][j] = ' ';

    // Clear registers
    for (int i = 0; i < 4; i++) {
        IR[i] = ' ';
        R[i]  = ' ';
    }

    IC = 0;
    C  = false;
    SI = 0;
    PI = 0;
    TI = 0;

    // Initialize PTR register to point to page table at memory location 50
    // PTR stores "0050" meaning page table starts at M[50]
    PTR[0] = '0';
    PTR[1] = '0';
    PTR[2] = '5';
    PTR[3] = '0';
    jobTerminated = false;

    // Clear PCB
    pcb.jobID = 0;
    pcb.TTL   = 0;
    pcb.TLL   = 0;
    pcb.TTC   = 0;
    pcb.LLC   = 0;

    // Clear page table (all pages unallocated)
    for (int i = 0; i < 10; i++)
        pageTable[i] = -1;

    // Clear frame allocation table
    for (int i = 0; i < 30; i++)
        frameOccupied[i] = false;

    // Reserve frame 5 (memory locations 50-59) for the page table
    frameOccupied[5] = true;
}

// = ADDRESS MAP 

// Translate virtual address to real address using page table
// Uses PTR register to locate page table base address in memory
// Returns real address, or -1 if page fault (sets PI=3)
int addressMap(int VA) {
    int pageNum      = VA / 10;
    int displacement = VA % 10;

    // Validate page number
    if (pageNum < 0 || pageNum >= 10) {
        PI = 2; // Operand error
        return -1;
    }

    // Get page table base from PTR register
    int ptBase = (PTR[2] - '0') * 10 + (PTR[3] - '0');

    // Look up frame number from page table in memory
    int frameNum = pageTable[pageNum];

    if (frameNum == -1) {
        PI = 3; // Page fault
        return -1;
    }

    return frameNum * 10 + displacement;
}

//  TERMINATE 

void terminate(int EM) {
    jobTerminated = true;
    fout << endl;

    // Line 1: Normal or Abnormal execution
    if (EM == 0)
        fout << "Program Executed Normally" << endl;
    else
        fout << "\nProgram Executed Abnormally" << endl;

    // Line 2: Specific error type
    switch (EM) {
        case 0:  fout << "No Error — Successful Termination (Halt Instruction)" << endl; break;
        case 1:  fout << "Error: Out of Data — GD instruction found no data card" << endl; break;
        case 2:  fout << "Error: Line Limit Exceeded — LLC exceeded TLL" << endl; break;
        case 3:  fout << "Error: Time Limit Exceeded — TTC exceeded TTL" << endl; break;
        case 4:  fout << "Error: Operation Code Error — Invalid opcode encountered" << endl; break;
        case 5:  fout << "Error: Operand Error — Invalid operand in instruction" << endl; break;
        case 6:  fout << "Error: Invalid Page Fault — Page fault on non-existent page" << endl; break;
        default: fout << "Error: Unknown Error" << endl; break;
    }

    // fout << "Job ID  : " << pcb.jobID << endl;
    // fout << "IC: " << IC << "  SI: " << SI << "  TI: " << TI << "  PI: " << PI << endl;
    // fout << "TTC: " << pcb.TTC << "/" << pcb.TTL
    //      << "  LLC: " << pcb.LLC << "/" << pcb.TLL << endl;
    // fout << endl << "-------------------------------------------" << endl << endl;
}

//  READ (GD) 

// Reads a data card into a 10-word block starting at real address
void read(int realAddr) {
    if (!fin.getline(buffer, 200)) {
        terminate(1); // Out of data
        return;
    }
    // Check for control cards in data section
    if (buffer[0] == '$') {
        terminate(1); // Out of data (hit $END or another control card)
        return;
    }

    int k = 0;
    int len = strlen(buffer);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 4; j++) {
            if (k < len)
                M[realAddr + i][j] = buffer[k++];
            else
                M[realAddr + i][j] = ' ';
        }
    }
}

//  WRITE (PD) 

// Writes a 10-word block starting at real address to output
void write(int realAddr) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 4; j++)
            fout << M[realAddr + i][j];
    }
    fout << endl;
    pcb.LLC++;
}

//  MOS (Master Mode - Interrupt Handler) 

void MOS() {
    int logicalAddr = (IR[2] - '0') * 10 + (IR[3] - '0');
    int pageNum = logicalAddr / 10;

    //  TI == 0 (No time-out) 
    if (TI == 0) {
        // --- Program Interrupts ---
        if (PI == 1) {
            terminate(4); // Opcode error
            return;
        }
        if (PI == 2) {
            terminate(5); // Operand error
            return;
        }
        if (PI == 3) {
            // Page fault — check if valid
            if (pageNum < 0 || pageNum > 9) {
                terminate(6); // Invalid page fault
                return;
            }

            // Allocate a frame for the faulting page
            int frame = allocate();
            if (frame == -1) {
                terminate(6); // No free frames
                return;
            }
            pageTable[pageNum] = frame;

            // Update page table in memory block 5 (using PTR register base)
            int ptBase = (PTR[2] - '0') * 10 + (PTR[3] - '0');
            M[ptBase + pageNum][0] = ' ';
            M[ptBase + pageNum][1] = ' ';
            M[ptBase + pageNum][2] = (frame / 10) + '0';
            M[ptBase + pageNum][3] = (frame % 10) + '0';

            PI = 0;

            // Handle pending SI after resolving page fault
            if (SI == 1) {
                int ra = addressMap(logicalAddr);
                read(ra);
                SI = 0;
            } else if (SI == 2) {
                int ra = addressMap(logicalAddr);
                write(ra);
                SI = 0;
                if (pcb.LLC > pcb.TLL) {
                    terminate(2);
                    return;
                }
            } else if (SI == 3) {
                terminate(6); // Halt + page fault = invalid
                return;
            } else {
                // Page fault during instruction fetch or LR/SR/CR/BT
                // Just resolved — execution will retry
                SI = 0;
            }
            return;
        }

        // --- Service Interrupts (no PI) ---
        if (SI == 1) {
            // GD — Get Data
            int ra = addressMap(logicalAddr);
            if (PI == 3) {
                // Page fault during GD — re-enter MOS
                MOS();
                return;
            }
            read(ra);
            SI = 0;
        }
        else if (SI == 2) {
            // PD — Put Data
            int ra = addressMap(logicalAddr);
            if (PI == 3) {
                MOS();
                return;
            }
            write(ra);
            SI = 0;
            if (pcb.LLC > pcb.TLL) {
                terminate(2);
                return;
            }
        }
        else if (SI == 3) {
            // H — Halt
            terminate(0);
            return;
        }
    }
    //  TI == 2 (Time-out) 
    else if (TI == 2) {
        if (PI == 1 || PI == 2 || PI == 3) {
            terminate(3); // Time limit takes priority
            return;
        }
        // PI == 0
        if (SI == 1) {
            terminate(3);
            return;
        }
        else if (SI == 2) {
            // Write output, then terminate with time limit
            int ra = addressMap(logicalAddr);
            if (ra != -1)
                write(ra);
            terminate(3);
            return;
        }
        else if (SI == 3) {
            terminate(0); // Halt within time is normal
            return;
        }
        else {
            terminate(3);
            return;
        }
    }
}

//  EXECUTE USER PROGRAM (User Mode) 

void executeUserProgram() {
    jobTerminated = false;

    while (!jobTerminated) {
        // --- Fetch phase: translate virtual IC to real address ---
        int realAddr = addressMap(IC);

        if (PI != 0) {
            // Page fault or error during instruction fetch
            IR[0] = ' '; IR[1] = ' ';
            IR[2] = (IC / 10) + '0';
            IR[3] = (IC % 10) + '0';
            MOS();
            if (jobTerminated) break;
            // Retry fetch
            realAddr = addressMap(IC);
            if (realAddr == -1) {
                terminate(6);
                break;
            }
        }

        // Load instruction into IR
        for (int i = 0; i < 4; i++)
            IR[i] = M[realAddr][i];

        IC++;

        // Increment time counter based on instruction type:
        // GD and SR take 2 units (1 page fault handling + 1 execution)
        // All other instructions take 1 unit
        if ((IR[0] == 'G' && IR[1] == 'D') || (IR[0] == 'S' && IR[1] == 'R')) {
            pcb.TTC += 2;  // 2 units: 1 for valid page fault + 1 for execution
        } else {
            pcb.TTC += 1;  // 1 unit for execution
        }

        if (pcb.TTC > pcb.TTL) {
            TI = 2;
        }

        // Decode operand
        int operand = -1;
        bool validOperand = (IR[2] >= '0' && IR[2] <= '9' && IR[3] >= '0' && IR[3] <= '9');
        if (validOperand) {
            operand = (IR[2] - '0') * 10 + (IR[3] - '0');
        }

        // ---- Decode & Execute ----

        if (IR[0] == 'L' && IR[1] == 'R') {
            if (!validOperand) { PI = 2; MOS(); break; }
            int ra = addressMap(operand);
            if (PI != 0) { MOS(); if (jobTerminated) break; ra = addressMap(operand); }
            if (ra == -1) { terminate(6); break; }
            for (int i = 0; i < 4; i++) R[i] = M[ra][i];
            if (TI == 2) { SI = 0; PI = 0; MOS(); break; }
        }
        else if (IR[0] == 'S' && IR[1] == 'R') {
            if (!validOperand) { PI = 2; MOS(); break; }
            int ra = addressMap(operand);
            if (PI != 0) { MOS(); if (jobTerminated) break; ra = addressMap(operand); }
            if (ra == -1) { terminate(6); break; }
            for (int i = 0; i < 4; i++) M[ra][i] = R[i];
            if (TI == 2) { SI = 0; PI = 0; MOS(); break; }
        }
        else if (IR[0] == 'C' && IR[1] == 'R') {
            if (!validOperand) { PI = 2; MOS(); break; }
            int ra = addressMap(operand);
            if (PI != 0) { MOS(); if (jobTerminated) break; ra = addressMap(operand); }
            if (ra == -1) { terminate(6); break; }
            C = true;
            for (int i = 0; i < 4; i++)
                if (R[i] != M[ra][i]) C = false;
            if (TI == 2) { SI = 0; PI = 0; MOS(); break; }
        }
        else if (IR[0] == 'B' && IR[1] == 'T') {
            if (!validOperand) { PI = 2; MOS(); break; }
            if (C == true) IC = operand;
            if (TI == 2) { SI = 0; PI = 0; MOS(); break; }
        }
        else if (IR[0] == 'G' && IR[1] == 'D') {
            if (!validOperand) { PI = 2; MOS(); break; }
            SI = 1;
            MOS();
            if (jobTerminated) break;
        }
        else if (IR[0] == 'P' && IR[1] == 'D') {
            if (!validOperand) { PI = 2; MOS(); break; }
            SI = 2;
            MOS();
            if (jobTerminated) break;
        }
        else if (IR[0] == 'H') {
            SI = 3;
            MOS();
            break;
        }
        else {
            // Invalid opcode
            PI = 1;
            MOS();
            break;
        }
    }
}

//  START EXECUTION 

void startExecution() {
    IC = 0;
    executeUserProgram();
}

//  LOAD 

void load() {
    int currentPage  = -1;
    int lineInPage   = 0;
    bool programMode = false;

    while (fin.getline(buffer, 200)) {
        // Skip blank lines
        if (strlen(buffer) == 0) continue;

        if (strncmp(buffer, "$AMJ", 4) == 0) {
            // ---- $AMJ: Job control card ----
            init();

            char temp[5];
            strncpy(temp, buffer + 4, 4);  temp[4] = '\0';
            pcb.jobID = atoi(temp);
            strncpy(temp, buffer + 8, 4);  temp[4] = '\0';
            pcb.TTL = atoi(temp);
            strncpy(temp, buffer + 12, 4); temp[4] = '\0';
            pcb.TLL = atoi(temp);

            programMode  = true;
            currentPage  = -1;
            lineInPage   = 0;

            cout << "Loading Job " << pcb.jobID
                 << " (TTL=" << pcb.TTL << ", TLL=" << pcb.TLL << ")" << endl;
        }
        else if (strncmp(buffer, "$DTA", 4) == 0) {
            // ---- $DTA: Data begins, start execution ----
            programMode = false;
            startExecution();
        }
        else if (strncmp(buffer, "$END", 4) == 0) {
            // ---- $END: End of job ----
            programMode  = false;
            currentPage  = -1;
            lineInPage   = 0;
        }
        else if (programMode) {
            // ---- Program card: load instructions into memory via paging ----
            int k = 0;
            int len = strlen(buffer);

            while (k < len) {
                if (currentPage == -1 || lineInPage >= 10) {
                    currentPage++;
                    lineInPage = 0;

                    if (currentPage >= 10) {
                        cout << "Error: Program too large (>10 pages)" << endl;
                        break;
                    }

                    int frame = allocate();
                    if (frame == -1) {
                        cout << "Error: No free frames for loading" << endl;
                        break;
                    }
                    pageTable[currentPage] = frame;

                    // Store in page table block (using PTR register base)
                    int ptBase = (PTR[2] - '0') * 10 + (PTR[3] - '0');
                    M[ptBase + currentPage][0] = ' ';
                    M[ptBase + currentPage][1] = ' ';
                    M[ptBase + currentPage][2] = (frame / 10) + '0';
                    M[ptBase + currentPage][3] = (frame % 10) + '0';
                }

                int frame = pageTable[currentPage];
                int realAddr = frame * 10 + lineInPage;

                for (int j = 0; j < 4; j++) {
                    if (k < len)
                        M[realAddr][j] = buffer[k++];
                    else
                        M[realAddr][j] = ' ';
                }
                lineInPage++;
            }
        }
    }
}

//  BEGIN (Entry Point) 

void begin() {
    srand((unsigned)time(0));
    fin.open("input.txt");
    fout.open("output.txt");

    if (!fin.is_open()) {
        cout << "Error: Cannot open input.txt" << endl;
        return;
    }

    load();

    fin.close();
    fout.close();
}

//  MAIN 

int main() {
    begin();
    cout << "Execution Finished. Check output.txt for results." << endl;
    return 0;
}
