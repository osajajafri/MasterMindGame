/*
 * MasterMind implementation: template; see comments below on which parts need to be completed
 * CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
 * This repo: https://gitlab-student.macs.hw.ac.uk/f28hs-2021-22/f28hs-2021-22-staff/f28hs-2021-22-cwk2-sys

 * Compile: 
 gcc -c -o lcdBinary.o lcdBinary.c
 gcc -c -o master-mind.o master-mind.c
 gcc -o master-mind master-mind.o lcdBinary.o
 * Run:     
 sudo ./master-mind

 OR use the Makefile to build
 > make all
 and run
 > make run
 and test
 > make test

 ***********************************************************************
 * The Low-level interface to LED, button, and LCD is based on:
 * wiringPi libraries by
 * Copyright (c) 2012-2013 Gordon Henderson.
 ***********************************************************************
 * See:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
*/

/* ======================================================= */
/* SECTION: includes                                       */
/* ------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h> //Addition
#include <ctype.h> //Addition

/* --------------------------------------------------------------------------- */
/* Config settings */
/* you can use CPP flags to e.g. print extra debugging messages */
/* or switch between different versions of the code e.g. digitalWrite() in Assembler */
#define DEBUG
#undef ASM_CODE

// =======================================================
// Tunables
// PINs (based on BCM numbering)
// For wiring see CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
// GPIO pin for green LED
#define LED 26
// GPIO pin for red LED
#define LED2 5
// GPIO pin for button
#define BUTTON 19
// =======================================================
// delay for loop iterations (mainly), in ms
// in mili-seconds: 0.2s
#define DELAY   200
// in micro-seconds: 3s
#define TIMEOUT 3000000
// =======================================================
// APP constants   ---------------------------------
// number of colours and length of the sequence
#define COLS 3
#define SEQL 3
// =======================================================

// generic constants

#ifndef	TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT			 0
#define	OUTPUT			 1

#define	LOW			 0
#define	HIGH			 1


// =======================================================
// Wiring (see inlined initialisation routine)

#define STRB_PIN 24
#define RS_PIN   25
#define DATA0_PIN 23
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22

/* ======================================================= */
/* SECTION: constants and prototypes                       */
/* ------------------------------------------------------- */

// =======================================================
// char data for the CGRAM, i.e. defining new characters for the display

static unsigned char newChar [8] = 
{
  0b11111,
  0b10001,
  0b10001,
  0b10101,
  0b11111,
  0b10001,
  0b10001,
  0b11111,
} ;

/* Constants */

static const int colors = COLS;
static const int seqlen = SEQL;

static char* color_names[] = { "red", "green", "blue" };

static int* theSeq = NULL;

static int *seq1, *seq2, *cpy1, *cpy2;

/* --------------------------------------------------------------------------- */

// data structure holding data on the representation of the LCD
struct lcdDataStruct
{
  int bits, rows, cols ;
  int rsPin, strbPin ;
  int dataPins [8] ;
  int cx, cy ;
} ;

static int lcdControl ;

/* ***************************************************************************** */
/* INLINED fcts from wiringPi/devLib/lcd.c: */
// HD44780U Commands (see Fig 11, p28 of the Hitachi HD44780U datasheet)

#define	LCD_CLEAR	0x01
#define	LCD_HOME	0x02
#define	LCD_ENTRY	0x04
#define	LCD_CTRL	0x08
#define	LCD_CDSHIFT	0x10
#define	LCD_FUNC	0x20
#define	LCD_CGRAM	0x40
#define	LCD_DGRAM	0x80

// Bits in the entry register

#define	LCD_ENTRY_SH		0x01
#define	LCD_ENTRY_ID		0x02

// Bits in the control register

#define	LCD_BLINK_CTRL		0x01
#define	LCD_CURSOR_CTRL		0x02
#define	LCD_DISPLAY_CTRL	0x04

// Bits in the function register

#define	LCD_FUNC_F	0x04
#define	LCD_FUNC_N	0x08
#define	LCD_FUNC_DL	0x10

#define	LCD_CDSHIFT_RL	0x04

// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices

#define	PI_GPIO_MASK	(0xFFFFFFC0)

static unsigned int gpiobase ;
static uint32_t *gpio ;

static int timed_out = 0;

/* ------------------------------------------------------- */
// misc prototypes

