#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "os_generic.h"
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/sensor.h>
#include "CNFGAndroid.h"
#include <time.h>

#include <setgame.h>

#define CNFG3D
#define CNFG_IMPLEMENTATION
#include "CNFG.h"

unsigned frames = 0;
unsigned long iframeno = 0;

#define GENLINEWIDTH 89
#define GENLINES 40

//https://ok-color-picker.netlify.app/#2a8f00
#define RED   0xde1100ff 
#define GREEN 0x2a8f00ff
#define BLUE  0x006afcff
//#define SELECTED_COLOR   0xaaaaaaff //light
//#define UNSELECTED_COLOR 0xccccccff //light
//#define BACKGROUND_COLOR 0xffffffff //light
#define SELECTED_COLOR   0x111111ff	//dark
#define UNSELECTED_COLOR 0x222222ff //dark
#define BACKGROUND_COLOR 0x090909ff //dark
#define FILLING_DENSITY 8

int selectedCards[4];
struct Game g;
short cardx, cardy;
int mode = 0;//current gamemode  normal, chain, ultra
double startTime;
double finishTime; //-1 if game didn't finish yet, also used to check if game finished yet

int genlinelen = 0;
char genlog[(GENLINEWIDTH+1)*(GENLINES+1)+2] = "log";
int genloglen;
int genloglines;
int firstnewline = -1;



void startGame(int mode);
void finishGame();
void example_log_function( int readSize, char * buf )
{
	static og_mutex_t * mt;
	if( !mt ) mt = OGCreateMutex();
	OGLockMutex( mt );
	int i;
	for( i = 0; (readSize>=0)?(i <= readSize):buf[i]; i++ )
	{
		char c = buf[i];
		if( c == '\0' ) c = '\n';
		if( ( c != '\n' && genlinelen >= GENLINEWIDTH ) || c == '\n' )
		{
			int k;
			genloglines++;
			if( genloglines >= GENLINES )
			{
				genloglen -= firstnewline+1;
				int offset = firstnewline;
				firstnewline = -1;

				for( k = 0; k < genloglen; k++ )
				{
					if( ( genlog[k] = genlog[k+offset+1] ) == '\n' && firstnewline < 0)
					{
						firstnewline = k;
					}
				}
				genlog[k] = 0;
				genloglines--;
			}
			genlinelen = 0;
			if( c != '\n' )
			{
				genlog[genloglen+1] = 0;
				genlog[genloglen++] = '\n';
			}
			if( firstnewline < 0 ) firstnewline = genloglen;
		}
		genlog[genloglen+1] = 0;
		genlog[genloglen++] = c;
		if( c != '\n' ) genlinelen++;
	}

	OGUnlockMutex( mt );
}

volatile int suspended;

short screenx, screeny;
int lastbuttonx = 0;
int lastbuttony = 0;
int lastmotionx = 0;
int lastmotiony = 0;
int lastbid = 0;
int lastmask = 0;
int lastkey, lastkeydown;

static int keyboard_up;

void HandleKey( int keycode, int bDown )
{
	lastkey = keycode;
	lastkeydown = bDown;
	if( keycode == 4 ) { AndroidSendToBack( 1 ); }
}

void HandleButton( int x, int y, int button, int bDown )
{
	lastbid = button;
	lastbuttonx = x;
	lastbuttony = y;
	if (bDown == 0)return;
	//bottom menu
	if (screeny - y < 75){
		int but = (int)((float)x/screenx*5);
		if (but > 1 && but-2!=mode){
			finishGame();
			mode = but-2;
			startGame(mode);
			return;
		}
		if (but == 1){
		finishGame();
		startGame(mode);
		}
		if (but == 0){
		if (mode != -1)
			mode = -1;
		else
			mode = g.mode;
		}
	return;
	}
	
	if (finishTime > 0)return;
	//select card
	int cardsPerSet = (mode != 2) ? 3 : 4;
	int clickedCard =  x/cardx + 3*(y/cardy);
	if (clickedCard >= g.sizeCards) return;
		//deselect card?
	for (int i=0;i<4;i++) {
		if (clickedCard == selectedCards[i])
			{
			for (int j=i;j<3;j++){selectedCards[j]=selectedCards[j+1];}
			selectedCards[3] = -1;
			return;
			}
	}
	for (int i=cardsPerSet-1;i>0;i--)selectedCards[i]=selectedCards[i-1];
	selectedCards[0] = clickedCard;
	if (selectedCards[cardsPerSet-1]==-1) return;
	//set found?
	unsigned char cards[4];
	for (int i=0;i<cardsPerSet;i++){
		cards[i] = g.cards[selectedCards[i]];
		//sort
		for (int j=i;j>0;j--){
			if (cards[j]<cards[j-1]){
			cards[j]   = cards[j]+cards[j-1];
			cards[j-1] = cards[j]-cards[j-1];
			cards[j]   = cards[j]-cards[j-1];
			}
		}
	}
	for (int i=0;i<g.sizeSets;i++){
		if (mode != 2){
		if (cards[0]==g.sets[i][0] && cards[1]==g.sets[i][1] &&
			cards[2]==g.sets[i][2] && cards[2]==g.sets[i][2]){
			for(int k=0;k<4;k++) selectedCards[k]=-1;
			handleFound(&g,g.sets[i]);}
		}
		else {
		if (cards[0]==g.sets[i][0] && cards[1]==g.sets[i][1] && cards[2]==g.sets[i][2]){
			for(int k=0;k<4;k++) selectedCards[k]=-1;
			handleFound(&g,g.sets[i]);}
		}
	}
	if (g.sizeSets == 0){
	finishGame();
	}
}

