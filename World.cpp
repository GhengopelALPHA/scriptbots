#include "World.h"

#include <ctime>

#include "settings.h"
#include "helpers.h"
#include "vmath.h"
#include <stdio.h>
#include <iostream>

using namespace std;

World::World() :
		CLOSED(false),
		DEBUG(false),
		pcontrol(false),
		pright(0),
		pleft(0),
		pinput1(0)
{
	//inititalize
	reset();

	//add random food
	while(getFood()<conf::INITFOOD) {
		cells[0][randi(0,CW)][randi(0,CH)] = conf::FOODMAX;
	}
	
	addRandomBots(conf::NUMBOTS, 1);
}

void World::update()
{
	modcounter++;
	vector<int> dt;
	float tinit;
	float tfin;

	//Process periodic events
	//Age goes up!
	if (modcounter%100==0) {
		for (int i=0;i<agents.size();i++) {
			agents[i].age+= 1;
		}		
	}
	
	if (conf::REPORTS_PER_EPOCH>0 && (modcounter%(10000/conf::REPORTS_PER_EPOCH)==0)) {
		//write report and record counts
		numHerbivore[ptr]= getHerbivores();
		numCarnivore[ptr]= getCarnivores();
		numFrugivore[ptr]= getFrugivores();
		numHybrid[ptr]= getHybrids();
		numTotal[ptr]= getAgents();
		ptr++;
		if(ptr == conf::RECORD_SIZE) ptr = 0;

		writeReport();
	}

	if (modcounter>=conf::FRAMES_PER_EPOCH) {
		modcounter=0;
		current_epoch++;
	}
	if ((modcounter%conf::FOODADDFREQ==0 && !CLOSED) || getFood()<conf::MINFOOD) {
		cx=randi(0,CW);
		cy=randi(0,CH);
		cells[0][cx][cy]= conf::FOODMAX;
	}
	if (modcounter%conf::HAZARDFREQ==0) {
		cx=randi(0,CW);
		cy=randi(0,CH);
		cells[2][cx][cy]= conf::HAZARDMAX;
	}
	if (modcounter%conf::FRUITADDFREQ==0) {
		cx=randi(0,CW);
		cy=randi(0,CH);
		if (cells[0][cx][cy]> conf::FOODMAX*conf::FRUITREQUIRE) cells[3][cx][cy]= conf::FRUITMAX;
	}

	for(cx=0;cx<CW;cx++){
		for(cy=0;cy<CH;cy++){
			//food = cells[0]...
			if (cells[0][cx][cy]>0 && cells[0][cx][cy]<conf::FOODMAX) {
				cells[0][cx][cy]+= conf::FOODGROWTH; //food quantity is changed by FOODGROWTH
			}
			cells[0][cx][cy]= capCell(cells[0][cx][cy], conf::FOODMAX); //cap food at FOODMAX

			//meat = cells[1]...
			if (cells[1][cx][cy]>0 && cells[1][cx][cy]<=conf::MEATMAX) {
				cells[1][cx][cy] -= conf::MEATDECAY/cells[1][cx][cy]; //meat decays exponentially
			}
			cells[1][cx][cy]= capCell(cells[1][cx][cy], conf::MEATMAX); //cap at MEATMAX

			//hazard = cells[2]...
			if (cells[2][cx][cy]>0){
				cells[2][cx][cy]-= conf::HAZARDDECAY; //hazard decays
			}
			if (cells[2][cx][cy]>conf::HAZARDMAX*9/10 && randf(0,1)<0.125){
				cells[2][cx][cy]= 0; //instant hazards will be reset to zero
			}
			cells[2][cx][cy]= capCell(cells[2][cx][cy], conf::HAZARDMAX); //cap at HAZARDMAX

			//fruit = cells[3]...
			if (cells[0][cx][cy]<=conf::FOODMAX*conf::FRUITREQUIRE && cells[3][cx][cy]>0){
				cells[3][cx][cy]-= conf::FRUITDECAY; //fruit decays from lack of plant life
			}
			if (randf(0,1)<conf::FOODSPREAD && cells[3][cx][cy]>0.5*conf::FRUITMAX) {
				//food seeding
				int ox= randi(cx-1-conf::FOODRANGE,cx+2+conf::FOODRANGE);
				int oy= randi(cy-1-conf::FOODRANGE,cy+2+conf::FOODRANGE);
				if (ox<0) ox+= CW;
				if (ox>CW-1) ox-= CW;
				if (oy<0) oy+= CH;
				if (oy>CH-1) oy-= CH; //code up to this point ensures world edges are crossed and not skipped
				if (cells[0][ox][oy]<conf::FOODMAX*3/4) cells[0][ox][oy]+= conf::FOODMAX/4;
			}
			cells[3][cx][cy] = capCell(cells[3][cx][cy], conf::FRUITMAX); //cap
		}
	}
	
	//reset any counter variables per agent
	for(int i=0;i<agents.size();i++){
		//reset spiked flag
		agents[i].spiked= false;
	}
	for(int i=0;i<agents.size();i++){
		//process indicator (used in drawing)
		if(agents[i].indicator>0) agents[i].indicator -= 1;
	}
	for(int i=0;i<agents.size();i++){
		//reset dfood for processOutputs
		agents[i].dfood=0;
	}

	//give input to every agent. Sets in[] array
	setInputs();

	//brains tick. computes in[] -> out[]
	brainsTick();

	//read output and process consequences of bots on environment. requires out[]
	processOutputs();

	//process bots:
	for (int i=0;i<agents.size();i++) {
		//find brain activity on this tick
		agents[i].setActivity();

		//doLiveMutate
		float MR= agents[i].MUTRATE1;
		float MR2= agents[i].MUTRATE2;
		agents[i].brain.liveMutate(MR, MR2, agents[i].out);
	}

	for (int i=0;i<agents.size();i++) {
		//health and deaths
		float baseloss= conf::HEALTHLOSS_WHEELS*(abs(agents[i].w1) + abs(agents[i].w2))/2;

		baseloss *= agents[i].metabolism/conf::MAXMETABOLISM;

		baseloss += agents[i].age/conf::MAXAGE*conf::HEALTHLOSS_AGING; //getting older reduces health.

		if (agents[i].boost) { //if boosting, init baseloss + age loss is multiplied
			baseloss *= conf::HEALTHLOSS_BOOSTMULT;
		}

		//process temperature preferences
		//calculate temperature at the agents spot. (based on distance from horizontal equator)
		float dd= 2.0*abs(agents[i].pos.y/conf::HEIGHT - 0.5);
		float discomfort= sqrt(abs(dd-agents[i].temperature_preference));
		if (discomfort<0.08) discomfort=0;
		baseloss += conf::TEMPERATURE_DISCOMFORT*discomfort; //add to baseloss

		//brain activity reduces health
		baseloss += conf::HEALTHLOSS_BRAINUSE*agents[i].brainActivity;

		agents[i].health -= baseloss;
	}
	
	for (int i=0;i<agents.size();i++) {
		//handle reproduction
		if (agents[i].repcounter<0 && agents[i].health>=conf::MINMOMHEALTH && modcounter%10==0) { 
			//agent is healthy and is ready to reproduce.

			if (agents[i].give>0.5){
				for (int j=0;j<agents.size();j++) {
					float d= (agents[i].pos-agents[j].pos).length();
					float deviation= abs(agents[i].species - agents[j].species); //species deviation check
					if (d<conf::FOOD_SHARING_DISTANCE && i!=j && agents[j].give>0.5 && deviation<=conf::MAXDEVIATION) {
						//this adds conf::BABIES new agents to agents[], but with two parents
						reproduce(i, j, agents[i].MUTRATE1, agents[i].MUTRATE2, agents[j].MUTRATE1, agents[j].MUTRATE2);
						agents[i].children++;
						agents[j].children++;
//						agents[i].health -= 0.2; //reduce health of birthing parent; not as much as assexual reproduction
						agents[i].repcounter= conf::REPRATE;
						agents[j].repcounter= conf::REPRATE;
						break;
						continue;
					}
				}
			}

			if (agents[i].give<=0.5 && randf(0,1)<0.01){
				//this adds conf::BABIES new agents to agents[], with just one parent
				reproduce(i, i, agents[i].MUTRATE1, agents[i].MUTRATE2, agents[i].MUTRATE1, agents[i].MUTRATE2);
				agents[i].children++;
				agents[i].health -= 0.4; //reduce health of birthing parent
				agents[i].repcounter= conf::REPRATE;
				continue;
			}
		}
	}

	for (int i=0;i<agents.size();i++) {
		//remove dead agents. first distribute meat
		if (agents[i].health<=0) { 
		
			cx= (int) agents[i].pos.x/conf::CZ;
			cy= (int) agents[i].pos.y/conf::CZ;

			float meat= cells[1][cx][cy];
			float agemult= 1.0;
			float spikedmult= 0.5;
			float stomachmult= (2-agents[i].stomach[1])/2; //carnivores give 50%
			if(agents[i].age<10) agemult= agents[i].age*0.1; //young killed agents should give very little resources until age 10
			if(agents[i].spiked) spikedmult= 1; //agents which were spiked will give even more meat to the table

			meat+= conf::MEATVALUE*conf::MEATMAX*agemult*spikedmult*stomachmult;
			cells[1][cx][cy]= capCell(meat,conf::MEATMAX);
		}
	}

	vector<Agent>::iterator iter= agents.begin();
	while (iter != agents.end()) {
		if (iter->health <=0) {
			iter= agents.erase(iter);
		} else if(iter->selectflag==1 && deleting==1){
			deleting= 0;
			iter= agents.erase(iter);
		} else {
			++iter;
		}
	}

	//add new agents, if environment isn't closed
	if (!CLOSED) {
		//make sure environment is always populated with at least NUMBOTS bots
		while (agents.size()<conf::NUMBOTS) {
			addRandomBots(1);
		}
		if (modcounter%200==0) {
			if (randf(0,1)<0.5){
				addRandomBots(1); //every now and then add random bots in
			}
		}
	}


}

