#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
//card: uint8_t c; 0<=card<=80; farbe, form, fuellung, anzahl; color, shape, filling, number
enum gamemode {normal = 0, chain = 1, ultra = 2};
struct Game{
	enum gamemode mode;
	int sizeCards;
	int sizeSets;
	int sizeSetsFound;
	int remainingCards;//cards remaining in Deck
	uint8_t deck[81];//all cards, shuffled
	uint8_t cards[45];//cards on table, including previous set in chain
	bool boolCards[81];     //cards on table
	uint8_t setsFound[40][4];//sets Found by player
	double startTime;
	double timeFound[40];
	//sets on table
	uint8_t sets[200][4];
};
uint8_t conjugateCard(uint8_t a, uint8_t b);//no side effects
void generateDeck(uint8_t * deck, int32_t seed);//modifies deck; shuffles
void findSets(struct Game *g);//modifies sets
void addCards(struct Game *g);//modifies cards, indirectly sets
void handleFound(struct Game *g, uint8_t set[4]);//modifies everything
struct Game initGame(enum gamemode mode);
void testBoolCards(struct  Game *g);
bool isSet(uint8_t *cards, enum gamemode mode);
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
		const int cardsPerSet = (g.mode==ultra)?4:2;
		for(int i=0;i<g.sizeSetsFound;i++){
			for(int j=0;j<cardsPerSet;j++){
				fputc(g.setsFound[i][j],file);length++;
			}
		}
		fseek(file,-1,SEEK_CUR);
		fputc(128+g.setsFound[g.sizeSetsFound-1][cardsPerSet-1],file);
	}
	//times in millis since last set in LEB128
	unsigned long long timeSet;
	unsigned long long totalTimeSets = 0;
	unsigned char byte;
	for(int i=0;i<g.sizeSetsFound;i++){
		timeSet = (long long)(1000*(g.timeFound[i]-g.startTime)-totalTimeSets);
		printf("%llu %llu secretValue \n",totalTimeSets, timeSet);
		totalTimeSets += timeSet;
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
uint8_t conjugateCard(uint8_t a, uint8_t b){
	//TODO replace with precalculated array[1080]
	uint8_t c = 0;
	for (int i=1;i<81;i*=3){
		c+=(81-(a%3+b%3))%3*i;
		a=a/3;
		b=b/3;
	}
	return c;
}
bool isSet(uint8_t *cards, enum gamemode mode){
	switch(mode){
	case ultra:
		if (conjugateCard(cards[0],cards[1]) ==
		    conjugateCard(cards[2],cards[3]) ||
		    conjugateCard(cards[0],cards[2]) ==
		    conjugateCard(cards[1],cards[3]) ||
		    conjugateCard(cards[0],cards[3]) ==
		    conjugateCard(cards[1],cards[2]))
			return true;
		return false;
		break;
	default:
		if (cards[2] == conjugateCard(cards[0],cards[1]))
			return true;
		return false;
		break;
	}
}
void generateDeck(uint8_t * deck, int32_t seed){
	//glibc PRNG, hardcoded because consist random numbers are required
	int32_t r[425];
	size_t i;
	r[0] = seed;
	for (i=1; i<31; i++) {
	  r[i] = (16807LL * r[i-1]) % 2147483647;
	  if (r[i] < 0) {
	    r[i] += 2147483647;
	  }
	}
	for (i=31; i<34; i++) {
	  r[i] = r[i-31];
	}
	for (i=34; i<344; i++) {
	  r[i] = r[i-31] + r[i-3];
	}
	for (i=344; i<425; i++) {
	  r[i] = r[i-31] + r[i-3];
	}
	//Fisher-Yates shuffle
	for (i=0;i<81;i++){
		int j = (((unsigned int)r[i+344]) >> 1)%(i+1);
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
	uint8_t a;	//currently all 6 permutations checked, only sorted added
	uint8_t b;	//could improve speed at least 2x
	uint8_t c;
	uint8_t d;
	switch ((*g).mode){
	case normal:{
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
	case chain:{
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
	case ultra:{
	//TODO cache conjugates, "hashmap" for conjugates
	int i1; int i2; int j1; int j2;
	uint8_t conjugates[78];
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
void handleFound(struct Game *g, uint8_t set[4]){
	//TODO fix boolCards
	(*g).timeFound[(*g).sizeSetsFound] = OGGetAbsoluteTime();
	for(int i=0;i<4;i++) (*g).setsFound[(*g).sizeSetsFound][i]=set[i];
	int cardsPerSet;
	int defaultSizeCards=12;
	switch ((*g).mode){
		case ultra:cardsPerSet=4;break;
		default:cardsPerSet=3;break;
	}
	if ((*g).mode==chain){
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
		for(int i=(*g).mode!=chain?0:3;i<(*g).sizeCards;i++){
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
		for(int i=((*g).mode!=chain)?0:3;i<(*g).sizeCards;i++){
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
	if ((*g).mode == chain)
		for (int i=0;i<3;i++)
			(*g).boolCards[set[i]]=true;
	(*g).sizeSetsFound++;
	findSets(g);
	if ((*g).sizeSets==0)
		addCards(g);
}
struct Game initGame(enum gamemode mode){
	struct Game g;
	g.mode=mode;
	g.sizeCards=12;
	g.sizeSetsFound=0;
	g.sizeSets=0;
	g.remainingCards=69;
	g.startTime = OGGetAbsoluteTime();
	generateDeck(g.deck, (int32_t)g.startTime);
	for(int i=0;i<81;i++) g.boolCards[i]=false;
	for (int i=0;i<12;i++){
		g.cards[i] = g.deck[i];
		g.boolCards[g.deck[i]]=true;
	}
	findSets(&g);
	while (g.sizeSets==0){
		addCards(&g);
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
