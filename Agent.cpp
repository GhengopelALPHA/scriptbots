#include "Agent.h"

#include "settings.h"
#include "helpers.h"
#include <stdio.h>
//#include <iostream>
#include <string>
#include "DWRAONBrain.h"
#include "MLPBrain.h"
#include "AssemblyBrain.h"

using namespace std;
Agent::Agent()
{
	//randomly spawned bots get the following attributes:
	pos= Vector2f(randf(0,conf::WIDTH),randf(0,conf::HEIGHT));
	angle= randf(-M_PI,M_PI);
	health= 1+randf(0.5,0.75);
	age=0;
	species=randi(-conf::NUMBOTS*200,conf::NUMBOTS*200); //related to numbots because it's a good relationship
	radius= randf(conf::MEANRADIUS*0.2,conf::MEANRADIUS*2.2);
	spikeLength= 0;
	jawPosition= 0;
	grabID= -1;
	grabbing= 0;
	red= 0.5;
	gre= 0.5;
	blu= 0.5;
	w1=0;
	w2=0;
	volume=1;
	give=0;
	clockf1= randf(5,100);
	clockf2= randf(5,100);
	boost=false;
	jump= 0;
	indicator=0;
	gencount=0;
	ir=0;
	ig=0;
	ib=0;
	temperature_preference= cap(randn(2.0*abs(pos.y/conf::HEIGHT - 0.5),0.05));
	lungs= randf(0,1);
	hybrid= false;
	numbabies= randi(1,4);
	sexproject= false;
	repcounter= conf::REPRATE;
	metabolism= 1.0;//randf(0.5,2);

	float herb,carn,frui;
	int overtype= randi(0,Stomach::FOOD_TYPES-1);
	herb= randf(0,1);
	carn= randf(0,1);
	frui= randf(0,1);
	stomach[Stomach::PLANT]= herb;
	stomach[Stomach::MEAT]= carn;
	stomach[Stomach::FRUIT]= frui;
	if (herb+carn+frui < 1){
		stomach[overtype]+= 1-herb-carn-frui;
		cap(stomach[overtype]);
	}

	id=0;
	
	smell_mod= randf(0.1, 3);
	sound_mod= randf(0.1, 3);
	hear_mod= randf(0.1, 3);
	eye_see_agent_mod= randf(0.1, 3);
//	eye_see_cell_mod= randf(0.1, 2);
	blood_mod= randf(0.1, 3);
	
	MUTRATE1= randf(0.08, 0.12); //chance of mutations occuring
	MUTRATE2= 0.005; //randf(0.001, 0.01); //severity of mutations

	freshkill= 0;
	
	in.resize(Input::INPUT_SIZE, 0);
	out.resize(Output::OUTPUT_SIZE, 0);

	brainact= 0;
	
	eardir.resize(NUMEARS, 0);
	for(int i=0;i<NUMEARS;i++) {
		eardir[i] = randf(0, 2*M_PI);
	}

	eyefov.resize(NUMEYES, 0);
	eyedir.resize(NUMEYES, 0);
	for(int i=0;i<NUMEYES;i++) {
		eyefov[i] = randf(0.5, 2);
		eyedir[i] = randf(0, 2*M_PI);
	}

	children= 0;
	killed= 0;
	hits= 0;

	death= "Killed by Unknown Factor!"; //default death message, just in case
}