void World::setInputs()
{
	// R1 G1 B1  R2 G2 B2  R3 G3 B3  R4 G4 B4 HEALTH CLOCK1 CLOCK2 SOUND HEARING SMELL BLOOD TEMP_DISCOMFORT PLAYER_INPUT1
	// 0  1  2   3  4  5   6  7  8   9  10 11   12	  13	  14	15	  16	  17	18	       19			   20

	float PI8=M_PI/8/2; //pi/8/2
	float PI38= 3*PI8; //3pi/8/2
	float PI4= M_PI/4;
   
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		Agent* a= &agents[i];

		//HEALTH
		a->in[12]= cap(a->health/2); //divide by 2 since health is in [0,2]

		//FOOD
		cx= 0;
		cy= 0;
//		a->in[#]= cells[0][cx][cy]/conf::FOODMAX;

//		a->in[#]= cells[1][cx][cy]/conf::MEATMAX;

		//SOUND SMELL EYES
		vector<float> r(NUMEYES,0);
		vector<float> g(NUMEYES,0);
		vector<float> b(NUMEYES,0);
					   
		float smaccum=0;

		vector<float> soaccum(NUMEARS,0);
		vector<float> hearaccum(NUMEARS,0);

		//BLOOD ESTIMATOR
		float blood= 0;

		//cell sense
		int minx, maxx, miny, maxy;
		int scx= (int) (a->pos.x/conf::CZ);
		int scy= (int) (a->pos.y/conf::CZ);

		minx= max((scx-1-conf::DIST/conf::CZ),(float)0);
		maxx= min((scx+2+conf::DIST/conf::CZ),(float)CW);
		miny= max((scy-1-conf::DIST/conf::CZ),(float)0);
		maxy= min((scy+2+conf::DIST/conf::CZ),(float)CH);

		//eyes: cell sight
		for(scx=minx;scx<maxx;scx++){
			for(scy=miny;scy<maxy;scy++){
				if (cells[0][scx][scy]==0 && cells[1][scx][scy]==0 && cells[2][scx][scy]==0) continue;
				Vector2f cellpos= Vector2f((float)(scx*conf::CZ+conf::CZ/2),(float)(scy*conf::CZ+conf::CZ/2)); //find midpoint of the cell
				float d= (a->pos-cellpos).length();

				if (d<conf::DIST) {
					float angle= (cellpos-a->pos).get_angle();

					for(int q=0;q<NUMEYES;q++){
						float aa = a->angle + a->eyedir[q];
						if (aa<-M_PI) aa += 2*M_PI;
						if (aa>M_PI) aa -= 2*M_PI;
						
						float diff1= aa- angle;
						if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1);
						diff1= fabs(diff1);
						
						float fov = a->eyefov[q];
						if (diff1<fov) {
							//we see this cell with this eye. Accumulate stats
							float mul1= a->eye_see_cell_mod*(fabs(fov-diff1)/fov)*((conf::DIST-d)/conf::DIST)*(d/conf::DIST)*2;
							r[q] += mul1*0.25*cells[1][scx][scy]; //meat looks red
							g[q] += mul1*0.25*cells[0][scx][scy]; //plants are green
							r[q] += mul1*0.25*cells[3][scx][scy]; //fruit looks yellow
							g[q] += mul1*0.25*cells[3][scx][scy];
							b[q] += mul1*0.25*cells[2][scx][scy]; //hazards are blue???
							if(a->selectflag && isDebug()){
								linesA.push_back(a->pos);
								linesB.push_back(cellpos);
							}
						}
					}

					float forwangle= a->angle;
					float diff4= forwangle- angle;
					if (fabs(forwangle)>M_PI) diff4= 2*M_PI- fabs(forwangle);
					diff4= fabs(diff4);
					if (diff4<PI38) {
						float mul4= ((PI38-diff4)/PI38)*(1-d/conf::DIST);
						//meat can also be sensed with blood sensor
						blood+= mul4*0.3*cells[1][scx][scy];
					}
				}
			}
		}
					
		for (int j=0;j<agents.size();j++) {
			if (i==j) continue;
			Agent* a2= &agents[j];

			if (a->pos.x<a2->pos.x-conf::DIST || a->pos.x>a2->pos.x+conf::DIST
					|| a->pos.y>a2->pos.y+conf::DIST || a->pos.y<a2->pos.y-conf::DIST) continue;

			float d= (a->pos-a2->pos).length();

			if (d<conf::DIST) {

				//smell
				smaccum+= 1-d/conf::DIST;

				//sound and hearing
				for (int q=0;q<NUMEARS;q++){

					Vector2f v(a->radius, 0);
					v.rotate(a->angle + a->eardir[q]);

					Vector2f earpos= a->pos+ v;

					float eardist= (earpos-a2->pos).length();

					//sound
					soaccum[q]+= a->sound_mod*(1-eardist/conf::DIST)*(max(fabs(a2->w1),fabs(a2->w2)));

					//hearing. Listening to other agents
					hearaccum[q]+= a->hear_mod*(1-eardist/conf::DIST)*a2->soundmul;
				}

				//eyes: bot sight
				float ang= (a2->pos- a->pos).get_angle(); //current angle between bots
				
				for(int q=0;q<NUMEYES;q++){
					float aa = a->angle + a->eyedir[q];
					if (aa<-M_PI) aa += 2*M_PI;
					if (aa>M_PI) aa -= 2*M_PI;
					
					float diff1= aa- ang;
					if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1);
					diff1= fabs(diff1);
					
					float fov = a->eyefov[q];
					if (diff1<fov) {
						//we see a2 with this eye. Accumulate stats
						float mul1= a->eye_see_agent_mod*(fabs(fov-diff1)/fov)*(1-d/conf::DIST)*(1-d/conf::DIST);
//						p[q] += mul1*(d/conf::DIST);
						r[q] += mul1*a2->red;
						g[q] += mul1*a2->gre;
						b[q] += mul1*a2->blu;
						if(a->selectflag && isDebug()){
							linesA.push_back(a->pos);
							linesB.push_back(a2->pos);
						}
					}
				}
				
				//blood sensor
				float forwangle= a->angle;
				float diff4= forwangle- ang;
				if (fabs(forwangle)>M_PI) diff4= 2*M_PI- fabs(forwangle);
				diff4= fabs(diff4);
				if (diff4<PI38) {
					float mul4= ((PI38-diff4)/PI38)*(1-d/conf::DIST);
					//if we can see an agent close with both eyes in front of us
					blood+= mul4*(1-agents[j].health/2); //remember: health is in [0 2]
					//agents with high life dont bleed. low life makes them bleed more
				}
			}
		}

		//temperature varies from 0 to 1 across screen.
		//it is 0 at equator (in middle), and 1 on edges. Agents can sense this range
		float temp= 2.0*abs(a->pos.y/conf::HEIGHT - 0.5);
		
		smaccum *= a->smell_mod;
		blood *= a->blood_mod;