void HandleMotion( int x, int y, int mask )
{
	lastmask = mask;
	lastmotionx = x;
	lastmotiony = y;
}

extern struct android_app * gapp;


void HandleDestroy()
{
}

void HandleSuspend()
{
	suspended = 1;
}

void HandleResume()
{
	suspended = 0;
}

void drawSquiggle(short xPos,short yPos,short length, int filling){
	//TODO nicht drehen, direkt berechnen; an Enden segmente statt nur Punkte
	//length in y-dir; x, y middle of the symbol
	//-sinus curve from -pi to pi, +-1 for thicknes
	//gets thinner like a unit circle at the ends
	//rotated by 1.2166738-pi/2 = -0.354 to be upright
	//see ~/documents/set/cards/squiggle.py
	short one = length/(2*M_PI);
	yPos += length*0.07;
	xPos -= length/M_PI;
	yPos -= length/2;
	for (short y = 0; y < length;y++){
		float scaledY = 2*M_PI*y/length;
		short width = one; //width rechteck + Einheitskreise an den Enden
		if (scaledY < 1)         {width = sqrt(1 - (1-scaledY)*(1-scaledY))*one;}
		if (scaledY > 2*M_PI - 1){width = sqrt(1 - (scaledY-2*M_PI+1)*(scaledY-2*M_PI+1))*one;}
	//	width superellipse; (x/a)**n+(y/b)**n = 1; b = length; a = length/(2*M_PI); n = 3
	//	x = a*nthroot(1-(y/b)**n)
	//	short width = (float)length/(2*M_PI)
	//	*cbrt(1 - fabsf((y/(float)length*2-1)*(y/(float)length*2-1)*(y/(float)length*2-1)));
		short  sine = sin(scaledY)*one+one;
		float angle = -0.354;
		short x1Rot = (-width+sine)*cos(angle) - y*sin(angle);
		short x2Rot = (+width+sine)*cos(angle) - y*sin(angle);
		short y1Rot = (-width+sine)*sin(angle) + y*cos(angle);
		short y2Rot = (+width+sine)*sin(angle) + y*cos(angle);
	//	CNFGTackPixel(xPos-width+sine,yPos+y);
	//	CNFGTackPixel(xPos+width+sine,yPos+y);
		if ((filling == 0) || (filling == 1)){
			CNFGTackPixel(xPos+x1Rot, yPos+y1Rot);
			CNFGTackPixel(xPos+x2Rot, yPos+y2Rot);
		}
		if ((filling == 1 && y%FILLING_DENSITY==0) || filling == 2){
			CNFGTackSegment(xPos+x1Rot,yPos+y1Rot,xPos+x2Rot,yPos+y2Rot);
		}
	}
}
void drawOval(short xPos,short yPos,short length, int filling){
	//Superellipse with a = 1; b = 3; n = 2; m = 3;
	//previously n = 1/2 m = 10
	const short a = length/5;
	const short b = length/2;
	for (short y = -b; y < b;y++){
	float yb = fabsf((float) y / b);
	short x = a*sqrt(1 - yb*yb*yb); //faster than pow()
	if ((filling == 0) || (filling == 1)){
		CNFGTackPixel(xPos-x, yPos+y);
		CNFGTackPixel(xPos+x, yPos+y);
	}
	if ((filling == 1 && y%FILLING_DENSITY==0) || filling == 2){
		CNFGTackSegment(xPos-x, yPos+y, xPos+x,yPos+y);
	}
	}
}
void drawDiamond(short xPos,short yPos,short length, int filling){
	//Superellipse with a = 1; b = 3; n = 3/4; m = 1;
	const short a = length*0.2;
	const short b = length/2;
	for (short y = -b; y < b;y++){
	float yb = fabsf((float) y / b);
	short x = a*cbrt((1-yb)*(1-yb)*(1-yb)*(1-yb));
	if ((filling == 0) || (filling == 1)){
		CNFGTackPixel(xPos-x, yPos+y);
		CNFGTackPixel(xPos+x, yPos+y);
	}
	if ((filling == 1 && y%FILLING_DENSITY==0) || filling == 2){
		CNFGTackSegment(xPos-x, yPos+y, xPos+x,yPos+y);
	}
	}
}
void drawCard(short xPos, short yPos, unsigned char c){
	CNFGTackRectangle(xPos+5, yPos+5, xPos+cardx-5, yPos+cardy-5);
	int color	= c%3;
	c = c/3;
	int shape	= c%3;
	c = c/3;
	int filling	= c%3;
	c = c/3;
	int number	= c%3;
	switch (color){ //color
	case 0:
		CNFGColor(RED);
		break;
	case 1:
		CNFGColor(GREEN);
		break;
	case 2:
		CNFGColor(BLUE);
		break;
	}
//	if (filling==0)	CNFGSetLineWidth(2); //activate once squiggles are fixed
//	else CNFGSetLineWidth(1);
	void (*drawShape)(short xPos, short yPos, short length, int filling);
	switch (shape){
	case 0:
		drawShape = drawOval;
		break;
	case 1:
		drawShape = drawDiamond;
		break;
	case 2:
		drawShape = drawSquiggle;
		break;
	}
	switch (number){
	case 0:
		drawShape(xPos + ((float)1/2)*cardx, yPos + 0.5*cardy, 0.8*cardy, filling);
		break;             
	case 1:                
		drawShape(xPos + ((float)1/3)*cardx, yPos + 0.5*cardy, 0.8*cardy, filling);
		drawShape(xPos + ((float)2/3)*cardx, yPos + 0.5*cardy, 0.8*cardy, filling);
		break;             
	case 2:                
		drawShape(xPos + ((float)1/4)*cardx, yPos + 0.5*cardy, 0.8*cardy, filling);
		drawShape(xPos + ((float)2/4)*cardx, yPos + 0.5*cardy, 0.8*cardy, filling);
		drawShape(xPos + ((float)3/4)*cardx, yPos + 0.5*cardy, 0.8*cardy, filling);
		break;
	}
}
void drawCards(){
	for (int i=0; i<g.sizeCards; i++){
		if ((mode == 1 && i<3 && g.sizeSetsFound>0 ) || finishTime > 0)
			CNFGColor(0x181818ff);
		else
			CNFGColor(UNSELECTED_COLOR);
		for (int j=0; j<4;j++){
		if (i==selectedCards[j]) CNFGColor(SELECTED_COLOR);}
		drawCard(i%3*cardx, i/3*cardy, g.cards[i]);
	}
}
void startGame(int mode){
	startTime = OGGetAbsoluteTime();
	srand((long)(startTime));
	//srand(1710620931);
	g = initGame(mode);
	for(int k=0;k<4;k++) selectedCards[k]=-1;
	finishTime = -1;
}
void finishGame(){
	for (int i=0;i<4;i++){
		selectedCards[i]=-1;
	}	
	finishTime = OGGetAbsoluteTime();
	//TODO save game in storage iff at least one set was found
}
int main()
{
	int i, x, y;
	double ThisTime = OGGetAbsoluteTime();
	double LastFPSTime = OGGetAbsoluteTime();
	double SecToWait;
	int linesegs = 0;

	startGame(mode);
	//startTime = OGGetAbsoluteTime();
	char timePassed[10];
	finishTime = -1;
	
	CNFGSetLineWidth(2);
	CNFGSetupFullscreen( "Test Bench", 0 );

	const char * assettext = "Not Found";
	AAsset * file = AAssetManager_open( gapp->activity->assetManager, "test.txt", AASSET_MODE_BUFFER );
	if( file )
	{
		size_t fileLength = AAsset_getLength(file);
		char * temp = malloc( fileLength + 1);
		memcpy( temp, AAsset_getBuffer( file ), fileLength );
		temp[fileLength] = 0;
		assettext = temp;
	}

	while(1)
	{
		int i, pos;
		float f;
		iframeno++;
		RDPoint pto[3];

		CNFGHandleInput();

		if( suspended ) { usleep(50000); continue; }

		CNFGClearFrame();

		CNFGGetDimensions( &screenx, &screeny );
		cardx = screenx/3; //size of one card
		cardy = cardx*0.64;
		if (cardy*6>screeny) cardy = screeny/6;

		CNFGBGColor = BACKGROUND_COLOR;
	
		if (mode != -1)
		drawCards();
		//debug screen
		else {
			char debugText[20] = "____________________";
			CNFGPenY = 10;
			CNFGPenX = 10;
			for (int i=0;i<g.sizeSets;i++){
				sprintf(debugText,"%2hhu %2hhu %2hhu %2hhu"
						,g.sets[i][0],g.sets[i][1],g.sets[i][2],g.sets[i][3]);
				CNFGDrawText(debugText,5);
				CNFGPenY+=25;
			}
			CNFGPenY = 10;
			CNFGPenX = 220;
			for (int i=0;i<g.sizeCards;i+=3){
				sprintf(debugText,"%2hhu %2hhu %2hhu"
						,g.cards[i+0],g.cards[i+1],g.cards[i+2]);
				CNFGDrawText(debugText,5);
				CNFGPenY+=25;
			}
			CNFGPenY+=25;
			for (int i=0;i<81;i+=3){
				sprintf(debugText,"%2hhu %2hhu %2hhu",g.deck[i],g.deck[i+1],g.deck[i+2]);
				if (i>(81-g.remainingCards)){
					CNFGColor(RED);
				}
				CNFGDrawText(debugText,5);
				CNFGPenY+=25;
			}
			CNFGColor(0xffffffff);
			CNFGPenY = 10;
			CNFGPenX = 420;
			sprintf(debugText,"%li",(long)startTime);
			CNFGDrawText(debugText,5);
			CNFGPenX = 420;
			CNFGPenY = 35;
			sprintf(debugText,"%i %i %i %i"
					,selectedCards[0],selectedCards[1],selectedCards[2],selectedCards[3]);
			CNFGDrawText(debugText,5);
			CNFGPenY = 100;
			CNFGPenX = 420;
			for (int i=0;i<g.sizeSetsFound;i++){
				sprintf(debugText,"%2hhu %2hhu %2hhu %2hhu"
						,g.setsFound[i][0],g.setsFound[i][1],g.setsFound[i][2],g.setsFound[i][3]);
				CNFGDrawText(debugText,5);
				CNFGPenY+=25;
			}
		}
		
		//bottom row
		CNFGPenX = 10;
		CNFGColor(UNSELECTED_COLOR);
		for (int i=0; i<5; i++){
		if (i-2 == g.mode){
			CNFGColor(SELECTED_COLOR);
			CNFGTackRectangle(screenx*0.2*i+5, screeny-60, screenx*0.2*(i+1)-5, screeny-5);
			CNFGColor(UNSELECTED_COLOR);
		}
		else 
			CNFGTackRectangle(screenx*0.2*i+5, screeny-60, screenx*0.2*(i+1)-5, screeny-5);
		}
		CNFGColor( 0xffffffff );
		CNFGPenY = screeny - 50;
		CNFGPenX = 10+screenx*0.2*0;
		if (g.sizeSets!=0)
			sprintf(timePassed,"%lis %i",(long)(ThisTime - startTime),g.remainingCards);
		else
			sprintf(timePassed,"%.3fs",finishTime-startTime);
		CNFGDrawText( timePassed ,8);
		CNFGPenX = 10+screenx*0.2*1;
		CNFGDrawText("Restart",8);
		CNFGPenX = 10+screenx*0.2*2;
		CNFGDrawText("Normal",8);
		CNFGPenX = 10+screenx*0.2*3;
		CNFGDrawText("Chain",8);
		CNFGPenX = 10+screenx*0.2*4;
		CNFGDrawText("Ultra",8);


		//log and FPS
		CNFGPenY = screeny - 1000;
		CNFGPenX = 5;
//		CNFGDrawText(genlog, 4);
		frames++;
		CNFGSwapBuffers();

		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 )
		{
			printf( "FPS: %d\n", frames );
			printf("%d %d \n",lastbuttonx, lastbuttony);
			printf("%s \n",timePassed);
			printf("%f \n",wgl_last_width_over_2);//CNFGSetLineWidth
			frames = 0;
			linesegs = 0;
			LastFPSTime+=1;
		}
	}

	return(0);
}