void Agent::printSelf()
{
	printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	printf("AGENT, ID: %i\n", id);
	printf("Species # = %i\n", species);
	printf("Metabolism = %f\n", metabolism);
	printf("Lungs = %f\n", lungs);
	printf("MUTRATE1 = %f, MUTRATE2 = %f\n", MUTRATE1, MUTRATE2);
	printf("children= %i\n", children);
	printf("killed= %i\n", killed);
	if (isHerbivore()) printf("Herbivore\n");
	if (isCarnivore()) printf("Carnivore\n");
	if (isFrugivore()) printf("Frugivore\n");
	for (int i=0; i<(int)mutations.size(); i++) {
		cout << mutations[i];
	}
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

void Agent::initEvent(float size, float r, float g, float b)
{
	indicator=size;
	ir=r;
	ig=g;
	ib=b;
}

void Agent::setActivity()
{
	brainact= brain.getActivity();
}

void Agent::tick()
{
	brain.tick(in, out);
}
Agent Agent::reproduce(Agent that, float MR, float MR2)
{
	//create baby. Note that if the bot selects itself to mate with, this function acts also as assexual reproduction
	//NOTES: Agent "this" is mother, Agent "that" is father, Agent "a2" is daughter
	//if a single parent's trait is required, use the mother's (this->)
	Agent a2;

	//spawn the baby somewhere closeby behind the mother
	//we want to spawn behind so that agents dont accidentally kill their young right away
	//note that this relies on bots actally driving forward, not backward. Well let natural selection choose who lives and who dies
	Vector2f fb(this->radius*2.5,0);
	fb.rotate(this->angle+M_PI+this->numbabies*randf(-0.4,0.4));
	a2.pos= this->pos + fb;
	a2.borderRectify();

	//basic trait inheritance
	a2.gencount= max(this->gencount+1,that.gencount+1);
	a2.numbabies= randf(0,1)<0.5 ? this->numbabies : that.numbabies;
	a2.metabolism= randf(0,1)<0.5 ? this->metabolism : that.metabolism;
	a2.stomach[Stomach::PLANT]= randf(0,1)<0.5 ? this->stomach[Stomach::PLANT]: that.stomach[Stomach::PLANT];
	a2.stomach[Stomach::MEAT]= randf(0,1)<0.5 ? this->stomach[Stomach::MEAT]: that.stomach[Stomach::MEAT];
	a2.stomach[Stomach::FRUIT]= randf(0,1)<0.5 ? this->stomach[Stomach::FRUIT]: that.stomach[Stomach::FRUIT];
	a2.species= randf(0,1)<0.5 ? this->species : that.species;
	a2.radius= randf(0,1)<0.5 ? this->radius : that.radius;

	a2.MUTRATE1= randf(0,1)<0.5 ? this->MUTRATE1 : that.MUTRATE1;
	a2.MUTRATE2= randf(0,1)<0.5 ? this->MUTRATE2 : that.MUTRATE2;
	a2.clockf1= randf(0,1)<0.5 ? this->clockf1 : that.clockf1;
	a2.clockf2= randf(0,1)<0.5 ? this->clockf2 : that.clockf2;

	a2.smell_mod = randf(0,1)<0.5 ? this->smell_mod : that.smell_mod;
	a2.sound_mod = randf(0,1)<0.5 ? this->sound_mod : that.sound_mod;
	a2.hear_mod = randf(0,1)<0.5 ? this->hear_mod : that.hear_mod;
	a2.eye_see_agent_mod = randf(0,1)<0.5 ? this->eye_see_agent_mod : that.eye_see_agent_mod;
//	a2.eye_see_cell_mod = randf(0,1)<0.5 ? this->eye_see_cell_mod : that.eye_see_cell_mod;
	a2.blood_mod = randf(0,1)<0.5 ? this->blood_mod : that.blood_mod;

	a2.temperature_preference= randf(0,1)<0.5 ? this->temperature_preference : that.temperature_preference;
	a2.lungs= randf(0,1)<0.5 ? this->lungs : that.lungs;
	
	a2.eardir = randf(0,1)<0.5 ? this->eardir : that.eardir;

	a2.eyefov = randf(0,1)<0.5 ? this->eyefov : that.eyefov;
	a2.eyedir = randf(0,1)<0.5 ? this->eyedir : that.eyedir;

	//mutations
	if (randf(0,1)<MR/10) a2.numbabies= (int) (randn(a2.numbabies, MR2*40));
	if (a2.numbabies<1) a2.numbabies= 1; //technically, 0 babies is perfectly logical, but in this sim it is pointless, so it's disallowed
	if (randf(0,1)<MR/5) a2.metabolism= abs(randn(a2.metabolism, MR2*5)); //don't let metabolism reach zero with absolute
	if (a2.metabolism>conf::MAXMETABOLISM) a2.metabolism= conf::MAXMETABOLISM;
	if (randf(0,1)<MR*5) a2.stomach[Stomach::PLANT]= cap(randn(a2.stomach[Stomach::PLANT], MR2*5));
	if (randf(0,1)<MR*5) a2.stomach[Stomach::MEAT]= cap(randn(a2.stomach[Stomach::MEAT], MR2*5));
	if (randf(0,1)<MR*5) a2.stomach[Stomach::FRUIT]= cap(randn(a2.stomach[Stomach::FRUIT], MR2*5));
	if (randf(0,1)<MR*20) a2.species+= (int) (randn(0, MR2*120));
	if (randf(0,1)<MR*5) a2.radius= randn(a2.radius, MR2*8);
	if (a2.radius<1) a2.radius= 1;

	if (randf(0,1)<MR) a2.MUTRATE1= abs(randn(a2.MUTRATE1, conf::METAMUTRATE1));
	if (randf(0,1)<MR) a2.MUTRATE2= abs(randn(a2.MUTRATE2, conf::METAMUTRATE2));
	//we dont really want mutrates to get to zero; thats too stable. so take absolute randn instead.

	if (randf(0,1)<MR) a2.clockf1= randn(a2.clockf1, MR2/4);
	if (a2.clockf1<2) a2.clockf1= 2;
	if (randf(0,1)<MR) a2.clockf2= randn(a2.clockf2, MR2/4);
	if (a2.clockf2<2) a2.clockf2= 2;

	if (randf(0,1)<MR) a2.smell_mod= randn(a2.smell_mod, MR2);
	if (randf(0,1)<MR) a2.sound_mod= randn(a2.sound_mod, MR2);
	if (randf(0,1)<MR) a2.hear_mod= randn(a2.hear_mod, MR2);
	if (randf(0,1)<MR) a2.eye_see_agent_mod= randn(a2.eye_see_agent_mod, MR2);
//	if (randf(0,1)<MR) a2.eye_see_cell_mod= randn(a2.eye_see_cell_mod, MR2);
	if (randf(0,1)<MR) a2.blood_mod= randn(a2.blood_mod, MR2);

	if (randf(0,1)<MR*5) a2.temperature_preference= cap(randn(a2.temperature_preference, MR2*2));
	if (randf(0,1)<MR*2) a2.lungs= cap(randn(a2.lungs, MR2*2));

	for(int i=0;i<NUMEARS;i++){
		if(randf(0,1)<MR*2) a2.eardir[i] = randn(a2.eardir[i], MR2*5);
		if(a2.eardir[i]<0) a2.eardir[i] = 0;
		if(a2.eardir[i]>2*M_PI) a2.eardir[i] = 2*M_PI;
	}

	#pragma omp parallel for
	for(int i=0;i<NUMEYES;i++){
		if(randf(0,1)<MR) a2.eyefov[i] = randn(a2.eyefov[i], MR2/2);
		if(a2.eyefov[i]<0) a2.eyefov[i] = 0;
		if(a2.eyefov[i]>M_PI) a2.eyefov[i] = M_PI; //eyes cannot wrap around bot
		
		if(randf(0,1)<MR*2) a2.eyedir[i] = randn(a2.eyedir[i], MR2*5);
		if(a2.eyedir[i]<0) a2.eyedir[i] = 0;
		if(a2.eyedir[i]>2*M_PI) a2.eyedir[i] = 2*M_PI;
		//not going to loop coordinates; 0,2pi is bots front, so it provides a good point to "bounce" off of
	}
	
	//create brain here
	a2.brain= this->brain.crossover(that.brain);
	a2.brain.initMutate(MR,MR2);

	a2.initEvent(10,0.8,0.8,0.8); //grey event means we were just born! Welcome!
	
	return a2;

}

void Agent::setHerbivore()
{
	this->stomach[Stomach::PLANT]= randf(0.7, 1);
	this->stomach[Stomach::MEAT]= randf(0, 0.3);
	this->stomach[Stomach::FRUIT]= randf(0, 0.3);
}

void Agent::setCarnivore()
{
	this->stomach[Stomach::PLANT]= randf(0, 0.3);
	this->stomach[Stomach::MEAT]= randf(0.7, 1);
	this->stomach[Stomach::FRUIT]= randf(0, 0.3);
}

void Agent::setFrugivore()
{
	this->stomach[Stomach::PLANT]= randf(0, 0.3);
	this->stomach[Stomach::MEAT]= randf(0, 0.3);
	this->stomach[Stomach::FRUIT]= randf(0.7, 1);
}

void Agent::setPos(float x, float y)
{
	this->pos.x= x;
	this->pos.y= y;
}

void Agent::borderRectify()
{
	//if this agent has fallen outside of the world borders, rectify and wrap him to the other side
	if (this->pos.x<0)			  this->pos.x= this->pos.x + conf::WIDTH;
	if (this->pos.x>conf::WIDTH)  this->pos.x= this->pos.x - conf::WIDTH;
	if (this->pos.y<0)			  this->pos.y= this->pos.y + conf::HEIGHT;
	if (this->pos.y>conf::HEIGHT) this->pos.y= this->pos.y - conf::HEIGHT;
}

bool Agent::isHerbivore()
{
	if (stomach[Stomach::PLANT]>=stomach[Stomach::MEAT] && stomach[Stomach::PLANT]>=stomach[Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isCarnivore()
{
	if (stomach[Stomach::MEAT]>=stomach[Stomach::PLANT] && stomach[Stomach::MEAT]>=stomach[Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isFrugivore()
{
	if (stomach[Stomach::FRUIT]>=stomach[Stomach::PLANT] && stomach[Stomach::FRUIT]>=stomach[Stomach::MEAT]) return true;
	return false;
}

void Agent::writeIfKilled(const char * cause)
/*======LIST OF CURRENT DEATH CAUSES=======//
"Killed by Unknown Factor!			Agent.cpp	~ln 87  Note: if no other message is applied, this will pop up
"Killed by Something ?"				World.cpp	~ln 210 Note: the space is for easy interpretation by Excel with text->data delimitation via spaces
"Killed by Spike Raising"			World.cpp	~ln 515
"Killed by a Hazard"				World.cpp	~ln 620
"Killed by Suffocation ."			World.cpp	~ln 632	Note: the space is for easy interpretation by Excel with text->data delimitation via spaces
"Killed by Excessive Generosity"	World.cpp	~ln 658
"Killed by a Collision"				World.cpp	~ln 687-9
"Killed by a Stabbing"				World.cpp	~ln 754 Note: death by spike
"Killed by a Chomping"				World.cpp	~ln 797 Note: death by jaws
"Killed by Natural Causes"			World.cpp	~ln 808	Note: contains wheel loss, aging, metabolism, boost penalty, and brain use
"Killed by Temp Discomfort"			World.cpp	~ln 816
"Killed by LackOf Oxygen"			World.cpp	~ln 821
"Killed by God (you)"				World.cpp	~ln 927
"Killed by Child Birth"				World.cpp	~ln 1069
*/
{
	if(this->health<0){
		this->death= cause;
		this->health= 0;
	}
}