//		for(int e;e<NUMEYES;e++){

		a->in[0]= cap(r[0]);
		a->in[1]= cap(g[0]);
		a->in[2]= cap(b[0]);
		
		a->in[3]= cap(r[1]);
		a->in[4]= cap(g[1]);
		a->in[5]= cap(b[1]);

		a->in[6]= cap(r[2]);
		a->in[7]= cap(g[2]);
		a->in[8]= cap(b[2]);

		a->in[9]= cap(r[3]);
		a->in[10]= cap(g[3]);
		a->in[11]= cap(b[3]);

		a->in[13]= abs(sin((modcounter+current_epoch/conf::FRAMES_PER_EPOCH)/a->clockf1));
		a->in[14]= abs(sin((modcounter+current_epoch/conf::FRAMES_PER_EPOCH)/a->clockf2));
		a->in[15]= cap(soaccum[0]);
		a->in[16]= cap(soaccum[1]);
		a->in[17]= cap(hearaccum[0]);
		a->in[18]= cap(hearaccum[1]);
		a->in[19]= cap(smaccum);
		a->in[20]= cap(blood);
		a->in[21]= temp;
		
		if (a->selectflag) {
			a->in[22]= pinput1;
		} else {
			a->in[22]= 0;
		}
	}
}

void World::processOutputs()
{
	//assign meaning
	//LEFT RIGHT R G B SPIKE BOOST SOUND_MULTIPLIER GIVING CHOICE STIMULANT JUMP
	// 0	1	 2 3 4   5	   6		 7			   8	  9	     10		 11
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		Agent* a= &agents[i];

		if (a->jump<=0) { //if not jumping, then change wheel speeds. otherwise, we want to keep wheel speeds constant
			if (pcontrol && a->selectflag) {
				a->w1= pright;
				a->w2= pleft;
			} else {
				a->w1= a->out[0]; //-(2*a->out[0]-1);
				a->w2= a->out[1]; //-(2*a->out[1]-1);
			}
		}
		a->red+= 0.2*(a->out[2]-a->red);
		a->gre+= 0.2*(a->out[3]-a->gre);
		a->blu+= 0.2*(a->out[4]-a->blu);
		if (a->jump<=0) a->boost= a->out[6]>0.5; //if jump height is zero, boost can change
		a->soundmul= a->out[7];
		a->give= a->out[8];

		//spike length should slowly tend towards out[5]
		float g= a->out[5];
		if (a->spikeLength<g) {
			a->spikeLength+=conf::SPIKESPEED;
			a->health-= conf::HEALTHLOSS_SPIKE_EXT;
		}
		else if (a->spikeLength>g) a->spikeLength= g; //its easy to retract spike, just hard to put it up

		//jump gets set to 2*(out[11] - 0.5) if itself is zero (the bot is on the ground) and if out[11] is greater than 0.5
		float height= (a->out[11] - 0.5);
		if (a->jump==0 && height>0) a->jump= height*2;
	}

	//move bots
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		Agent* a= &agents[i];

		//IDK where else to put this, but this looks like as good a place as any
		a->jump-= conf::GRAVITYACCEL;
		if(a->jump<-1) a->jump= 0; //-1 because we will be nice and give a "recharge" time between jumps

		Vector2f v(a->radius/2, 0);
		v.rotate(a->angle + M_PI/2);

		Vector2f w1p= a->pos+ v; //wheel positions
		Vector2f w2p= a->pos- v;

		float BW1= conf::BOTSPEED*a->w1;
		float BW2= conf::BOTSPEED*a->w2;
		if (a->boost) { //if boosting
			BW1=BW1*conf::BOOSTSIZEMULT;
			BW2=BW2*conf::BOOSTSIZEMULT;
		}

		//move bots
		Vector2f vv= w2p- a->pos;
		vv.rotate(-BW1);
		a->pos= w2p-vv;
		if (a->jump<=0) {
			a->angle -= BW1;
		}
		if (a->angle<-M_PI) a->angle= M_PI - (-M_PI-a->angle);
		vv= a->pos - w1p;
		vv.rotate(BW2);
		a->pos= w1p+vv;
		if (a->jump<=0) {
			a->angle += BW2;
		}
		if (a->angle>M_PI) a->angle= -M_PI + (a->angle-M_PI);

		//wrap around the map
		if (a->pos.x<0) a->pos.x+= conf::WIDTH;
		if (a->pos.x>=conf::WIDTH) a->pos.x-= conf::WIDTH;
		if (a->pos.y<0) a->pos.y+= conf::HEIGHT;
		if (a->pos.y>=conf::HEIGHT) a->pos.y-= conf::HEIGHT;
	}

	//process interaction with cells
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		Agent* a= &agents[i];

		int scx= (int) a->pos.x/conf::CZ;
		int scy= (int) a->pos.y/conf::CZ;

		if (a->jump<=0){ //no interaction with these cells if jumping
			//plant food: layer 0
			float food= cells[0][scx][scy];
			float plantintake= 0;
			if (food>0 && a->health<2) {
				//agent eats the food
				float speedmul= 1-max(abs(a->w1), abs(a->w2));
				plantintake= min(food,conf::FOODINTAKE)*a->stomach[0]*(1-a->stomach[1])*(1-a->stomach[2])*speedmul;
				if (a->health>=conf::MINMOMHEALTH) a->repcounter -= a->metabolism*plantintake;
				a->health+= plantintake;
				cells[0][scx][scy]-= min(food,conf::FOODWASTE*plantintake/conf::FOODINTAKE);
			}

			//meat food: layer 1
			float meat= cells[1][scx][scy];
			float meatintake= 0;
			if (meat>0 && a->health<2) {
				//agent eats meat
				float speedmul= 1-max(abs(a->w1), abs(a->w2));
				meatintake= min(meat,conf::MEATINTAKE)*a->stomach[1]*(1-a->stomach[0])*(1-a->stomach[2])*speedmul;
				if (a->health>=conf::MINMOMHEALTH) a->repcounter -= a->metabolism*meatintake;
				a->health += meatintake;
				cells[1][scx][scy]-= min(meat,conf::MEATWASTE*meatintake/conf::MEATINTAKE);
			}

			//hazards: layer 2
			float hazard= cells[2][scx][scy];
			if (hazard>0){
				a->health -= conf::HAZARDDAMAGE*hazard;
			}
			//agents fill up hazard cells only up to 9/10, because any greater can be reset to zero
			if (modcounter%5==0){
				if((hazard + conf::HAZARDDEPOSIT)<=conf::HAZARDMAX*9/10) hazard+= min(conf::HAZARDDEPOSIT,conf::HAZARDMAX*9/10 - hazard);
				cells[2][scx][scy]= capCell(hazard,conf::HAZARDMAX);
			}

			//Fruit food: layer 3
			float fruit= cells[3][scx][scy];
			float fruitintake= 0;
			if (fruit>0 && a->health<2) {
				//agent eats meat
				float speedmul= 1-max(abs(a->w1), abs(a->w2));
				fruitintake= min(fruit,conf::FRUITINTAKE)*a->stomach[2]*(1-a->stomach[0])*(1-a->stomach[1])*speedmul;
				if (a->health>=conf::MINMOMHEALTH) a->repcounter -= a->metabolism*fruitintake;
				a->health += fruitintake;
				cells[3][scx][scy]-= min(fruit,conf::FRUITWASTE*fruitintake/conf::FRUITINTAKE);
			}
		}

		//land/water: layer 4
		float air= cells[4][scx][scy];
		if (pow(air-a->lungs,2)>=0.01) a->health -= conf::HEALTHLOSS_BADAIR*pow(air-a->lungs,2);

		if (a->health>2){ //if health has been increased over the cap, convert the overflow into repcounter
			a->repcounter -= a->metabolism*(a->health-2);
			a->health= 2;
		}
	}

	//process giving and receiving of food
	#pragma omp parallel for //CONSIDER: making this modcounter-based w/ timed reset
	for (int i=0;i<agents.size();i++) {
		if (agents[i].give>0.5) {
			for (int j=0;j<agents.size();j++) {
				float d= (agents[i].pos-agents[j].pos).length();
				if (d<conf::FOOD_SHARING_DISTANCE && agents[j].health<agents[i].health) {
					//initiate transfer
					agents[j].health += conf::FOODTRANSFER;
					agents[i].health -= conf::FOODTRANSFER;
					agents[j].dfood += conf::FOODTRANSFER; //only for drawing
					agents[i].dfood -= conf::FOODTRANSFER;
				}
			}
		}
	}

	//process collision dynamics
	if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents
