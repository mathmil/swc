#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
//card: unsigned char c; 0<=card<=80; farbe, form, fuellung, anzahl; color shape, filling, number
struct Game{
	int mode; 			//mode 0 = normal, 1=chain, 2=ultra
	int sizeCards;			//anzahl karten ufm tisch chain: mit dem set
	int sizeSets;			//anzahl sets ufm tisch
	int sizeSetsFound;		//anzahl bisher gefundener sets=3*verbrauchte karten
	int remainingCards; //cards remaining in Deck
	unsigned char deck[81];		//alle karten, gemischt 
	unsigned char cards[27];	//karten ufm tisch, bei chain ersten 3 letztes set
								//swf keybinds say its below 27
	bool boolCards[81];		//karten ufm tisch
	unsigned char setsFound[40][4];	//bisher gefundene sets
	//sets ufm tisch, 100 ist ne geschaetzte obergrenze fuer chain und normal }
	//sortiert nach Kartenwert
	double timeFound[40];
	unsigned char sets[100][4];
};
unsigned char conjugateCard(unsigned char a, unsigned char b); //no side effects, TODO replace with faster pregenerated arry
void generateDeck(unsigned char * deck);	//modifies deck; shuffles
void findSets(struct Game *g);			//modifies sets; chains need testing
void addCards(struct Game *g);			//modifies cards, indirectly sets, unfinshed, unused
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
unsigned char conjugateCard(unsigned char a, unsigned char b){
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
	while ((*g).sizeSets >0){
	continue;
	}
	return;
}
void findSets(struct Game *g){	//nice way to find every set without finding one twice?
	//recalc boolCards
	//if ((*g).mode == 1){ 
		for (int i = 0; i<81; i++){
			(*g).boolCards[i]=false;
		}
		for (int i=0;i<(*g).sizeCards;i++){
			(*g).boolCards[(*g).cards[i]]=true;
		}
	//}
	//testBoolCards(g);	
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
		if (a<b && b<c && (*g).boolCards[c]){	//if optimized needs a different uniqueness check
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
		if (b<c && (*g).boolCards[c]){	//if optimized needs a different uniqueness check
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
	case 2:{
	//calc conjugates
	int i1; int i2; int j1; int j2;
	unsigned char conjugates[78];
	int pairCount = ((*g).sizeCards)*((*g).sizeCards+1)/2;
	for(int i=0;i<pairCount;i++){
		i1 = (int)((sqrt(8*i+1)-1)/2);//index first card, n
		i2 = i-(int)(i1*(i1+1)/2);
		conjugates[i]=conjugateCard(
		(*g).cards[i1],(*g).cards[i2]);
		for (int j=0;j<i;j++){
			if(conjugates[i]==conjugates[j]){
			j1 = (int)((sqrt(8*j+1)-1)/2);//index first card, n
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
	//for(int j=0; j<11;j++){for(int k=j+1;k<12;k++){
	//	conjugates[(int)(j*(j+1)/2)+k]=conjugateCard((*g).cards[j],(*g).cards[k]);
	//}}

	break;
	}
	}
}
void handleFound(struct Game *g, unsigned char set[4]){
	(*g).timeFound[(*g).sizeSetsFound] = OGGetAbsoluteTime();
	int cardsPerSet;
	int defaultSizeCards=12;
	switch ((*g).mode){
		case 0:cardsPerSet=3;break;
		case 1:cardsPerSet=3;break;
		case 2:cardsPerSet=4;break;
	}
	if ((*g).mode==1){
		defaultSizeCards=15;
		if ((*g).sizeSetsFound == 0){ //move cards
			for (int i=(*g).sizeCards-1;i>-1;i--){
				(*g).cards[i+3]=(*g).cards[i];
			}
			(*g).sizeCards+=3;
		}
		for (int i=0;i<3;i++){
			(*g).boolCards[(*g).cards[i]]=false;
			(*g).cards[i] = set[i];
		}
	}
	for (int j=0;j<cardsPerSet;j++){//für alle karten im set
	if ((*g).remainingCards>0 && (*g).sizeCards <= defaultSizeCards){//karten ersetzen
		for(int i=(*g).mode!=1?0:3;i<(*g).sizeCards;i++){
		if ((*g).cards[i]==set[j]){
		(*g).boolCards[set[j]]=false;
		(*g).cards[i]=(*g).deck[81-(*g).remainingCards];
		(*g).boolCards[(*g).deck[81-(*g).remainingCards]]=true;
		(*g).remainingCards--;
		break;
		}
		}
	} else  { //karten nur entfernen & aufrutschen, nicht ersetzen
		for(int i=(*g).mode!=1?0:3;i<(*g).sizeCards;i++){//karte finden
		if ((*g).cards[i]==set[j]){
		(*g).boolCards[(*g).cards[i]]=false;
		for(int k=i;k<(*g).sizeCards-1;k++){//karten rutschen
		(*g).cards[k]=(*g).cards[k+1];
		}
		(*g).sizeCards--;
		//(*g).remainingCards--;
		break;
		}
		}
	}
	}
	if ((*g).mode == 1)
		for (int i=0;i<3;i++)
			(*g).boolCards[set[i]]=true;
	//log set
	for(int i=0;i<4;i++) (*g).setsFound[(*g).sizeSetsFound][i]=set[i];
	
	(*g).sizeSetsFound++;
	//add more cards if neccessary
	findSets(g);
	while (((*g).sizeSets==0 || (*g).sizeCards%3!=0) && (*g).remainingCards>0){
	// || (*g).sizeCards<defaultSizeCards 
	(*g).cards[(*g).sizeCards]=(*g).deck[81-(*g).remainingCards];
	(*g).boolCards[(*g).deck[81-(*g).remainingCards]]=true;
	(*g).sizeCards++;
	(*g).remainingCards--;
	findSets(g);
	}
}
struct Game initGame(int mode){
	struct Game g;
	g.mode=mode;
	g.sizeCards=12;
	g.sizeSetsFound=0;
	g.sizeSets=0;
	g.remainingCards=69;
	generateDeck(g.deck);
	for(int i=0;i<81;i++) g.boolCards[i]=false;
	for (int i=0;i<12;i++){
		g.cards[i] = g.deck[i];
		g.boolCards[g.deck[i]]=true;
	}
	findSets(&g);
	while (g.sizeSets==0){//add more (than 12) cards if neccessary
	for (int i=0;i<3;i++){
	g.cards[g.sizeCards]=g.deck[g.sizeCards];
	g.boolCards[g.deck[g.sizeCards]]=true;
	g.sizeCards++;
	}
	findSets(&g);
	}
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
