#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
//card: unsigned char c; 0<=card<=80; farbe, form, fuellung, anzahl; color, shape, filling, number
struct Game{
	int mode;//mode 0 = normal, 1=chain, 2=ultra
	int sizeCards;
	int sizeSets;
	int sizeSetsFound;
	int remainingCards;//cards remaining in Deck
	unsigned char deck[81];//all cards, shuffled
	unsigned char cards[27];//cards on table, including previous set in chain
                            //swf keybinds say its below 27
	bool boolCards[81];     //cards on table
	unsigned char setsFound[40][4];//sets Found by player
	double startTime;
	double timeFound[40];
	//sets on table, cards are sorted
	unsigned char sets[200][4];
};
unsigned char conjugateCard(unsigned char a, unsigned char b);//no side effects
void generateDeck(unsigned char * deck);//modifies deck; shuffles
void findSets(struct Game *g);//modifies sets
void addCards(struct Game *g);//modifies cards, indirectly sets
void handleFound(struct Game *g, unsigned char set[4]);//modifies everything
struct Game initGame(int mode);
void testBoolCards(struct  Game *g);
void simulateGame(int mode); //needs global variables
void simulateGame(int mode){
	struct Game g = initGame(mode);
	int maxBoard=12;
	while(g.sizeSets>0 && g.sizeSetsFound < 28){
//	printf("------------------------------ \n");
//	boards[(g.sizeCards/3)]++;
	handleFound(&g,g.sets[0]);//((rand())%g.sizeSets)
	if (g.sizeCards>maxBoard){maxBoard=g.sizeCards;}
	}
//	games[maxBoard/3]++;
//	remainingCards[g.sizeCards/3]++;
}
void writeGameToFile(struct Game g, FILE* file){
	int length = 0;
	fseek(file,2, SEEK_CUR);
	//startTime
	long long startTime = (long long)g.startTime;
	for (int i=0;i<8;i++){
		fputc(startTime%256,file);length++;
		startTime >>= 8;
	}
	//cards
	if (g.sizeSetsFound>0){
		const int cardsPerSet = (g.mode==2)?4:2;
		for(int i=0;i<g.sizeSetsFound;i++){
			for(int j=0;j<cardsPerSet;j++){
				fputc(g.setsFound[i][j],file);length++;
			}
		}
		fseek(file,-1,SEEK_CUR);
		fputc(128+g.setsFound[g.sizeSetsFound-1][cardsPerSet-1],file);
	}
	//times in millis since last set in LEB128
	unsigned long timeSet;
	unsigned char byte;
	for(int i=0;i<g.sizeSetsFound;i++){
		if (i>0)
			timeSet = (long)(1000*(g.timeFound[i]-g.timeFound[i-1]));
		else
			timeSet = (long)(1000*(g.timeFound[i]-g.startTime));
		do {
			byte = timeSet%128;
			timeSet >>=7;
			if (timeSet!=0)
				byte+=128;
			fputc(byte,file);length++;
		} while (timeSet!=0);
	}
	//message length
	fseek(file,-length-2,SEEK_CUR);
	fputc(length     %256,file);
	fputc((length>>8)%256,file);
	fseek(file,length,SEEK_CUR);
	return;
}
unsigned char conjugateCard(unsigned char a, unsigned char b){
	//TODO replace with precalculated array[1080]
	unsigned char c = 0;
	for (int i=1;i<81;i*=3){
		c+=(81-(a%3+b%3))%3*i;
		a=a/3;
		b=b/3;
	}
	return c;
}
void generateDeck(unsigned char * deck){
	for (unsigned char i=0;i<81;i++){
		unsigned char j = rand()%(i+1);
		*(deck+i) = i;
		int temp=*(deck+j);
		*(deck+j)=i;
		*(deck+i)=temp;
	}
}
void addCards(struct Game *g){
	//TODO add duffs device to call findSets less often
	while (((*g).sizeSets==0 || (*g).sizeCards%3!=0) && (*g).remainingCards>0){
		(*g).cards[(*g).sizeCards]=(*g).deck[81-(*g).remainingCards];
		(*g).boolCards[(*g).deck[81-(*g).remainingCards]]=true;
		(*g).sizeCards++;
		(*g).remainingCards--;
		findSets(g);
	}
}
void findSets(struct Game *g){
	//nice way to find every set without finding one twice?
	//TODO Cache sets
	//recalc boolCards
	for (int i = 0; i<81; i++){
		(*g).boolCards[i]=false;
	}
	for (int i=0;i<(*g).sizeCards;i++){
		(*g).boolCards[(*g).cards[i]]=true;
	}
//	testBoolCards(g);	
	(*g).sizeSets=0;
	unsigned char a;	//currently all 6 permutations checked, only sorted added
	unsigned char b;	//could improve speed at least 2x
	unsigned char c;
	unsigned char d;
	switch ((*g).mode){
	case 0:{
	normal:
	for(int i = 0; i<(*g).sizeCards;i++){
	for(int j = 0; j<(*g).sizeCards;j++){
		a = (*g).cards[i];
		b = (*g).cards[j];
		c = conjugateCard(a,b);
		if (a<b && b<c && (*g).boolCards[c]){
			//if optimized needs a different uniqueness check
			(*g).sets[(*g).sizeSets][0]=a;
			(*g).sets[(*g).sizeSets][1]=b;
			(*g).sets[(*g).sizeSets][2]=c;
			(*g).sizeSets++;
		}
	}
	}
	break;
	}
	case 1:{
	if((*g).sizeSetsFound==0){goto normal;}//first Chain Set
	for(int i = 0; i<3;i++){
	for(int j = 3; j<(*g).sizeCards;j++){
		a = (*g).cards[i];
		b = (*g).cards[j];
		c = conjugateCard(a,b);
		if (b<c && (*g).boolCards[c]){//always keep chain first?
			if (a>b){a=a+b;b=a-b;a=a-b;}
			if (b>c){b=b+c;c=b-c;b=b-c;}
			(*g).sets[(*g).sizeSets][0]=a;
			(*g).sets[(*g).sizeSets][1]=b;
			(*g).sets[(*g).sizeSets][2]=c;
			(*g).sizeSets++;
		}
	}
	}
	break;
	}
	case 2:{//ultra
	//TODO cache conjugates, "hashmap" for conjugates
	int i1; int i2; int j1; int j2;
	unsigned char conjugates[78];
	int pairCount = ((*g).sizeCards)*((*g).sizeCards+1)/2;
	for(int i=0;i<pairCount;i++){
		i1 = (int)((sqrt(8*i+1)-1)/2);//index first card
		i2 = i-(int)(i1*(i1+1)/2);
		conjugates[i]=conjugateCard(
		(*g).cards[i1],(*g).cards[i2]);
		for (int j=0;j<i;j++){
			if(conjugates[i]==conjugates[j]){
			j1 = (int)((sqrt(8*j+1)-1)/2);//index first card
			j2 = j-(int)(j1*(j1+1)/2);
			a = (*g).cards[i1];
			b = (*g).cards[i2];
			c = (*g).cards[j1];
			d = (*g).cards[j2];
			if (a==b || b==c || c==d) continue;
			if (a>b){a=a+b;b=a-b;a=a-b;}
			if (c>d){c=c+d;d=c-d;c=c-d;}
			if (a>c){
				a=a+c;c=a-c;a=a-c;
				b=b+d;d=b-d;b=b-d;
			}
			if (b>c){b=b+c;c=b-c;b=b-c;}
			if (c>d){c=c+d;d=c-d;c=c-d;}
			(*g).sets[(*g).sizeSets][0] = a;
			(*g).sets[(*g).sizeSets][1] = b;
			(*g).sets[(*g).sizeSets][2] = c;
			(*g).sets[(*g).sizeSets][3] = d;
			(*g).sizeSets++;
			}
		}	
	}
	break;
	}
	}
}
void handleFound(struct Game *g, unsigned char set[4]){
	//TODO fix boolCards
	(*g).timeFound[(*g).sizeSetsFound] = OGGetAbsoluteTime();
	for(int i=0;i<4;i++) (*g).setsFound[(*g).sizeSetsFound][i]=set[i];
	int cardsPerSet;
	int defaultSizeCards=12;
	switch ((*g).mode){
		case 0:cardsPerSet=3;break;
		case 1:cardsPerSet=3;break;
		case 2:cardsPerSet=4;break;
	}
	if ((*g).mode==1){
		defaultSizeCards=15;
		if ((*g).sizeSetsFound == 0){
			//move cards
			for (int i=(*g).sizeCards-1;i>-1;i--){
				(*g).cards[i+3]=(*g).cards[i];
			}
			(*g).sizeCards+=3;
		}
		else
			for (int i=0;i<3;i++)
				(*g).boolCards[(*g).cards[i]]=false;
		for (int i=0;i<3;i++)
			(*g).cards[i] = set[i];
		
	}
	for (int j=0;j<cardsPerSet;j++){
	if ((*g).remainingCards>0 && (*g).sizeCards <= defaultSizeCards){
		//replace set
		for(int i=(*g).mode!=1?0:3;i<(*g).sizeCards;i++){
		if ((*g).cards[i]==set[j]){
		(*g).boolCards[set[j]]=false;
		(*g).cards[i]=(*g).deck[81-(*g).remainingCards];
		(*g).boolCards[(*g).deck[81-(*g).remainingCards]]=true;
		(*g).remainingCards--;
		break;
		}
		}
	} else  {
		//remove set
		for(int i=((*g).mode!=1)?0:3;i<(*g).sizeCards;i++){
		if ((*g).cards[i]==set[j]){
		(*g).boolCards[(*g).cards[i]]=false;
		//move cards
		for(int k=i;k<(*g).sizeCards-1;k++){
			(*g).cards[k]=(*g).cards[k+1];
		}
		(*g).sizeCards--;
		break;
		}
		}
	}
	}
	if ((*g).mode == 1)
		for (int i=0;i<3;i++)
			(*g).boolCards[set[i]]=true;
	(*g).sizeSetsFound++;
	findSets(g);
	if ((*g).sizeSets==0)
		addCards(g);
}
struct Game initGame(int mode){
	struct Game g;
	g.mode=mode;
	g.sizeCards=12;
	g.sizeSetsFound=0;
	g.sizeSets=0;
	g.remainingCards=69;
	g.startTime = OGGetAbsoluteTime();
	srand((long)(g.startTime));
	//srand(1710620931);
	//srand(1710740959);//2 6 13; ecken; seitenmitten; fixed by 60dc847 (v0.1.5)
	//srand(1710763402);//12 10 8; 10 8 5; fixed by 60dc847 (v0.1.5)
	generateDeck(g.deck);
	for(int i=0;i<81;i++) g.boolCards[i]=false;
	for (int i=0;i<12;i++){
		g.cards[i] = g.deck[i];
		g.boolCards[g.deck[i]]=true;
	}
	findSets(&g);
	if (g.sizeSets==0)
		addCards(&g);
	return g;
}
void testBoolCards(struct  Game *g){
	printf("Testing bools, sizeSetsFound %i \n",(*g).sizeSetsFound);
	int boolCardCount = 0;
	for (int i = 0; i<81; i++){
		if ((*g).boolCards[i]) boolCardCount++;
	}
	for (int i=0;i<(*g).sizeCards;i++){
		if (!(*g).boolCards[(*g).cards[i]])
			printf("eine Karte existiert in boolCards nicht \n");
	}
	if (boolCardCount != (*g).sizeCards)
		printf("boolCards hat %i Karten weniger \n",(*g).sizeCards-boolCardCount);
}