//		#pragma omp parallel for
		for (int i=0;i<agents.size();i++) {
			Agent* a= &agents[i];

			for (int j=0;j<agents.size();j++) {
				if (i==j) continue;

				Agent* a2= &agents[j];

				float d= (a->pos-a2->pos).length();
				float sumrad= a->radius+a2->radius;
				if (d<sumrad && a->jump<=0 && a2->jump<=0) {
					//if inside each others radii and neither are jumping, fix physics
					float ov= (sumrad-d);
					if (ov>0 && d>0.0001) {
						if (ov>conf::TOOCLOSE) {//if bots are too close, they get injured before being pushed away
							float DMG= ov*conf::HEALTHLOSS_BUMP;
							a->health-= DMG*sumrad/2/a->radius; //larger bots take less damage, bounce less
							a2->health-= DMG*sumrad/2/a2->radius;
							if (a->ir==0){ a->initEvent(30,0,0.5,1); a2->initEvent(30,0,0.5,1); }
						}
						float ff1= ov/d*a2->radius/a->radius*conf::REACTPOWER; //the radii come in here for inertia-like effect
						float ff2= ov/d*a->radius/a2->radius*conf::REACTPOWER;
						a->pos.x-= (a2->pos.x-a->pos.x)*ff1;
						a->pos.y-= (a2->pos.y-a->pos.y)*ff1;
						a2->pos.x+= (a2->pos.x-a->pos.x)*ff2;
						a2->pos.y+= (a2->pos.y-a->pos.y)*ff2;

						if (a->pos.x>conf::WIDTH) a->pos.x-= conf::WIDTH;
						if (a->pos.y>conf::HEIGHT) a->pos.y-= conf::HEIGHT;
						if (a2->pos.x>conf::WIDTH) a2->pos.x-= conf::WIDTH;
						if (a2->pos.y>conf::HEIGHT) a2->pos.y-= conf::HEIGHT;
						if (a->pos.x<0) a->pos.x+= conf::WIDTH;
						if (a->pos.y<0) a->pos.y+= conf::HEIGHT;
						if (a2->pos.x<0) a2->pos.x+= conf::WIDTH;
						if (a2->pos.y<0) a2->pos.y+= conf::HEIGHT;

//						printf("%f, %f, %f, %f, and %f\n", a->pos.x, a->pos.y, a2->pos.x, a2->pos.y, ov);
					}
				}

				//low speed doesn't count, nor does a small spike (duh). If the target is jumping in midair, can't attack either
				if(a->spikeLength*conf::SPIKELENGTH<a->radius || a->w1<0.2 || a->w2<0.2 || a2->jump>0) continue;
				else if(d<=sumrad + conf::SPIKELENGTH*a->spikeLength) {

					//these two are in collision and agent i has extended spike and is going decent fast!
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/8) {
						//bot i is also properly aligned!!! that's a hit
						float DMG= conf::SPIKEMULT*a->spikeLength*max(fabs(a->w1),fabs(a->w2));

						a2->health-= DMG;

						if (a->health>2) a->health=2; //cap health at 2
						a->spikeLength= 0; //retract spike back down

						a->initEvent(20*DMG,1,1,0); //yellow event means bot has spiked the other bot. nice!
						if (a2->health<=0) a->initEvent(20*DMG,1,0,0); //red event means bot has killed the other bot. nice!

						Vector2f v2(1,0);
						v2.rotate(a2->angle);
						float adiff= v.angle_between(v2);
						if (fabs(adiff)<M_PI/2) {
							//this was attack from the back. Retract spike of the other agent (startle!)
							//this is done so that the other agent cant right away "by accident" attack this agent
							a2->spikeLength= 0;
						}
						
						a2->spiked= true; //set a flag saying that this agent was hit this turn
					}
				}
			}
		}
	}
}

