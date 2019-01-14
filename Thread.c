
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "Board_GLCD.h"
#include "GLCD_Config.h"
#include "Board_Buttons.h"
#include "stm32f2xx_hal.h"
#include <cstring>
#include <stdlib.h>

// Print LCD func
void Print_LCD(char* message);


/*----------------------------------------------------------------------------
 *      Threads Code
 *---------------------------------------------------------------------------*/
 
void Thread_Task (void const *argument);        // physics + controlls thread function
void GUI_Thread (void const *argument);         // GUI thread function              
osThreadId tid_Thread;                          // thread id
osThreadId tid_GuiThread;                       // GUI thread ID
osThreadDef (Thread_Task, osPriorityHigh, 1, 0);// thread object
osThreadDef (GUI_Thread, osPriorityHigh, 1, 0); // GUI thread object

/*
    Threads initialization function
*/
int Init_Thread (void) {

  tid_Thread = osThreadCreate (osThread(Thread_Task), NULL);
  if (!tid_Thread) return(-1);
	tid_GuiThread = osThreadCreate (osThread(GUI_Thread), NULL);
   if (!tid_GuiThread) return(-1);
  return(0);
}

/*
  Print square figure int the point (x,y). Big rectangle 24*24 and 4 small (12*12) rectangles inside.
*/
void Print_square(int x , int y){
	GLCD_DrawRectangle(x*24U,y*24U, 24U, 24U ); // paint big rectangle (24*24)
	GLCD_DrawRectangle(x*24U,y*24U, 12U, 12U );  //small rectangle 1 (12*12)
	GLCD_DrawRectangle((x*24U)+12U,y*24U, 12U, 12U ); //small rectangle 2 (12*12)
	GLCD_DrawRectangle((x*24U)+12U,(y*24U)+12U, 12U, 12U ); //small rectangle 3 (12*12)
	GLCD_DrawRectangle((x*24U),(y*24U)+12U, 12U, 12U ); //small rectangle 4 (12*12)
}


/*
 Delay function. Game speed configuration.
*/
void delay ()
{
	unsigned int i;
	for (i = 0; i < 10000000; i++) {}
}
/*
 Button pressing pause function
*/
void delayBtn ()
{
	unsigned int i;
	for (i = 0; i < 1000000; i++) {}
}

/*
Game field array. Dim: 10*12. if   gamefield[i,j]  == 1 then we should draw figure at this point
*/
int* gameField;
/*
  Field y size
*/
int fieldySize = 10;
/*
  Field x size
*/
int fieldxSize = 12;

/*
  Main GUI thread.
  Performs screen updates.
  Paints figures in point  i,j if gameField contain 1 in this cell.
*/
void GUI_Thread(void const *argument){
  int i, j = 0; 
 // game field declaration
 gameField =(int *)malloc(fieldySize * fieldxSize * sizeof(int));   
	// game field initialization ( default all zeros) 
	for (i = 0; i <  fieldxSize; i++) {
   for (j = 0; j < fieldySize; j++)  {
		 *(gameField + j*fieldxSize + i) = 0; 
	 }
	}
	/*
	Main cycle. Just draw the whole gameField each time.
	*/
	while (1) {
		for (i = 0; i <  fieldxSize; i++) {
   for (j = 0; j < fieldySize; j++)  {
		 // if  field[i,j] is 1, then draw figure.
		 if (*(gameField + j*fieldxSize + i) == 1) {
			 Print_square(i,j);
			
		 }
	 }
	}
		// clear screen each iteration
		GLCD_ClearScreen();
	}

}

/*
Main game behaivour thread. It changes gamefield array.
*/
void Thread_Task (void const *argument) {
  /*
	Current figure coordinates.
	*/
	int curX, curY = 0;
	/*
	Default x coordinate in the center of the screen
	*/
	curX = fieldxSize/2;
  while (1) {
		/*
		Main physics if current figure can fall -- it falls (curY++).
		It can fall if:
				1) it is at the bottom of the screen. (curY == fieldySize-1)
				2) it has some other figure below (*(gameField + (curY+1)*fieldxSize + curX) !=1)

		*/
		if ((curY != fieldySize-1) && (*(gameField + (curY+1)*fieldxSize + curX) !=1)) {
			*(gameField + curY*fieldxSize + curX) = 0; //We also need to clear previous figure position
		  curY++;
		
		}
		/*
		If we can't fall. We start a new figure
		*/
		else {

			curY=0;
			curX = fieldxSize/2;
		}
    /*Controlls handling.*/
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)) {	                // checks if PA0 is set. (Wake button)
			   /*
			     If we can move figure to the left -- move it.
			     We can move it if:
			        1) Current x  position is greater than 0. (curX > 0)
			        2) There are no other figures to the left ((curY)*fieldxSize + (curX-1)) !=1))
			   
			   */
         if ((curX > 0) && (*(gameField + (curY)*fieldxSize + (curX-1)) !=1)) {
					 *(gameField + curY*fieldxSize + curX) = 0; //We also need to clear previous figure position
					 curX--;
				 }
				 HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0); // reset pin for button
				 delayBtn();  // some delay for avoiding unnecessary multiple handling
    }
		
		if (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)) {	                // checks if PC13 is set (Tamper button)
				/*
			     If we can move figure to the right -- move it.
			     We can move it if:
			        1) Current x  position is less than maximum. (curX < fieldxSize-1)
			        2) There are no other figures to the right ((*(gameField + (curY)*fieldxSize + (curX+1)) !=1))
			   
			   */
			if ((curX < fieldxSize-1) && (*(gameField + (curY)*fieldxSize + (curX+1)) !=1)) {
				*(gameField + curY*fieldxSize + curX) = 0; //We also need to clear previous figure position
				curX++;
			}
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // reset pin for button
			delayBtn(); // some delay for avoiding unnecessary multiple handling
    }
                                             
		/*
		 Set figure position in calculated coordinates
		*/
		*(gameField + curY*fieldxSize + curX) = 1;
		 delay();  // suspend thread, this needed for game speed changing
  }
}
