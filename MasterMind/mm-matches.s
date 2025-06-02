@ This ARM Assembler code should implement a matching function, for use in the MasterMind program, as
@ described in the CW2 specification. It should produce as output 2 numbers, the first for the
@ exact matches (peg of right colour and in right position) and approximate matches (peg of right
@ color but not in right position). Make sure to count each peg just once!

@ Example (first sequence is secret, second sequence is guess):
@ 1 2 1
@ 3 1 3 ==> 0 1
@ You can return the result as a pointer to two numbers, or two values
@ encoded within one number
@
@ -----------------------------------------------------------------------------
HEXBASE:
    .word 0x20000000  
digits:
    .word 0x30 @ '0'
    .word 0x31 @ '1'
    .word 0x32 @ '2'
    .word 0x33 @ '3'
    .word 0x44 @ '4'
    .word 0x45 @ '5'
    .word 0x46 @ '6'
    .word 0x47 @ '7'
    .word 0x48 @ '8'
    .word 0x49 @ '9'
    .word 0x4A @ 'A'
    .word 0x4B @ 'B'
    .word 0x4C @ 'C'
    .word 0x4D @ 'D'
    .word 0x4E @ 'E'
    .word 0x4F @ 'F'

.section .bss
.align 2
.global __end_stack
__end_stack:
    .space 4   

.text
@ this is the matching fct that should be called from the C part of the CW	
.global matches
@ use the name `asmmain` here, for standalone testing of the assembler code
@ when integrating this code into `master-mind.c`, choose a different name
@ otw there will be a clash with the main function in the C code
.global asmmain

@ Entry point for standalone testing
asmmain: 
    PUSH {R4-R8, LR}      @ Save registers we'll use
    LDR  R0, =secret      @ pointer to secret sequence
    LDR  R1, =guess       @ pointer to guess sequence
    
    BL   matches          @ call the matches function
    
    @ Display or use result here if needed
    @ R0 contains the combined value as a 2-digit decimal number
    @ where tens digit = exact matches, ones digit = approx matches
    
exit:
    MOV  R7, #1           @ load system call code
    SWI  0                @ return this value
    POP  {R4-R8, PC}      @ Restore registers and return

@ -----------------------------------------------------------------------------
@ this is the matching fct that should be callable from C
@ Input: R0, R1 = pointers to int arrays to match
@ Output: R0 = combined result as 2-digit decimal number
@         (exact matches in tens place, approx matches in ones place)
matches:
    PUSH {R4-R10, LR}     @ Save registers
    
    @ Make working copies of the arrays to avoid modifying the original data
    SUB  SP, SP, #24      @ Allocate space for working copies (3 words each)
    MOV  R4, SP           @ R4 = secret copy
    ADD  R5, SP, #12      @ R5 = guess copy
    
    @ Copy arrays to working space
    LDR  R6, [R0]         @ Load secret[0]
    STR  R6, [R4]         @ Store in secret copy
    LDR  R6, [R0, #4]     @ Load secret[1]
    STR  R6, [R4, #4]     @ Store in secret copy
    LDR  R6, [R0, #8]     @ Load secret[2]
    STR  R6, [R4, #8]     @ Store in secret copy

    LDR  R6, [R1]         @ Load guess[0]
    STR  R6, [R5]         @ Store in guess copy
    LDR  R6, [R1, #4]     @ Load guess[1]
    STR  R6, [R5, #4]     @ Store in guess copy
    LDR  R6, [R1, #8]     @ Load guess[2]
    STR  R6, [R5, #8]     @ Store in guess copy
    
    MOV  R6, #0           @ Initialize exact matches counter
    MOV  R7, #0           @ Initialize approximate matches counter
    MOV  R8, #3           @ Load sequence length
    
    @ First pass: Count exact matches
    MOV  R9, #0           @ Initialize index
exact_loop:
    LDR  R2, [R4, R9, LSL #2]    @ Load secret value
    LDR  R3, [R5, R9, LSL #2]    @ Load guess value
    CMP  R2, R3                  @ Compare values
    BNE  not_exact               @ Branch if not equal
    ADD  R6, R6, #1              @ Increment exact matches
    MOV  R2, #NAN1               @ Mark as matched in secret
    STR  R2, [R4, R9, LSL #2]
    MOV  R3, #NAN2               @ Mark as matched in guess
    STR  R3, [R5, R9, LSL #2]
not_exact:
    ADD  R9, R9, #1              @ Increment index
    CMP  R9, R8                  @ Compare with length
    BLT  exact_loop              @ Continue if not done
    
    @ Second pass: Count approximate matches
    MOV  R9, #0                  @ Reset index for outer loop
approx_outer:
    LDR  R2, [R4, R9, LSL #2]    @ Load secret value
    CMP  R2, #NAN1               @ Skip if already matched
    BEQ  next_outer
    
    MOV  R10, #0                 @ Index for inner loop
approx_inner:
    LDR  R3, [R5, R10, LSL #2]   @ Load guess value
    CMP  R3, #NAN2               @ Skip if already matched
    BEQ  next_inner
    CMP  R2, R3                  @ Compare values
    BNE  next_inner
    ADD  R7, R7, #1              @ Increment approximate matches
    MOV  R3, #NAN2               @ Mark as matched
    STR  R3, [R5, R10, LSL #2]
    B    next_outer
next_inner:
    ADD  R10, R10, #1
    CMP  R10, R8
    BLT  approx_inner
next_outer:
    ADD  R9, R9, #1
    CMP  R9, R8
    BLT  approx_outer
    
    @ Combine results for C return value as a 2-digit decimal number
    MOV  R0, R6                  @ R0 = exact matches
    MOV  R2, #10                 @ R2 = 10
    MOV  R1, R0                  @ Copy exact matches to R1
    MUL  R0, R1, R2              @ R0 = exact * 10
    ADD  R0, R0, R7              @ R0 = (exact * 10) + approx
    
    @ Clean up and return
    ADD  SP, SP, #24             @ Deallocate working space
    POP  {R4-R10, PC}            @ Restore registers and return

@ show the sequence in R0, use a call to printf in libc to do the printing, a useful function when debugging 
showseq: 			@ Input: R0 = pointer to a sequence of 3 int values to show
    @ COMPLETE THE CODE HERE (OPTIONAL)
    
@ =============================================================================
.section .note.GNU-stack,"",%progbits  
.data

@ constants about the basic setup of the game: length of sequence and number of colors	
.equ LEN, 3
.equ COL, 3
.equ NAN1, 8
.equ NAN2, 9

@ a format string for printf that can be used in showseq
f4str: .asciz "Seq:    %d %d %d\n"

@ a memory location, initialised as 0, you may need this in the matching fct
n: .word 0x00
    
@ INPUT DATA for the matching function
.align 4
secret: .word 1 
    .word 2 
    .word 1 

.align 4
guess:	.word 3 
    .word 1 
    .word 3 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 0 1
.align 4
expect: .byte 0
    .byte 1

.align 4
secret1: .word 1 
     .word 2 
     .word 3 

.align 4
guess1:	.word 1 
    .word 1 
    .word 2 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 1
.align 4
expect1: .byte 1
     .byte 1

.align 4
secret2: .word 2 
     .word 3
     .word 2 

.align 4
guess2:	.word 3 
    .word 3 
    .word 1 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 0
.align 4
expect2: .byte 1
     .byte 0