void World::brainsTick()
{
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		agents[i].tick();
	}
}

void World::positionOfInterest(int type, float &xi, float &yi) {
	int maxi= -1;
	if (type==1) {
		//the interest of type 1 is the oldest agent
		int maxage= -1;
		for (int i=0;i<agents.size();i++) {
		   if (agents[i].age>maxage) { 
			   maxage= agents[i].age; 
			   maxi= i;
		   }
		}
	} else if(type==2){
		//interest of type 2 is the selected agent
		for (int i=0;i<agents.size();i++) {
			if (agents[i].selectflag==1) {maxi= i; break; }
		}
	} else if(type==3){
		//interest of type 3 is most advanced generation
		int maxgen= 0;
		for (int i=0;i<agents.size();i++) {
		   if (agents[i].gencount>maxgen) { 
			   maxgen= agents[i].gencount; 
			   maxi= i;
		   }
		}
	} else if(type==4){
		//interest of type 4 is healthiest
		float maxhealth= -1;
		for (int i=0;i<agents.size();i++) {
			if (agents[i].health>maxhealth*0.9) { //0.9 multiplier is there to reduce quick-jumps when there is competition
				maxhealth= agents[i].health;
				maxi= i;
			}
		}
	} else if(type==5){
		//interest of type 5 is most productive
		float maxprog= -1;
		for (int i=0;i<agents.size();i++) {
			if (agents[i].age!=0){
				if ((float)(agents[i].children/agents[i].age)>maxprog){
					maxprog= agents[i].children/agents[i].age;
					maxi= i;
				}
			}
		}
	}
	/* else if(type==){
		//interest of type  is relatives
		float mingen= 100000;
		for (int i=0;i<agents.size();i++) {
			if (agents[i].selectflag && agents[i].health<=0) {
				for (int j=0;j<agents.size();j++) {
					float gendif= abs(agents[i].gencount-agents[j].gencount+1);
					if (i!=j && gendif<mingen) {
						mingen= gendif;
						maxi= i;
					}
				}
			}
		}
	}*/
	if (maxi!=-1) {
		xi= agents[maxi].pos.x;
		yi= agents[maxi].pos.y;
	}
}

