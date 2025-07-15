🧠 Mastermind Game on Raspberry Pi (F28HS Systems Programming Coursework)

📌 Overview

This project is an embedded systems implementation of the Mastermind board game, developed in C and ARM Assembly, running on a Raspberry Pi 2/3/4 (not 5). The program simulates the logic of the classic code-breaking game using low-level GPIO control of LEDs, a button, and an LCD display.
This coursework was submitted for F28HS: Hardware-Software Interface at Heriot-Watt University as part of a 60% weighted assessment.
________________________________________
🎯 Objective

To gain hands-on experience with:

•	Direct interaction between embedded hardware and external devices

•	Systems-level programming using C and ARM assembler

•	Timers, GPIO control, hardware resource management

•	Design decisions relevant to performance and resource efficiency

________________________________________
🛠️ Hardware Components

•	Raspberry Pi 2/3/4 (not 5)

•	Breadboard

•	2 LEDs

  o	Green LED (Data) → GPIO 26

  o	Red LED (Control) → GPIO 5

•	Push Button → GPIO 19

•	16x2 LCD Display

  o	DATA: GPIO 10, 22, 23, 27

  o	Control: GPIO 24 (EN), GPIO 25 (RS), RW to GND

  o	Contrast: Controlled via potentiometer

•	3.3V power lines, resistors, and jumpers

________________________________________
🧩 Game Description

Mastermind is a two-player game between a codekeeper (RPi) and codebreaker (user). The RPi generates a random sequence (e.g., R G G) from a set of colors (encoded as numbers). The user makes guesses via button inputs, and the system responds using LED blinks and LCD output indicating:

•	Number of exact matches (right color & position)

•	Number of approximate matches (right color, wrong position)

________________________________________
🔁 Gameplay Flow

1.	On startup, RPi blinks LEDs spelling out a greeting using the team member’s surname.

2.	A secret code is generated (or passed via command-line).

3.	Player enters a sequence using button presses (e.g., 2 presses = input "2").

4.	Input is confirmed via LED blinking:

  o	Red LED blinks once to confirm input
  
  o	Green LED echoes the number entered

5.	After full input, red LED blinks twice

6.	Feedback:

  o	Green LED blinks: exact matches
  
  o	Red LED blinks once (separator)
  
  o	Green LED blinks: approximate matches

7.	Display result on LCD

8.	Repeat until success, or max rounds reached

9.	On success: LCD shows "SUCCESS" and attempt count; LEDs blink in celebration

________________________________________
🔧 Software Details

🔹 Languages & Tools:

  •	C (GNU toolchain)
  
  •	ARM Assembler (AAPCS conventions)
  
  •	gcc, as, ld, gdb, Address Sanitizer
  
  •	Raspberry Pi OS 32-bit
  
  •	GPIO register access (manual, no external libs)

🔹 Core Files:

  •	master-mind.c: Main game logic
  
  •	mm-matches.s: Matching logic in ARM Assembly
  
  •	lcdBinary.c: GPIO control prototypes (digitalWrite, pinMode, etc.)
  
________________________________________
  🧪 Testing & Debugging

Supports command-line flags for testing and automation:

./cw2                # Default mode

./cw2 -d             # Debug mode: shows secret & guess results

./cw2 -s 123         # Set a fixed secret sequence

./cw2 -u 121 313     # Unit test: returns number of exact & approx. matches

Example unit test output:


<img width="498" height="699" alt="image" src="https://github.com/user-attachments/assets/e843e91f-27ce-4dc2-9b03-bf190a8592a5" />


💡 Key Features

•	✅ Pure C and ARM Assembler hybrid implementation

•	✅ Fully functioning hardware-controlled Mastermind game

•	✅ LCD support for displaying match results and final messages

•	✅ Command-line test support

•	✅ Custom blinking greeting using surname vowels/consonants


📸 Screenshots / Media



<img width="595" height="546" alt="image" src="https://github.com/user-attachments/assets/d18e6dd4-1594-4a4a-a7e2-64a2301c3c3b" />



📚 Learning Outcomes

•	Developed skills in embedded software-hardware integration

•	Gained practical experience with GPIO register manipulation

•	Understood time-critical embedded input/output operations

•	Learned to debug low-level code and use unit testing effectively


