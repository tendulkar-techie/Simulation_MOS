# Multiprogramming OS Simulation — Phase 1
A C++ simulation of a multiprogramming operating system implementing a virtual machine with batch job processing.

## Virtual Machine
| Component | Details |
| Memory | 100 words × 4 chars (`M[100][4]`), addresses 00–99 |
| Registers | `R[4]` (General Purpose), `IR[4]` (Instruction), `IC` (Counter), `C` (Condition flag) |
| Limits | TTL (Time Limit), TLL (Line Limit) enforced per job |
| I/O | Card Reader ← `input.txt` \| Line Printer → `output.txt` |

## Instruction Set (7 Instructions)
| Instruction | Operation |
| `GDxx` | Read one data card → `M[xx..xx+9]` |
| `PDxx` | Print `M[xx..xx+9]` to output |
| `LRxx` | `R ← M[xx]` |
| `SRxx` | `M[xx] ← R` |
| `CRxx` | Compare `R` with `M[xx]`; set flag `C` |
| `BTxx` | If `C == true` then `IC = xx` |
| `H` | Halt — terminate job |

## How It Works

1. `LOAD()` reads `input.txt` card by card — `$AMJ` initializes a job, program lines are loaded into memory word by word, `$DTA` triggers execution, `$END` moves to the next job.
2. `EXECUTEUSERPROGRAM()` runs the fetch-decode-execute cycle, checking TTL each cycle.
3. `MOS()` handles system interrupts: `SI=1` → read, `SI=2` → write (checks TLL), `SI=3` → terminate.

### Job Deck Format
$AMJjjjjttttllll    ← Job ID, Time Limit, Line Limit
<program lines>     ← Instructions as packed 4-char words
$DTA                ← Triggers execution
<data lines>        ← Read at runtime by GD
$END####

## Jobs

| Job | Program | Purpose | Output |
|---|---|---|---|
| 1 | `GD10 GD20 PD20 PD10 H` | Read two cards, print in reverse | `SECOND` / `FIRST` |
| 2 | `GD20 PD20 H` | Print single line | `OPERATING SYSTEM` |
| 3 | `GD20 PD20 H` | Print single line | `Hello World` |
| 4 | `GD40 PD40 H` | Read at memory offset 40 | `COMPUTER ENGINEERING` |
| 5 | `GD50 PD50 H` | Read at memory offset 50 | `CPU MACHINE SIMULATION` |
| 6 | `GD10 LR10 SR20 PD20 H` | Load register and store | `LOAD` |
| 7 | `GD10 GD20 LR10 CR20 BT06 PD10 H` | Compare and branch | `COMPARE BRANCH TEST` |
| 8 | `GD10 LR10 GD20 LR20 SR30 PD30 H` | Load two lines, store second | `INPU` |
| 9 | `GD10 LR10 SR20 GD30 CR20 CR30 BT09 PD20 H` | Compare two inputs | `ALPH` |
| 10 | `GD10 PD10 GD20 PD20 GD30 PD30 H` | Print pattern | `*` / `**` / `***` |

Each job's output is separated by **two blank lines**.

## Error Handling

- **Time Limit Exceeded** — printed if `TTC > TTL`
- **Line Limit Exceeded** — printed if `LLC > TLL`