void World::addCarnivore()
{
	Agent a;
	a.id= idcounter;
	idcounter++;
	a.stomach[0]= randf(0, 0.2);
	a.stomach[1]= randf(0.8, 1);
	a.stomach[2]= randf(0, 0.2);
	agents.push_back(a);
}

void World::addHerbivore()
{
	Agent a;
	a.id= idcounter;
	idcounter++;
	a.stomach[0]= randf(0.8, 1);
	a.stomach[1]= randf(0, 0.2);
	a.stomach[2]= randf(0, 0.2);
	agents.push_back(a);
}

void World::addRandomBots(int num, int type)
{
	for (int i=0;i<num;i++) {
		if (type==1) addHerbivore(); //herbivore
		else if (type==2) addCarnivore(); //carnivore
		else {
			Agent a;
			a.id= idcounter;
			idcounter++;
			agents.push_back(a);
		}
	}
}

void World::reproduce(int ai, int bi, float aMR, float aMR2, float bMR, float bMR2)
{
	float MR= min(aMR,bMR);
	float MR2= min(aMR2,bMR2); //Using min because max, though correct, will lead to rampant mutation (is that perhaps wanted?) will test
	if (randf(0,1)<0.04) MR= MR*randf(1, 10);
	if (randf(0,1)<0.04) MR2= MR2*randf(1, 10);

	Agent that= agents[bi];
	Agent a= agents[ai];
	if (ai==bi){
		that= agents[ai];
		agents[ai].initEvent(30,0,0.8,0); //green event means agent asexually reproduced
	}else{
		agents[ai].initEvent(30,0,0,0.8);
		agents[bi].initEvent(30,0,0,0.8); //blue events mean agents sexually reproduced.
	}

	for (int i=0;i<conf::BABIES;i++) {
		Agent a2 = a.reproduce(that,MR,MR2);
		if (ai!=bi){
			a2.hybrid= true; //if parents are not the same agent (sexual reproduction), then mark the child
		}
		a2.id= idcounter;
		idcounter++;
		agents.push_back(a2);
	}
}

