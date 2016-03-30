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
	health= 0.5+randf(0,0.5);
	age=0;
	species=randi(-conf::NUMBOTS*20,conf::NUMBOTS*20); //related to numbots because it's a good relationship
	radius= randf(conf::MEANRADIUS*0.2,conf::MEANRADIUS*1.8);
	spikeLength=0;
	red= 0;
	gre= 0;
	blu= 0;
	w1=0;
	w2=0;
	soundmul=1;
	give=0;
	clockf1= randf(5,100);
	clockf2= randf(5,100);
	boost=false;
	jump= 0;
	indicator=0;
	gencount=0;
	selectflag=0;
	ir=0;
	ig=0;
	ib=0;
	temperature_preference= cap(randn(2.0*abs(pos.y/conf::HEIGHT - 0.5),0.05));
	lungs= randf(0,1);
	hybrid= false;
	stomach[0]= randf(0,1);
	stomach[1]= randf(0,1);
	stomach[2]= randf(0,1);
	repcounter= conf::REPRATE;
	metabolism= randf(0.2,conf::MAXMETABOLISM);

	id=0;
	
	smell_mod= randf(0.1, 0.5);
	sound_mod= randf(0.5, 1.6);
	hear_mod= randf(0.5, 2);
	eye_see_agent_mod= randf(0.5, 3);
	eye_see_cell_mod= randf(0.1, 2);
	blood_mod= randf(0.5, 3);
	
	MUTRATE1= 0.09; //randf(0.001, 0.005); //chance of mutations occuring
	MUTRATE2= 0.005; //randf(0.01, 0.5); //severity of mutations

	spiked= false;
	
	in.resize(INPUTSIZE, 0);
	out.resize(OUTPUTSIZE, 0);

	brainActivity= 0;
	
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
}

void Agent::printSelf()
{
	printf("\nAGENT, ID: %i\n", id);
	printf("Species # = %i\n", species);
	printf("MUTRATE1 = %f, MUTRATE2 = %f\n", MUTRATE1, MUTRATE2);
	printf("children= %i\n", children);
	for (int i=0;i<mutations.size();i++) {
		cout << mutations[i];
	}
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
	brainActivity= brain.getActivity();
}

void Agent::tick()
{
	brain.tick(in, out);
}
Agent Agent::reproduce(Agent that, float MR, float MR2)
{
	//create baby. Note that if the bot selects itself to mate with, this function acts also as asexual reproduction
	//NOTES: Agent "this" is mother, Agent "that" is father, Agent "a2" is daughter
	//if a single parent's trait is required, use the mother's (this->)
	Agent a2;

	//spawn the baby somewhere closeby behind agent
	//we want to spawn behind so that agents dont accidentally eat their young right away
	Vector2f fb(this->radius*3,0);
	fb.rotate(a2.angle+M_PI);
	a2.pos= this->pos + fb + Vector2f(randf(-this->radius*2,this->radius*2), randf(-this->radius*2,this->radius*2));
	if (a2.pos.x<0) a2.pos.x= conf::WIDTH+a2.pos.x;
	if (a2.pos.x>conf::WIDTH) a2.pos.x= a2.pos.x-conf::WIDTH;
	if (a2.pos.y<0) a2.pos.y= conf::HEIGHT+a2.pos.y;
	if (a2.pos.y>conf::HEIGHT) a2.pos.y= a2.pos.y-conf::HEIGHT;

	//basic trait inheritance
	a2.gencount= max(this->gencount+1,that.gencount+1);
	a2.metabolism= randf(0,1)<0.5 ? this->metabolism : that.metabolism;
	a2.stomach[0]= randf(0,1)<0.5 ? this->stomach[0]: that.stomach[0];
	a2.stomach[1]= randf(0,1)<0.5 ? this->stomach[1]: that.stomach[1];
	a2.stomach[2]= randf(0,1)<0.5 ? this->stomach[2]: that.stomach[2];
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
	a2.eye_see_cell_mod = randf(0,1)<0.5 ? this->eye_see_cell_mod : that.eye_see_cell_mod;
	a2.blood_mod = randf(0,1)<0.5 ? this->blood_mod : that.blood_mod;

	a2.temperature_preference= randf(0,1)<0.5 ? this->temperature_preference : that.temperature_preference;
	a2.lungs= randf(0,1)<0.5 ? this->lungs : that.lungs;
	
	a2.eardir = randf(0,1)<0.5 ? this->eardir : that.eardir;

	a2.eyefov = randf(0,1)<0.5 ? this->eyefov : that.eyefov;
	a2.eyedir = randf(0,1)<0.5 ? this->eyedir : that.eyedir;

	//mutations
	if (randf(0,1)<MR/2) a2.metabolism= randn(a2.metabolism, MR2*5);
	if (a2.metabolism<0) a2.metabolism= 0;
	if (a2.metabolism>conf::MAXMETABOLISM) a2.metabolism= conf::MAXMETABOLISM;
	if (randf(0,1)<MR*5) a2.stomach[0]= cap(randn(a2.stomach[0], MR2*5));
	if (randf(0,1)<MR*5) a2.stomach[1]= cap(randn(a2.stomach[1], MR2*5));
	if (randf(0,1)<MR*5) a2.stomach[2]= cap(randn(a2.stomach[2], MR2*5));
	if (randf(0,1)<MR*100) a2.species= (int) randn(a2.species, MR2*100);
	if (randf(0,1)<MR*5) a2.radius= randn(a2.radius, MR2*3);
	if (a2.radius<1) a2.radius= 1;

	if (randf(0,1)<MR) a2.MUTRATE1= abs(randn(a2.MUTRATE1, conf::METAMUTRATE1));
	if (randf(0,1)<MR) a2.MUTRATE2= abs(randn(a2.MUTRATE2, conf::METAMUTRATE2));
	//we dont really want mutrates to get to zero; thats too stable. so take absolute randn instead.

	if (randf(0,1)<MR*5) a2.clockf1= randn(a2.clockf1, MR2/4);
	if (a2.clockf1<2) a2.clockf1= 2;
	if (randf(0,1)<MR*5) a2.clockf2= randn(a2.clockf2, MR2/4);
	if (a2.clockf2<2) a2.clockf2= 2;

	if (randf(0,1)<MR) a2.smell_mod= randn(a2.smell_mod, MR2);
	if (randf(0,1)<MR) a2.sound_mod= randn(a2.sound_mod, MR2);
	if (randf(0,1)<MR) a2.hear_mod= randn(a2.hear_mod, MR2);
	if (randf(0,1)<MR) a2.eye_see_agent_mod= randn(a2.eye_see_agent_mod, MR2);
	if (randf(0,1)<MR) a2.eye_see_cell_mod= randn(a2.eye_see_cell_mod, MR2);
	if (randf(0,1)<MR) a2.blood_mod= randn(a2.blood_mod, MR2);

	if (randf(0,1)<MR*5) a2.temperature_preference= cap(randn(a2.temperature_preference, MR2*3));
	if (randf(0,1)<MR*5) a2.lungs= cap(randn(a2.lungs, MR2*3));

	for(int i=0;i<NUMEARS;i++){
		if(randf(0,1)<MR*2) a2.eardir[i] = randn(a2.eardir[i], MR2*5);
		if(a2.eardir[i]<0) a2.eardir[i] = 0;
		if(a2.eardir[i]>2*M_PI) a2.eardir[i] = 2*M_PI;
	}

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
	
	return a2;

}