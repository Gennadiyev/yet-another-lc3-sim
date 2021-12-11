# Yet another LC-3 simulator

The project contains a singleton file `lc3sim.c` which contains all the necessary codes to compile an LC-3 simulator with basic functionalities.

The project is originally a course project (lab 3) of CS169, by **Prof. Jingwen Leng**.

## Disclaimers / Known issues

1. Part of the code is provided by TA.
2. If you're looking for a real fully-functional simulator, look [here](https://highered.mheducation.com/sites/0072467509/student_view0/lc-3_simulator.html) if you like a somewhat official toolchain of LC-3, or [here](https://wchargin.com/lc3web/) for a browser preview, or use [Calysto-LC3](https://github.com/Calysto/calysto_lc3) if you love python and jupyter notebook. **NEVER use this code for anything formal**.
3. The program **only compiles on Linux / Unix systems** due to the usage of `<termios.h>` and `<unistd.h>`. The two libraries are used to fulfill `TRAP` calls from LC-3 (specifically, `IN` and `GETC`).
4. The program **have flawed implementation of `RTI`** because it does **not support supervisor mode**.
5. The program may be **misusing supervisor stack**.

## Features

Aside from what's specified in the lab requirements, the simulator additionally supports:

- `TRAP` routines  
  Fully functional `TRAP` routines with `GETC`, `IN`, `OUT`, `PUTS`, `PUTSP`, `HALT` support.

## Building

The program can be built using the following command:

```bash
gcc -std=c99 -O2 -o simulator lc3sim.c
```

## Usage

The program can be fed with a special `isaprogram` input, which is typically formatted as follows:

```
0x3000
0xE006
0x1020
0xF022
0x1021
0x6200
0x0BFB
0xF025
0x0048
0x0065
0x006C
0x006C
0x006F
0x0020
0x004B
0x0075
0x006E
0x0021
0x0000
```

The above code can be generated using:

```bash
lc3as hello_kun.asm # Compile test assembly code
hexdump -v -e ' "0x" 2/1 "%02X" "\n" ' hello_kun.obj | tee hello_kun.isaprogram # Output isaprogram
```

The built simulator executable can be used as follows:

```bash
./simulator hello_kun.isaprogram
```

## Acknowledgements

- **Prof. Jingwen Leng**
- TA: **Zhihui Zhang**
- TA: **Le Guan**
- **Chengyu Yang** for providing `lab2.asm` as a test case ~~because I forget to back it up~~.