void World::writeReport()
{
	printf("Writing Report, Epoch: %i\n", current_epoch);
	//save all kinds of nice data stuff
	int topcarn=0;
	int topfrug=0;
	int topherb=0;
	int happyspecies=0;
	//vector<int> species;
	//vector<int> members;
	//species.resize(1,0);
	//members.resize(1,0);

	for(int i=0;i<agents.size();i++){ 
		if((agents[i].stomach[0]>=agents[i].stomach[1] && agents[i].stomach[0]>=agents[i].stomach[2]) 
			&& agents[i].gencount>topherb) topherb= agents[i].gencount;
		if((agents[i].stomach[1]>=agents[i].stomach[0] && agents[i].stomach[1]>=agents[i].stomach[2])
			&& agents[i].gencount>topfrug) topfrug= agents[i].gencount;
		if((agents[i].stomach[2]>=agents[i].stomach[0] && agents[i].stomach[2]>=agents[i].stomach[1])
			&& agents[i].gencount>topcarn) topcarn= agents[i].gencount;

		/*for(int s=0;s<species.size();s++){ //for every logged species...

			int speciesdiff= abs(species[s]-agents[i].species);

			if(speciesdiff>conf::MAXDEVIATION){ //is our bot not near any of them?...
				if(s==species.size()-1){ //if so, let's make sure we've checked all logged species...
					species.push_back(agents[i].species); //and add this one if it's unique enough.
					members.push_back(1);
					break;
				}

			} else { //If our bot is actually a member of an already logged species, then we increment the member array...
				members[s]++;
				//and make the species num an average to better find other members
//				species[s]= (int) (species[s] + agents[i].species) / members[s];
				//we will then make sure that only species with at least 3 members are counted
				break;
			}
		}*/
	}

/*	for(int s=0;s<species.size();s++){
		if(members[s]>=3) happyspecies++; //3 agents with the same species id counts as a species
	}*/

	FILE* fr = fopen("report.txt", "a");
	fprintf(fr, "Epoch: %i #Agents: %i #Herbi: %i #Frugi: %i #Carni: %i #0.75Plant: %i #0.5Meat: %i #0.1Hazard: %i TopH: %i TopF: %i TopC: %i #Hybrids: %i #Spikes: %i\n",
		current_epoch, getAgents(), getHerbivores(), getFrugivores(), getCarnivores(), getFood(), getMeat(), getHazards(), topherb, topfrug, topcarn, getHybrids(), getSpiked());
	fclose(fr);
}


void World::reset()
{
	current_epoch= 0;
	modcounter= 0;
	idcounter= 0;
	CW= conf::WIDTH/conf::CZ;
	CH= conf::HEIGHT/conf::CZ; //note: can add custom variables from loaded savegames here possibly

	agents.clear();

	//handle layers
	for(cx=0;cx<CW;cx++){
		for(cy=0;cy<CH;cy++){
			for(int l=0;l<LAYERS;l++){
				cells[l][cx][cy]= 0;
			}
//			cells[>>>TEMPERATURE_LAYER<<<][cx][cy]= 2.0*abs((float)cy/CH - 0.5); [old temperature indicating code]
		}
	}

	cellsLandMasses(4);

	//open report file; null it up if it exists
	FILE* fr = fopen("report.txt", "w");
	fclose(fr);

	ptr=0;
}

void World::setClosed(bool close)
{
	CLOSED = close;
}

bool World::isClosed() const
{
	return CLOSED;
}


void World::processMouse(int button, int state, int x, int y)
{
	 if (state==0) {		
		 float mind=1e10;
		 float mini=-1;
		 float d;

		 for (int i=0;i<agents.size();i++) {
			 d= pow(x-agents[i].pos.x,2)+pow(y-agents[i].pos.y,2);
				 if (d<mind) {
					 mind=d;
					 mini=i;
				 }
			 }
		 if (mind<1000) {
			 //toggle selection of this agent
			 for (int i=0;i<agents.size();i++) {
				if(i!=mini) agents[i].selectflag=false;
			 }
			 agents[mini].selectflag= !agents[mini].selectflag;
			 agents[mini].initEvent(10,1.0,1.0,1.0);
			 agents[mini].printSelf();
			 setControl(false);
		 }
	 }
}
	 