int failure (int fatal, const char *message, ...);
void waitForEnter (void);
void waitForButton (uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: hardware interface (LED, button, LCD display)  */
/* ------------------------------------------------------- */
/* low-level interface to the hardware */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Either put them in a separate file, lcdBinary.c, and use   */
/* inline Assembler there, or use a standalone Assembler file */
/* You can also directly implement them here (inline Asm).    */
/* ********************************************************** */

/* These are just prototypes; you need to complete the code for each function */

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
void digitalWrite (uint32_t *gpio, int pin, int value);

/* set the @mode@ of a GPIO @pin@ to INPUT or OUTPUT; @gpio@ is the mmaped GPIO base address */
void pinMode(uint32_t *gpio, int pin, int mode);

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
/* can use digitalWrite(), depending on your implementation */
void writeLED(uint32_t *gpio, int led, int value);

/* read a @value@ (LOW or HIGH) from pin number @pin@ (a button device); @gpio@ is the mmaped GPIO base address */
int readButton(uint32_t *gpio, int button);

/* wait for a button input on pin number @button@; @gpio@ is the mmaped GPIO base address */
/* can use readButton(), depending on your implementation */
void waitForButton (uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: game logic                                     */
/* ------------------------------------------------------- */
/* AUX fcts of the game logic */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* initialise the secret sequence; by default it should be a random sequence */
void initSeq() {
  /* ***  COMPLETE the code here  ***  */
	theSeq = (int *)malloc(seqlen * sizeof(int));//Allocate memory for theSeq

	srand(time(0));// Seed the random number generator

	for (int i =0;i< SEQL;i++){// Loop through theSeq array
	    theSeq[i]=rand()% COLS + 1;// Assign a random number in range [1, cols]
	  }
}

/* display the sequence on the terminal window, using the format from the sample run in the spec */
void showSeq(int *seq) {
  /* ***  COMPLETE the code here  ***  */
	int i;
	  fprintf(stdout, "SECRET: ");// Print "SECRET: "
	  for (i=0;i<SEQL;++i){//Loops through seq arr
	    fprintf(stdout, "%d ",seq[i]); //Prints each digit of the sequence
	  }
	  fprintf(stdout, "\n");
}

#define NAN1 8
#define NAN2 9

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
// int /* or int* */ countMatches(int *seq1, int *seq2) {
//   /* ***  COMPLETE the code here  ***  */
// 	int i,j,k,m;

// 	  int *check = (int *)malloc(seqlen * sizeof(int)); // Allocate memory for check array
// 	  int exact=0; //Initialize exact and approx match counters
// 	  int approx=0;

//     // Initialize check array to 0
// 	  for(i=0;i<SEQL;i++)
// 	      check[i]=0;

//     // Loop to check for exact matches
// 	  for(k=0; k< SEQL; k++){

// 	      if(seq1[k]==seq2[k]){ //Check if elements match at the same position
// 	        check[k]=1; //Mark as seen
// 	        exact++; //Increment exact matches
// 	      }
// 	  }
//     // Loop to check for approximate matches
// 	  for(j=0;j<SEQL;j++){
// 	    if (seq2[j] == seq1[j]) // Skip if exact match already found
// 	          {
// 	              continue;
// 	          }

// 	    else {
// 	      for(m=0;m<SEQL;m++){

// 	        if (!check[m] && m != j && seq2[j] == seq1[m]) {// Check if element hasn't been seen and matches elsewhere

// 	          approx++; //Increment approximate matches
// 	          check[m] = 1; //Mark as seen
// 	          break; //Exit the loop after finding match
// 	      }

// 	    }
// 	  }
// 	}

// 	  free(check);// Free the allocated memory for the check array
// 	  int ret = concat(exact, approx);//Concatenate exact and approximate matches using the helper function
// 	  return ret; //Return the combined result
// }

extern int matches(int *seq1, int *seq2); //Declaring assembly function

int countMatches(int *seq1, int *seq2) // Replace countMatches with an assembly function call  
{
  return matches(seq1, seq2);
}



/* show the results from calling countMatches on seq1 and seq1 */
void showMatches(int /* or int* */ code, /* only for debugging */ int *seq1, int *seq2, /* optional, to control layout */ int lcd_format) {
  /* ***  COMPLETE the code here  ***  */
	int index = 0;

	  // Temporary array to store encoded values
	  int *temp = (int *)malloc(2 * sizeof(int));

	  // While loop to split code digits into array
	  while (code != 0 && index < seqlen)
	  {
	      temp[index] = code % 10; // Extracts last digit (approx matches)
	      ++index;
	      code /= 10; // Extracts first digit (exact matches)
	  }

	  //Store the digits in the corresponding integer values
	  int approx = temp[0];
	  int exact = temp[1];

	  // Print exact and approximate values to terminal (debugging output)
	  printf("%d exact\n", exact);
	  printf("%d approximate\n", approx);

	  // Free temp array
	  free(temp);
}

/* parse an integer value as a list of digits, and put them into @seq@ */
/* needed for processing command-line with options -s or -u            */
void readSeq(int *seq, int val) {
  /* ***  COMPLETE the code here  ***  */
	int i = 0;
	while (val != 0 && i < seqlen) // Loop to read digits of the value into the seq array
	{
	    seq[i] = val % 10; //Store last digit of val into seq
	    ++i;
	    val /= 10; //Remove last digit from val
	}

  // If the number has fewer digits than seqlen, fill the remaining positions with 0 
	reverse(seq, 0, seqlen - 1);
}

/* read a guess sequence fron stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
int *readNum(int max) {
  /* ***  COMPLETE the code here  ***  */
	int index = 0;
	  int *arr = (int *)malloc(seqlen * sizeof(int)); // Allocate memory for array to store digits of max
	  if (!arr)
	      return NULL; //Return NULL if memory allocation fails

	    while (max != 0 && index < SEQL){// Split max into its digits and store them in the array
	      arr[index] = max % 10;// add last digit of max to arr array
	      ++index;;// increment loop index
	      max /= 10; // remove last digit from max
	    }

	    reverse(arr, 0, SEQL - 1);// Reverse the array to correct order
	    //Return the array
	  return arr;
}


/* Helper Functions */
int concat(int x, int y){ // Helper function to combine two integers
    int temp = y;
    do //Shift digits of y to the right until alla digits are moved
    {
        x *= 10; //Shift x by one digit to the left
        y /= 10; //Remove the last digit from y

    } while (y != 0); //repeat until all digits of y are processed
    return x + temp; //Combine the modified x and original y
}
/* Helper function to reverse array from start to end */
void reverse(int arr[], int start, int end){

    int temp;
    while (start < end) //Loop until start and end meet
    {
        temp = arr[start]; //Swap the elements at start and end positions
        arr[start] = arr[end];
        arr[end] = temp;
        start++; //Move start index forward
        end--; //Move end index backward
    }
}

/* Helper function to display  */
void showGuess(int colorNum, struct lcdDataStruct *lcd){

    switch (colorNum)//Check the color number
    {
    case 1:
        lcdPuts(lcd, " R");//Display Red on LCD
        fprintf(stderr, "Input : 1\n");//Output in terminal 1
        break;
    case 2:
        lcdPuts(lcd, " G");//Display Green on LCD
        fprintf(stderr, "Input : 2\n");//Output in terminal 2
        break;
    case 3:
        lcdPuts(lcd, " B");//Display Blue on LCD
        fprintf(stderr, "Input : 3\n");//Output in terminal 3
        break;
    }
}

/* Helper function to show user guess on LCD */
void showMatchesLCD(int code, struct lcdDataStruct *lcd){
    // Variable as index value
    int index = 0;

    // Temporary array to store encoded values
    int *temp = (int *)malloc(2 * sizeof(int));

    char *text = (char *)malloc(3 * sizeof(char)); //For holding string representation of digits

    // While loop to split code into digits and store in temp array
    /* If code is not 0 and the current index is less than the secret sequence length */ 
    while (code != 0 && index < seqlen)
    {
        temp[index] = code % 10; //Extract last digit
        index++;
        code /= 10; //Remove last digit from code
    }

    // Assign exact and approx values
    int approx = temp[0];
    int exact = temp[1];

    // Print out Exact and approximate values to terminal
     printf("Exact: %d    Approximate: %d\n\n", exact, approx);


    // Free temp array
    free(temp);

    //Display on LCD
    lcdPosition(lcd, 0, 1);
    lcdPuts(lcd, "Exact: ");
    sprintf(text, "%d", exact);
    lcdPosition(lcd, 6, 1);
    lcdPuts(lcd, text);
    blinkN(gpio, LED, exact);// Blink LED based on exact matches
    blinkN(gpio, LED2, 1);// Blink another LED after showing exact matches

    lcdPosition(lcd, 8, 1);
    lcdPuts(lcd, "Approx: ");
    sprintf(text, "%d", approx);
    lcdPosition(lcd, 15, 1);
    lcdPuts(lcd, text);
    blinkN(gpio, LED, approx);// Blink LED based on approximate matches
}




// Function to blink LEDs for Greeting message
void blinkGreetings(uint32_t *gpio, int pinLED, int pin2LED2, const char *surname) {
  // Blink LEDs based on letter type in first 5 letters of surname
  for (int i = 0; i < 5 && surname[i] != '\0'; i++) {
      char c = tolower(surname[i]);
      if (isalpha(c)) {
          if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {// Vowel - blink green LED
              writeLED(gpio, pinLED, HIGH);
              delay(700); //wait 700ms
              writeLED(gpio, pinLED, LOW);
          } else {// Consonant - blink red LED
              writeLED(gpio, pin2LED2, HIGH);
              delay(700); //wait 700ms
              writeLED(gpio, pin2LED2, LOW);
          }
          delay(700); // Wait 700ms between blinks
      }
  }
}

/* *********************************************************************************************************** */

/* ======================================================= */
/* SECTION: TIMER code                                     */
/* ------------------------------------------------------- */
/* TIMER code */

/* timestamps needed to implement a time-out mechanism */
static uint64_t startT, stopT;

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* you may need this function in timer_handler() below  */
/* use the libc fct gettimeofday() to implement it      */
uint64_t timeInMicroseconds(){
  /* ***  COMPLETE the code here  ***  */
	 struct timeval tv;
	  gettimeofday(&tv,NULL); //Get current time in seconds and microseconds
	  return (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec; //Convert to microseconds
}

/* this should be the callback, triggered via an interval timer, */
/* that is set-up through a call to sigaction() in the main fct. */
void timer_handler (int signum) {
  /* ***  COMPLETE the code here  ***  */
	 static int count = 0; //Keep track of the number of timer expirations
	  stopT= timeInMicroseconds(); // Get the current time
	  count ++; //Increment count 
	  fprintf(stderr, "Timer expired %d times. Time took: %f\n", count, (stopT - startT) / 1000000.0); // Print count and elapsed time
	  timed_out = 1; // Set timeout marked
}


/* initialise time-stamps, setup an interval timer, and install the timer_handler callback */
void initITimer(uint64_t timeout){
  /* ***  COMPLETE the code here  ***  */
	  struct sigaction sa;
	  struct itimerval timer;

	  memset(&sa, 0, sizeof(sa)); // Clear memory for sa struct
	  sa.sa_handler = &timer_handler; // Set signal handler
	  sigaction(SIGALRM, &sa, NULL);// Register handler for SIGALRM
  

	  /* Set up non recurring timer */
	  timer.it_value.tv_sec = timeout;//Set timeout value  
	  timer.it_value.tv_usec = 0;// Set microseconds to 0
	  timer.it_interval.tv_sec = 0; //No repeat interval
	  timer.it_interval.tv_usec = 0;
	  setitimer(ITIMER_REAL, &timer, NULL); //Start timer

	  startT = timeInMicroseconds();//Record the start time
}

/* ======================================================= */
/* SECTION: Aux function                                   */
/* ------------------------------------------------------- */
/* misc aux functions */

int failure (int fatal, const char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  if (!fatal) //  && wiringPiReturnCodes)
    return -1 ;

  va_start (argp, message) ;
  vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  fprintf (stderr, "%s", buffer) ;
  exit (EXIT_FAILURE) ;

  return 0 ;
}

/*
 * waitForEnter:
 *********************************************************************************
 */

void waitForEnter (void)
{
  printf ("Press ENTER to continue: ") ;
  (void)fgetc (stdin) ;
}

/*
 * delay:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void delay (unsigned int howLong)
{
  struct timespec sleeper, dummy ;

  sleeper.tv_sec  = (time_t)(howLong / 1000) ;
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

  nanosleep (&sleeper, &dummy) ;
}

/* From wiringPi code; comment by Gordon Henderson
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicroseconds (unsigned int howLong)
{
  struct timespec sleeper ;
  unsigned int uSecs = howLong % 1000000 ;
  unsigned int wSecs = howLong / 1000000 ;

  /**/ if (howLong ==   0)
    return ;
#if 0
  else if (howLong  < 100)
    delayMicrosecondsHard (howLong) ;
#endif
  else
  {
    sleeper.tv_sec  = wSecs ;
    sleeper.tv_nsec = (long)(uSecs * 1000L) ;
    nanosleep (&sleeper, NULL) ;
  }
}

/* ======================================================= */
/* SECTION: LCD functions                                  */
/* ------------------------------------------------------- */
/* medium-level interface functions (all in C) */

/* from wiringPi:
 * strobe:
 *	Toggle the strobe (Really the "E") pin to the device.
 *	According to the docs, data is latched on the falling edge.
 *********************************************************************************
 */

void strobe (const struct lcdDataStruct *lcd)
{

  // Note timing changes for new version of delayMicroseconds ()
  digitalWrite (gpio, lcd->strbPin, 1) ; delayMicroseconds (50) ;
  digitalWrite (gpio, lcd->strbPin, 0) ; delayMicroseconds (50) ;
}

/*
 * sentDataCmd:
 *	Send an data or command byte to the display.
 *********************************************************************************
 */

void sendDataCmd (const struct lcdDataStruct *lcd, unsigned char data)
{
  register unsigned char myData = data ;
  unsigned char          i, d4 ;

  if (lcd->bits == 4)
  {
    d4 = (myData >> 4) & 0x0F;
    for (i = 0 ; i < 4 ; ++i)
    {
      digitalWrite (gpio, lcd->dataPins [i], (d4 & 1)) ;
      d4 >>= 1 ;
    }
    strobe (lcd) ;

    d4 = myData & 0x0F ;
    for (i = 0 ; i < 4 ; ++i)
    {
      digitalWrite (gpio, lcd->dataPins [i], (d4 & 1)) ;
      d4 >>= 1 ;
    }
  }
  else
  {
    for (i = 0 ; i < 8 ; ++i)
    {
      digitalWrite (gpio, lcd->dataPins [i], (myData & 1)) ;
      myData >>= 1 ;
    }
  }
  strobe (lcd) ;
}

/*
 * lcdPutCommand:
 *	Send a command byte to the display
 *********************************************************************************
 */

void lcdPutCommand (const struct lcdDataStruct *lcd, unsigned char command)
{
#ifdef DEBUG
  //fprintf(stderr, "lcdPutCommand: digitalWrite(%d,%d) and sendDataCmd(%d,%d)\n", lcd->rsPin,   0, lcd, command);
#endif
  digitalWrite (gpio, lcd->rsPin,   0) ;
  sendDataCmd  (lcd, command) ;
  delay (2) ;
}

void lcdPut4Command (const struct lcdDataStruct *lcd, unsigned char command)
{
  register unsigned char myCommand = command ;
  register unsigned char i ;

  digitalWrite (gpio, lcd->rsPin,   0) ;

  for (i = 0 ; i < 4 ; ++i)
  {
    digitalWrite (gpio, lcd->dataPins [i], (myCommand & 1)) ;
    myCommand >>= 1 ;
  }
  strobe (lcd) ;
}

/*
 * lcdHome: lcdClear:
 *	Home the cursor or clear the screen.
 *********************************************************************************
 */

void lcdHome (struct lcdDataStruct *lcd)
{
#ifdef DEBUG
  //fprintf(stderr, "lcdHome: lcdPutCommand(%d,%d)\n", lcd, LCD_HOME);
#endif
  lcdPutCommand (lcd, LCD_HOME) ;
  lcd->cx = lcd->cy = 0 ;
  delay (5) ;
}

void lcdClear (struct lcdDataStruct *lcd)
{
#ifdef DEBUG
 // fprintf(stderr, "lcdClear: lcdPutCommand(%d,%d) and lcdPutCommand(%d,%d)\n", lcd, LCD_CLEAR, lcd, LCD_HOME);
#endif
  lcdPutCommand (lcd, LCD_CLEAR) ;
  lcdPutCommand (lcd, LCD_HOME) ;
  lcd->cx = lcd->cy = 0 ;
  delay (5) ;
}

/*
 * lcdPosition:
 *	Update the position of the cursor on the display.
 *	Ignore invalid locations.
 *********************************************************************************
 */

void lcdPosition (struct lcdDataStruct *lcd, int x, int y)
{
  // struct lcdDataStruct *lcd = lcds [fd] ;

  if ((x > lcd->cols) || (x < 0))
    return ;
  if ((y > lcd->rows) || (y < 0))
    return ;

  lcdPutCommand (lcd, x + (LCD_DGRAM | (y>0 ? 0x40 : 0x00)  /* rowOff [y] */  )) ;

  lcd->cx = x ;
  lcd->cy = y ;
}



/*
 * lcdDisplay: lcdCursor: lcdCursorBlink:
 *	Turn the display, cursor, cursor blinking on/off
 *********************************************************************************
 */

void lcdDisplay (struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |=  LCD_DISPLAY_CTRL ;
  else
    lcdControl &= ~LCD_DISPLAY_CTRL ;

  lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}

void lcdCursor (struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |=  LCD_CURSOR_CTRL ;
  else
    lcdControl &= ~LCD_CURSOR_CTRL ;

  lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}

void lcdCursorBlink (struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |=  LCD_BLINK_CTRL ;
  else
    lcdControl &= ~LCD_BLINK_CTRL ;

  lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}

/*
 * lcdPutchar:
 *	Send a data byte to be displayed on the display. We implement a very
 *	simple terminal here - with line wrapping, but no scrolling. Yet.
 *********************************************************************************
 */

void lcdPutchar (struct lcdDataStruct *lcd, unsigned char data)
{
  digitalWrite (gpio, lcd->rsPin, 1) ;
  sendDataCmd  (lcd, data) ;

  if (++lcd->cx == lcd->cols)
  {
    lcd->cx = 0 ;
    if (++lcd->cy == lcd->rows)
      lcd->cy = 0 ;
    
    // TODO: inline computation of address and eliminate rowOff
    lcdPutCommand (lcd, lcd->cx + (LCD_DGRAM | (lcd->cy>0 ? 0x40 : 0x00)   /* rowOff [lcd->cy] */  )) ;
  }
}


/*
 * lcdPuts:
 *	Send a string to be displayed on the display
 *********************************************************************************
 */

void lcdPuts (struct lcdDataStruct *lcd, const char *string)
{
  while (*string)
    lcdPutchar (lcd, *string++) ;
}

/* ======================================================= */
/* SECTION: aux functions for game logic                   */
/* ------------------------------------------------------- */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* --------------------------------------------------------------------------- */
/* interface on top of the low-level pin I/O code */

/* blink the led on pin @led@, @c@ times */
void blinkN(uint32_t *gpio, int led, int c) { 
  /* ***  COMPLETE the code here  ***  */
	  for (int i = 0; i < c; i++) // Blink LED c times
	  {
	    /* turns the led on and off with  certain delay */
	    writeLED(gpio, led, HIGH); // Turn on the LED
	    delay(700);// Wait 700ms
	    writeLED(gpio, led, LOW); // Turn off the LED 
	    delay(700);// Wait 700ms
	  }
}

/* ======================================================= */
/* SECTION: main fct                                       */
/* ------------------------------------------------------- */

int main (int argc, char *argv[])
{ // this is just a suggestion of some variable that you may want to use
  struct lcdDataStruct *lcd ;
  int bits, rows, cols ;
  unsigned char func ;

  int num = 6;
  int count = 0;
  int found = 0, attempts = 0, i, j, code;
  int c, d, buttonPressed, rel, foo;
  int *attSeq;

  int pinLED = LED, pin2LED2 = LED2, pinButton = BUTTON;
  int fSel, shift, pin,  clrOff, setOff, off, res;
  int fd ;

  int  exact, contained;
  char str1[32];
  char str2[32];
  
  struct timeval t1, t2 ;
  int t ;

  char buf [32] ;
  
  const char *surname = "Jafri";

  // variables for command-line processing
  char str_in[20], str[20] = "some text";
  int verbose = 0, debug = 0, help = 0, opt_m = 0, opt_n = 0, opt_s = 0, unit_test = 0, res_matches = 0;
  
  char *userInput; // Declare a pointer to store the dynamically allocated string
  userInput = (char *)malloc(seqlen * sizeof(char));
  // -------------------------------------------------------
  // process command-line arguments

  // see: man 3 getopt for docu and an example of command line parsing
  { // see the CW spec for the intended meaning of these options
    int opt;
    while ((opt = getopt(argc, argv, "hvdus:")) != -1) {
      switch (opt) {
      case 'v':
	verbose = 1;
	break;
      case 'h':
	help = 1;
	break;
      case 'd':
	debug = 1;
	break;
      case 'u':
	unit_test = 1;
	break;
      case 's':
	opt_s = atoi(optarg); 
	break;
      default: /* '?' */
	fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
	exit(EXIT_FAILURE);
      }
    }
  }

  if (help) {
    fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n"); 
    fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n"); 
    fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n"); 
    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
    exit(EXIT_SUCCESS);
  }
  
  if (unit_test && optind >= argc-1) {
    fprintf(stderr, "Expected 2 arguments after option -u\n");
    exit(EXIT_FAILURE);
  }

  if (verbose && unit_test) {
    printf("1st argument = %s\n", argv[optind]);
    printf("2nd argument = %s\n", argv[optind+1]);
  }

  if (verbose) {
    fprintf(stdout, "Settings for running the program\n");
    fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
    fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
    fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
    if (opt_s)  fprintf(stdout, "Secret sequence set to %d\n", opt_s);
  }

  seq1 = (int*)malloc(seqlen*sizeof(int));
  seq2 = (int*)malloc(seqlen*sizeof(int));
  cpy1 = (int*)malloc(seqlen*sizeof(int));
  cpy2 = (int*)malloc(seqlen*sizeof(int));

  // check for -u option, and if so run a unit test on the matching function
  if (unit_test && argc > optind+1) { // more arguments to process; only needed with -u 
    strcpy(str_in, argv[optind]);
    opt_m = atoi(str_in);
    strcpy(str_in, argv[optind+1]);
    opt_n = atoi(str_in);
    // CALL a test-matches function; see testm.c for an example implementation
    readSeq(seq1, opt_m); // turn the integer number into a sequence of numbers
    readSeq(seq2, opt_n); // turn the integer number into a sequence of numbers
    if (verbose)
      fprintf(stdout, "Testing matches function with sequences %d and %d\n", opt_m, opt_n);
    res_matches = countMatches(seq1, seq2);
    showMatches(res_matches, seq1, seq2, 1);
    exit(EXIT_SUCCESS);
  } else {
    /* nothing to do here; just continue with the rest of the main fct */
  }

  if (opt_s) { // if -s option is given, use the sequence as secret sequence
    if (theSeq==NULL)
      theSeq = (int*)malloc(seqlen*sizeof(int));
    readSeq(theSeq, opt_s);
    if (verbose) {
      fprintf(stderr, "Running program with secret sequence:\n");
      showSeq(theSeq);
    }
  }
  
  // -------------------------------------------------------
  // LCD constants, hard-coded: 16x2 display, using a 4-bit connection
  bits = 4; 
  cols = 16; 
  rows = 2; 
  // -------------------------------------------------------

  printf ("Raspberry Pi LCD driver, for a %dx%d display (%d-bit wiring) \n", cols, rows, bits) ;

  if (geteuid () != 0)
    fprintf (stderr, "setup: Must be root. (Did you forget sudo?)\n") ;

  // init of guess sequence, and copies (for use in countMatches)
  attSeq = (int*) malloc(seqlen*sizeof(int));
  cpy1 = (int*)malloc(seqlen*sizeof(int));
  cpy2 = (int*)malloc(seqlen*sizeof(int));

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000 ;

  // -----------------------------------------------------------------------------
  // memory mapping 
  // Open the master /dev/memory device

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    return failure (FALSE, "setup: Unable to open /dev/mem: %s\n", strerror (errno)) ;

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
  if ((int32_t)gpio == -1)
    return failure (FALSE, "setup: mmap (GPIO) failed: %s\n", strerror (errno)) ;

  // -------------------------------------------------------
  // Configuration of LED and BUTTON

  /* ***  COMPLETE the code here  ***  */
  pinMode(gpio, pinLED, OUTPUT);   // Set pinLED as an output to control an LED  
  pinMode(gpio, pin2LED2, OUTPUT); // Set pin2LED2 as an output for another LED  
  pinMode(gpio, pinButton, INPUT); // Set pinButton as an input to read button state  
  writeLED(gpio, pin2LED2, LOW); // Ensure LED2 is initially turned off  
  writeLED(gpio, pinLED, LOW);   // Ensure LED1 is initially turned off  

  // -------------------------------------------------------
  // INLINED version of lcdInit (can only deal with one LCD attached to the RPi):
  // you can use this code as-is, but you need to implement digitalWrite() and
  // pinMode() which are called from this code
  // Create a new LCD:
  lcd = (struct lcdDataStruct *)malloc (sizeof (struct lcdDataStruct)) ;
  if (lcd == NULL)
    return -1 ;

  // hard-wired GPIO pins
  lcd->rsPin   = RS_PIN ;
  lcd->strbPin = STRB_PIN ;
  lcd->bits    = 4 ;
  lcd->rows    = rows ;  // # of rows on the display
  lcd->cols    = cols ;  // # of cols on the display
  lcd->cx      = 0 ;     // x-pos of cursor
  lcd->cy      = 0 ;     // y-pos of curosr

  lcd->dataPins [0] = DATA0_PIN ;
  lcd->dataPins [1] = DATA1_PIN ;
  lcd->dataPins [2] = DATA2_PIN ;
  lcd->dataPins [3] = DATA3_PIN ;
  // lcd->dataPins [4] = d4 ;
  // lcd->dataPins [5] = d5 ;
  // lcd->dataPins [6] = d6 ;
  // lcd->dataPins [7] = d7 ;

  // lcds [lcdFd] = lcd ;

  digitalWrite (gpio, lcd->rsPin,   0) ; pinMode (gpio, lcd->rsPin,   OUTPUT) ;
  digitalWrite (gpio, lcd->strbPin, 0) ; pinMode (gpio, lcd->strbPin, OUTPUT) ;

  for (i = 0 ; i < bits ; ++i)
  {
    digitalWrite (gpio, lcd->dataPins [i], 0) ;
    pinMode      (gpio, lcd->dataPins [i], OUTPUT) ;
  }
  delay (35) ; // mS

// Gordon Henderson's explanation of this part of the init code (from wiringPi):
// 4-bit mode?
//	OK. This is a PIG and it's not at all obvious from the documentation I had,
//	so I guess some others have worked through either with better documentation
//	or more trial and error... Anyway here goes:
//
//	It seems that the controller needs to see the FUNC command at least 3 times
//	consecutively - in 8-bit mode. If you're only using 8-bit mode, then it appears
//	that you can get away with one func-set, however I'd not rely on it...
//
//	So to set 4-bit mode, you need to send the commands one nibble at a time,
//	the same three times, but send the command to set it into 8-bit mode those
//	three times, then send a final 4th command to set it into 4-bit mode, and only
//	then can you flip the switch for the rest of the library to work in 4-bit
//	mode which sends the commands as 2 x 4-bit values.

  if (bits == 4)
  {
    func = LCD_FUNC | LCD_FUNC_DL ;			// Set 8-bit mode 3 times
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    func = LCD_FUNC ;					// 4th set: 4-bit mode
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    lcd->bits = 4 ;
  }
  else
  {
    failure(TRUE, "setup: only 4-bit connection supported\n");
    func = LCD_FUNC | LCD_FUNC_DL ;
    lcdPutCommand  (lcd, func     ) ; delay (35) ;
    lcdPutCommand  (lcd, func     ) ; delay (35) ;
    lcdPutCommand  (lcd, func     ) ; delay (35) ;
  }

  if (lcd->rows > 1)
  {
    func |= LCD_FUNC_N ;
    lcdPutCommand (lcd, func) ; delay (35) ;
  }

  // Rest of the initialisation sequence
  lcdDisplay     (lcd, TRUE) ;
  lcdCursor      (lcd, FALSE) ;
  lcdCursorBlink (lcd, FALSE) ;
  lcdClear       (lcd) ;

  lcdPutCommand (lcd, LCD_ENTRY   | LCD_ENTRY_ID) ;    // set entry mode to increment address counter after write
  lcdPutCommand (lcd, LCD_CDSHIFT | LCD_CDSHIFT_RL) ;  // set display shift to right-to-left

  // END lcdInit ------
  // -----------------------------------------------------------------------------
  // Start of game
  fprintf(stderr,"Printing welcome message on the LCD display ...\n");
  /* ***  COMPLETE the code here  ***  */
  	lcdPosition(lcd,0,0);
    lcdPuts(lcd,"Welcome to the");//Print welcome message in lcd
    lcdPosition(lcd,0,1);
    lcdPuts(lcd,"Mastermind Game");

    /*Personalized Greeting Message*/
    fprintf(stderr, "Greetings Jafri.\n");
    blinkGreetings(gpio, pinLED, pin2LED2, surname);

  /* initialise the secret sequence */
  if (!opt_s)
    initSeq();
  if (debug)
    showSeq(theSeq);

  // optionally one of these 2 calls:
  waitForEnter () ;
  lcdClear(lcd);//Clears the welcome message to allow printing of a new one
  // waitForButton (gpio, pinButton) ;


  if (debug) {//if debug mode
      printf("\n");
      printf("========================\n");
      while (attempts < 5) { //Limit 5 attempts
        attempts++; //Increment attempts 

        printf("\nGuess %d: ", attempts);
        scanf("%s", userInput); // Get user input and stores it char arr


        for (int m = 0; m < seqlen; m++) { //Map user input to sequence values
          switch (userInput[m])
          {
          case 'R':
              attSeq[m] = 1;
              break;
          case 'G':
              attSeq[m] = 2;
              break;
          case 'B':
              attSeq[m] = 3;
              break;
          default:
              attSeq[m] = 0;
          }
        }
        int sequence = countMatches(theSeq, attSeq); // Compare sequences (secret sequence and user sequence)
        if (sequence == 30) { //Checks the result returned by the countmatches function, indicating 3 exact matches and 0 approximate matches
          found = 1; //Mark sequence found
          showMatches(sequence, theSeq, attSeq, 1); //Display the Exact and Approx matches on the terminal  
          break; //Exit the loop
          }
          showMatches(sequence, theSeq, attSeq, 1); //Displays the matches if the returned result is not equal to 30
          delay(1000); //Delay before next attempt
        }
        if (found) {
          printf("\nCongratulations! You broke the code in %d attempts!\n\n", attempts);

          // Free allocated memory
          free(userInput);
          free(attSeq);
          free(theSeq);
          return 0;  //Exit program
        }
          else
          {
            printf("\nSequence not found.\nNumber of attempts: %d\n", attempts);
            printf("Better luck next time!\n");
            showSeq(theSeq); //Display the sequence if the user fails to guess within 5 attempts
            
            //Free the memory
            free(userInput);
            free(attSeq);
            free(theSeq);
            return 0; //Exit program
          }
      }


  // -----------------------------------------------------------------------------
  // +++++ main loop
  while (attempts < 5 ) {
    attempts++;
    lcdPosition(lcd, 0, 0);
         lcdPuts(lcd, "Guess:");// Print out guess on LCD

         fprintf(stderr, "\nGuess %d:\n", attempts); //Display ''Guess" on LCD
         int count = 0, num = 6;

         for (int k = 0; k < 3; k++) //Loop for 3 guesses
         {
           for (int i = 0; i < 3; i++)
           {
             waitForButton(gpio, BUTTON); // Wait for button press
             buttonPressed = readButton(gpio, BUTTON);// Read button state
             if (buttonPressed == HIGH) {
               count++; //Increment button press
               fprintf(stderr, "Button Pressed  "); //Display message when button pressed
               delay(DELAY);
               }

             if (count == 3) {//if the user presses button 3 times
               break; //Exit loop
               }
           }
           lcdPosition(lcd, num, 0);
           blinkN(gpio, pin2LED2, 1);// Blink RED LED as separator
           showGuess(count, lcd); //Displays guess on LCD using helper functions
           fprintf(stdout,"\n");
           blinkN(gpio, pinLED, count); // Blink GREEN LED to show input count
           num = num + 2; //Adjust LCD position for next input
           attSeq[k] = count; //Store guess in atteSeq arr
           count = 0;
           }
           printf("\n");// Act as separator
           blinkN(gpio,pin2LED2, 2); //Blink RED LED twice as separator
           int sequence = countMatches(theSeq, attSeq); //Compare counts between secret sequence and user sequence

           if (sequence == 30)//Check Exact 3 matches
           {
               found = 1;//Mark found 
               showMatchesLCD(sequence, lcd); //Displays matches on LCD using helper fucntion
               break;//Exit loop
           }
           showMatchesLCD(sequence, lcd);//Displays matches when not 30 using helper function
           delay(2000);
           lcdClear(lcd);
           blinkN(gpio, pin2LED2, 3);//Blink RED LED three times as round ends
       }

    /* ******************************************************* */
    /* ***  COMPLETE the code here  ***                        */
    /* this needs to implement the main loop of the game:      */
    /* check for button presses and count them                 */
    /* store the input numbers in the sequence @attSeq@        */
    /* compute the match with the secret sequence, and         */
    /* show the result                                         */
    /* see CW spec for details                                 */
    /* ******************************************************* */
  
  if (found) {
      /* ***  COMPLETE the code here  ***  */
	        lcdClear(lcd);
	         fprintf(stdout, "\nCongratulations! You broke the code in %d attempts!\n\n", attempts);//Print success message
	         lcdPosition(lcd, 0, 0);
	         lcdPuts(lcd, "SUCCESS! ");//Prints success message on lcd
	         
           //Blinking of LEDs 
	         writeLED(gpio, pin2LED2, HIGH);//RED LED stays ON
	         blinkN(gpio, pinLED, 3);//Blink GREEN LED 3 times
	         writeLED(gpio,pin2LED2, LOW);//RED LED is switched off
	         delay(300);
	         lcdClear(lcd);

  } else { //If sequence is not found in 5 attempts
	  	  lcdClear(lcd);
           fprintf(stderr, "Sequence not found.\nNumber of attempts: %d\n", attempts);//print failure message
           fprintf(stderr, "Better luck next time!\n");
           showSeq(theSeq);//Show the sequence
           lcdPosition(lcd, 0, 0);
           lcdPuts(lcd, "Game Over!"); //Display Game over
           delay(300);
           lcdClear(lcd);
  }
          
  // Free allocated memory
      free(userInput);
      free(attSeq);
      free(theSeq);
  return 0;
}