void World::draw(View* view, int layer)
{
	//draw cell layer
	if(layer!=0) {
		for(int x=0;x<CW;x++) {
			for(int y=0;y<CH;y++) {
				float val;
				if (layer==1) { 
					//plant food
					val= 0.5*cells[0][x][y]/conf::FOODMAX;
				} else if (layer==2) { 
					//meat food
					val= 0.5*cells[1][x][y]/conf::MEATMAX;
				} else if (layer==3) {
					//hazards
					val = 0.5*cells[2][x][y]/conf::HAZARDMAX;
				} else if (layer==4) {
					//fruit food
					val = 0.5*cells[3][x][y]/conf::FRUITMAX;
				} else if (layer==5) {
					//land
					val = cells[4][x][y];
				}
/*				} else if (layer==TEMPERATURE_LAYER) { 
					//temperature
					val = cells[3][x][y];
				}*/

				view->drawCell(x,y,val);
			}
		}
	}
	
	//draw all agents
	vector<Agent>::const_iterator it;
	for ( it = agents.begin(); it != agents.end(); ++it) {
		view->drawAgent(*it);
	}
	
	view->drawData();
}

int World::getHerbivores() const
{
	int count= 0;
	for (int i=0;i<agents.size();i++) {
		if (agents[i].stomach[0]>=agents[i].stomach[1] && agents[i].stomach[0]>=agents[i].stomach[2]) count++;
	}
	return count;
}

int World::getCarnivores() const
{
	int count= 0;
	for (int i=0;i<agents.size();i++) {
		if (agents[i].stomach[1]>=agents[i].stomach[0] && agents[i].stomach[1]>=agents[i].stomach[2]) count++;
	}
	return count;
}

int World::getFrugivores() const
{
	int count= 0;
	for (int i=0;i<agents.size();i++) {
		if (agents[i].stomach[2]>=agents[i].stomach[0] && agents[i].stomach[2]>=agents[i].stomach[1]) count++;
	}
	return count;
}

int World::getAgents() const
{
	return agents.size();
}

int World::getFood() const //count plant cells with 75% max or more
{
	int numfood=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float food= 0.5*cells[0][i][j]/conf::FOODMAX;
			if(food>conf::FOODMAX*3/4){
				numfood++;
			}
		}
	}
	return numfood;
}

int World::getMeat() const //count meat cells with 50% max or more
{
	int nummeat=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float meat= 0.5*cells[1][i][j]/conf::MEATMAX;
			if(meat>conf::MEATMAX/2){
				nummeat++;
			}
		}
	}
	return nummeat;
}

int World::getHazards() const //count hazard cells with 10% max or more
{
	int numhazards=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float hazard= 0.5*cells[2][i][j]/conf::HAZARDMAX;
			if(hazard>conf::HAZARDMAX*0.1){
				numhazards++;
			}
		}
	}
	return numhazards;
}

int World::getHybrids() const
{
	int numhybrid= 0;
	for (int i=0;i<agents.size();i++) {
		if (agents[i].hybrid) numhybrid++;
	}
	
	return numhybrid;
}

int World::getSpiked() const
{
	//gets number of agents with spikes, not spiked agents
	int count= 0;
	for (int i=0;i<agents.size();i++) {
		if (agents[i].spikeLength*conf::SPIKELENGTH>=agents[i].radius) count++;
	}
	return count;
}

int World::epoch() const
{
	return current_epoch;
}

void World::setControl(bool state)
{
	//reset left and right wheel controls 
	pleft= 0;
	pright= 0;

	pcontrol= state;
}

void World::cellsRandomFill(int layer, float amount, int number)
{
	for (int i=0;i<number;i++) {
		cx=randi(0,CW);
		cy=randi(0,CH);
		cells[layer][cx][cy]= amount;
	}
}

void World::cellsLandMasses(int layer)
{
	//creates land masses for the layer given
	int leftcount= CW*CH;

	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			cells[layer][i][j]= -1; //"null" all cells
		}
	}

	for (int i=0;i<conf::CONTINENTS;i++) {
		//spawn init continents (land= 1)
		cx=randi(0,CW);
		cy=randi(0,CH);
		cells[layer][cx][cy]= 1;
	}
	for (int i=0;i<sqrt(CW*CH*conf::OCEANPERCENT)/10;i++) {
		//spawn oceans (water= 0)
		cx=randi(0,CW);
		cy=randi(0,CH);
		cells[layer][cx][cy]= 0;
	}

	while(leftcount!=0){
		#pragma omp parallel for
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				//land spread
				if (cells[layer][i][j]==1){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2);
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[layer][ox][oy]==-1 && randf(0,1)<0.9) cells[layer][ox][oy]= 1;
				}

				//water spread
				if (cells[layer][i][j]==0){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2);
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[layer][ox][oy]==-1 && randf(0,1)<0.9) cells[layer][ox][oy]= 0;
				}
			}
		}
		
		leftcount= 0;
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				if (cells[layer][i][j]==-1){
					leftcount++;
				}
			}
		}
	}
}

float World::capCell(float a, float top) const
{
	return min(top,max(a,(float)0));
}

void World::setDebug(bool state)
{
	DEBUG = state;
}

bool World::isDebug() const
{
	return DEBUG;
}
