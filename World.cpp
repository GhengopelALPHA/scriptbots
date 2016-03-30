#include "World.h"

#include <ctime>

#include "settings.h"
#include "helpers.h"
#include "vmath.h"
#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <iostream>

using namespace std;

World::World() :
		CLOSED(false),
		DEBUG(false),
		SELECTION(-1),
		pcontrol(false),
		pright(0),
		pleft(0),
		pinput1(0)
{
	//inititalize
	reset();

	spawn();
}

void World::update()
{
	modcounter++;
	vector<int> dt;

	//Process periodic events
	//Age goes up!
	if (modcounter%100==0) {
		#pragma omp parallel for
		for (int i=0; i<(int)agents.size(); i++) {
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

		deaths.clear();
	}

	if (modcounter>=conf::FRAMES_PER_EPOCH) {
		modcounter=0;
		current_epoch++;
		//if autosave enabled
//		ReadWrite* savehelper= new ReadWrite(); //for loading/saving
//		savehelper->saveWorld(this, 0, 0, "AUTOSAVE.SCB");

	}
	if ((modcounter%conf::FOODADDFREQ==0 && !CLOSED) || getFood()<conf::MINFOOD) {
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::PLANTS][cx][cy]= conf::FOODMAX;
	}
	if (modcounter%conf::HAZARDFREQ==0) {
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::HAZARDS][cx][cy]= conf::HAZARDMAX;
	}
	if (modcounter%conf::FRUITADDFREQ==0) {
		bool trigger= false;
		while (!trigger) {
			int cx=randi(0,CW);
			int cy=randi(0,CH);
			if (cells[Layer::PLANTS][cx][cy]>= conf::FOODMAX*conf::FRUITREQUIRE) {
				cells[Layer::FRUITS][cx][cy]= conf::FRUITMAX;
				trigger= true;
			}
		}
	}

	#pragma omp parallel for schedule(dynamic)
	for(int cx=0; cx<(int)CW;cx++){
		for(int cy=0; cy<(int)CH;cy++){
			//food = cells[Layer::PLANTS]...
			if (cells[Layer::PLANTS][cx][cy]>0 && cells[Layer::PLANTS][cx][cy]<conf::FOODMAX) {
				cells[Layer::PLANTS][cx][cy]-= conf::FOODDECAY; //food quantity is changed by FOODDECAY
				if (cells[Layer::HAZARDS][cx][cy]>0) {
					cells[Layer::PLANTS][cx][cy]+= conf::FOODGROWTH*cells[Layer::HAZARDS][cx][cy]/conf::HAZARDMAX; //food grows out of waste/hazard
				}
			}
			cells[Layer::PLANTS][cx][cy]= capm(cells[Layer::PLANTS][cx][cy], 0, conf::FOODMAX); //cap food at FOODMAX

			//meat = cells[Layer::MEATS]...
			if (cells[Layer::MEATS][cx][cy]>0 && cells[Layer::MEATS][cx][cy]<=conf::MEATMAX) {
				cells[Layer::MEATS][cx][cy] -= conf::MEATDECAY/cells[Layer::MEATS][cx][cy]; //meat decays exponentially
			}
			cells[Layer::MEATS][cx][cy]= capm(cells[Layer::MEATS][cx][cy], 0, conf::MEATMAX); //cap at MEATMAX

			//hazard = cells[Layer::HAZARDS]...
			if (cells[Layer::HAZARDS][cx][cy]>0){
				cells[Layer::HAZARDS][cx][cy]-= conf::HAZARDDECAY; //hazard decays
			}
			if (cells[Layer::HAZARDS][cx][cy]>conf::HAZARDMAX*9/10 && randf(0,1)<0.125){
				cells[Layer::HAZARDS][cx][cy]= 0; //instant hazards will be reset to zero
			}
			cells[Layer::HAZARDS][cx][cy]= capm(cells[Layer::HAZARDS][cx][cy], 0, conf::HAZARDMAX); //cap at HAZARDMAX

			//fruit = cells[Layer::FRUITS]...
			if (cells[Layer::PLANTS][cx][cy]<=conf::FOODMAX*conf::FRUITREQUIRE && cells[Layer::FRUITS][cx][cy]>0){
				cells[Layer::FRUITS][cx][cy]-= conf::FRUITDECAY; //fruit decays from lack of plant life
			}
			if (randf(0,1)<conf::FOODSPREAD && cells[Layer::FRUITS][cx][cy]>0.5*conf::FRUITMAX) {
				//food seeding
				int ox= randi(cx-1-conf::FOODRANGE,cx+2+conf::FOODRANGE);
				int oy= randi(cy-1-conf::FOODRANGE,cy+2+conf::FOODRANGE);
				if (ox<0) ox+= CW;
				if (ox>CW-1) ox-= CW;
				if (oy<0) oy+= CH;
				if (oy>CH-1) oy-= CH; //code up to this point ensures world edges are crossed and not skipped
				if (cells[Layer::PLANTS][ox][oy]<=conf::FOODMAX*3/4) cells[Layer::PLANTS][ox][oy]+= conf::FOODMAX/4;
			}
			cells[Layer::FRUITS][cx][cy]= capm(cells[Layer::FRUITS][cx][cy], 0, conf::FRUITMAX); //cap
		}
	}
	
	//reset any counter variables per agent and do debug stuff
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		//reduce fresh kill flag
		if(agents[i].freshkill>0) agents[i].freshkill-= 1;

		//process indicator (used in drawing)
		if(agents[i].indicator>0) agents[i].indicator-= 1;

		//reset dfood for processOutputs
		agents[i].dfood= 0;
	}

	//give input to every agent. Sets in[] array
	setInputs();

	//brains tick. computes in[] -> out[]
	brainsTick();

	//read output and process consequences of bots on environment. requires out[]
	processOutputs();

	//process bots:
	for (int i=0; i<(int)agents.size(); i++) {
		//find brain activity on this tick
		agents[i].setActivity();

		//doLiveMutate
		float MR= agents[i].MUTRATE1;
		float MR2= agents[i].MUTRATE2;
		agents[i].brain.liveMutate(MR, MR2, agents[i].out);
	}

	//process health:
	healthTick();
	
	//handle reproduction
	//do not omp any of this!
	if (modcounter%5==0){
		for (int i=0; i<(int)agents.size(); i++) {
			if (agents[i].repcounter<0 && agents[i].health>=conf::MINMOMHEALTH) { 
				//agent is healthy and is ready to reproduce.

				if(randf(0,1)<0.75){
					for (int j=0; j<(int)agents.size(); j++) {
						if(i==j || !(agents[j].sexproject)) continue;
						float d= (agents[i].pos-agents[j].pos).length();
						float deviation= abs(agents[i].species - agents[j].species); //species deviation check
						if (d<=conf::SEXTING_DISTANCE && deviation<=conf::MAXDEVIATION) {
							//this adds agent[i].numbabies to world, but with two parents
							reproduce(i, j);
							agents[i].repcounter= conf::REPRATE;
							agents[j].repcounter= conf::REPRATE;
							break;
							continue;
						}
					}
				} else {
					//this adds agents[i].numbabies to world, but with just one parent
					reproduce(i, i);
					agents[i].repcounter= conf::REPRATE;
					continue;
				}
			}
		}
	}

	for (int i=0; i<(int)agents.size(); i++) {
		//remove dead agents. first distribute meat
		if (agents[i].health<=0) { 
			if (agents[i].health<0) agents[i].writeIfKilled("Killed by Something ?");
		
			int cx= (int) capm(agents[i].pos.x/conf::CZ, 0, conf::WIDTH/conf::CZ);
			int cy= (int) capm(agents[i].pos.y/conf::CZ, 0, conf::HEIGHT/conf::CZ);

			float meat= cells[Layer::MEATS][cx][cy];
			float agemult= 1.0;
			float freshmult= 0.25; //if this agent wasnt freshly killed, default to 25%
			float stomachmult= (2-agents[i].stomach[Stomach::MEAT])/2; //carnivores give 50%
			if(agents[i].age<10) agemult= agents[i].age*0.1; //young killed agents should give very little resources until age 10
			if(agents[i].freshkill>0) freshmult= 1; //agents which were spiked recently will give full meat

			meat+= conf::MEATVALUE*conf::MEATMAX*agemult*freshmult*stomachmult;
			cells[Layer::MEATS][cx][cy]= capm(meat, 0, conf::MEATMAX);

			//collect all the death causes from all dead agents
			deaths.push_back(agents[i].death);
		}
	}

	vector<Agent>::iterator iter= agents.begin();
	while (iter != agents.end()) {
		if (iter->health <=0) {
			if (SELECTION==iter->id) printf("The Selected Agent was %s!\n", iter->death);
			iter= agents.erase(iter);
		} else {
			++iter;
		}
	}

	//add new agents, if environment isn't closed
	if (!CLOSED) {
		//make sure environment is always populated with at least NUMBOTS bots
		if (agents.size()<conf::NUMBOTS) {
			addAgents(conf::NUMBOTS-agents.size());
		}
		if (agents.size()<conf::ENOUGHBOTS && modcounter%50==0) {
			if (randf(0,1)<0.5){
				addAgents(1); //every now and then add random bot in if population too low
			}
		}
	}


}

void World::setInputs()
{
	// R1 G1 B1  R2 G2 B2  R3 G3 B3  R4 G4 B4 HEALTH CLOCK1 CLOCK2 SOUND HEARING SMELL BLOOD TEMP_DISCOMFORT PLAYER_INPUT1 
	// 0  1  2   3  4  5   6  7  8   9  10 11   12	  13	  14   15,16  17,18	  19	20	       21			   22
	// FOOD_SMELL MEAT_SMELL HAZARD_SMELL WATER_SMELL
	//     23         24          25           26

	float PI8=M_PI/8/2; //pi/8/2
	float PI38= 3*PI8; //3pi/8/2
	float PI4= M_PI/4;
   
	#pragma omp parallel for// schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];

		//HEALTH
		a->in[Input::HEALTH]= cap(a->health/2); //divide by 2 since health is in [0,2]

		//FOOD
//		a->in[#]= cells[Layer::PLANTS][cx][cy]/conf::FOODMAX;

//		a->in[#]= cells[Layer::MEATS][cx][cy]/conf::MEATMAX;

		//SOUND SMELL EYES
		vector<float> r(NUMEYES,0);
		vector<float> g(NUMEYES,0);
		vector<float> b(NUMEYES,0);
					   
		float smellsum=0;

		vector<float> soundsum(NUMEARS,0);
		vector<float> hearsum(NUMEARS,0);

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

		float fruit= 0, meat= 0, hazard= 0, water= 0;

		//cell "smelling": a faster/better food- & hazard-revealing solution than adding to eyesight
		for(scx=minx;scx<maxx;scx++){
			for(scy=miny;scy<maxy;scy++){
				fruit+= cells[Layer::FRUITS][scx][scy]/conf::FRUITMAX;
				meat+= cells[Layer::MEATS][scx][scy]/conf::MEATMAX;
				hazard+= cells[Layer::HAZARDS][scx][scy]/conf::HAZARDMAX;
				water+= (1-cells[Layer::LAND][scx][scy]);
			}
		}
		fruit*= a->smell_mod/(maxx-minx)/(maxy-miny);
		meat*= a->smell_mod/(maxx-minx)/(maxy-miny);
		hazard*= a->smell_mod/(maxx-minx)/(maxy-miny);
		water*= a->smell_mod/(maxx-minx)/(maxy-miny);


				/* EYESIGHT CODE
				if (cells[Layer::PLANTS][scx][scy]==0 && cells[Layer::MEATS][scx][scy]==0 && cells[Layer::HAZARDS][scx][scy]==0) continue;
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
							r[q] += mul1*0.25*cells[Layer::MEATS][scx][scy]; //meat looks red
							g[q] += mul1*0.25*cells[Layer::PLANTS][scx][scy]; //plants are green
							r[q] += mul1*0.25*cells[Layer::FRUITS][scx][scy]; //fruit looks yellow
							g[q] += mul1*0.25*cells[Layer::FRUITS][scx][scy];
							b[q] += mul1*0.25*cells[Layer::HAZARDS][scx][scy]; //hazards are blue???
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
						blood+= mul4*0.3*cells[Layer::MEATS][scx][scy];
					}
				}*/
					
		for (int j=0; j<(int)agents.size(); j++) {
			if (i==j) continue;
			Agent* a2= &agents[j];

			if (a->pos.x<a2->pos.x-conf::DIST || a->pos.x>a2->pos.x+conf::DIST
					|| a->pos.y>a2->pos.y+conf::DIST || a->pos.y<a2->pos.y-conf::DIST) continue;

			float d= (a->pos-a2->pos).length();

			if (d<conf::DIST) {

				//smell
				smellsum+= a->smell_mod*(1-d/conf::DIST);

				//sound and hearing
				for (int q=0;q<NUMEARS;q++){

					Vector2f v(a->radius, 0);
					v.rotate(a->angle + a->eardir[q]);

					Vector2f earpos= a->pos+ v;

					float eardist= (earpos-a2->pos).length();

					//sound
					soundsum[q]+= a->sound_mod*(1-eardist/conf::DIST)*(max(fabs(a2->w1),fabs(a2->w2)));

					//hearing. Listening to other agents
					hearsum[q]+= a->hear_mod*(1-eardist/conf::DIST)*a2->volume;
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
						r[q] += mul1*a2->red;
						g[q] += mul1*a2->gre;
						b[q] += mul1*a2->blu;
						if(a->id==SELECTION && isDebug()){
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
					blood+= a->blood_mod*mul4*(1-agents[j].health/2); //remember: health is in [0 2]
					//agents with high life dont bleed. low life makes them bleed more
				}
			}
		}

		//temperature varies from 0 to 1 across screen.
		//it is 0 at equator (in middle), and 1 on edges. Agents can sense this range
		float temp= (float)2.0*abs(a->pos.y/conf::HEIGHT - 0.5);

//		for(int e;e<NUMEYES;e++){

		a->in[Input::RED1]= cap(r[0]);
		a->in[Input::GRE1]= cap(g[0]);
		a->in[Input::BLU1]= cap(b[0]);
		
		a->in[Input::RED2]= cap(r[1]);
		a->in[Input::GRE2]= cap(g[1]);
		a->in[Input::BLU2]= cap(b[1]);

		a->in[Input::RED3]= cap(r[2]);
		a->in[Input::GRE3]= cap(g[2]);
		a->in[Input::BLU3]= cap(b[2]);

		a->in[Input::RED4]= cap(r[3]);
		a->in[Input::GRE4]= cap(g[3]);
		a->in[Input::BLU4]= cap(b[3]);

		a->in[Input::CLOCK1]= abs(sin((modcounter+current_epoch*conf::FRAMES_PER_EPOCH)/a->clockf1));
		a->in[Input::CLOCK2]= abs(sin((modcounter+current_epoch*conf::FRAMES_PER_EPOCH)/a->clockf2));
		a->in[Input::SOUND1]= cap(soundsum[0]);
		a->in[Input::SOUND2]= cap(soundsum[1]);
		a->in[Input::HEARING1]= cap(hearsum[0]);
		a->in[Input::HEARING2]= cap(hearsum[1]);
		a->in[Input::BOT_SMELL]= cap(smellsum);
		a->in[Input::BLOOD]= cap(blood);
		a->in[Input::TEMP]= temp;
		if (a->id==SELECTION) {
			a->in[Input::PLAYER]= pinput1;
		} else {
			a->in[Input::PLAYER]= 0.0;
		}
		a->in[Input::FRUIT_SMELL]= cap(fruit);
		a->in[Input::MEAT_SMELL]= cap(meat);
		a->in[Input::HAZARD_SMELL]= cap(hazard);
		a->in[Input::WATER_SMELL]= cap(water);
		a->in[Input::RANDOM]= cap(randn(a->in[Input::RANDOM],0.08));
	}
}

void World::brainsTick()
{
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		agents[i].tick();
	}
}

void World::processOutputs()
{
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];

		if (a->health<=0) continue; //skip dead agents
		if (a->jump<=0) { //if not jumping, then change wheel speeds. otherwise, we want to keep wheel speeds constant
			if (pcontrol && a->id==SELECTION) {
				a->w1= pright;
				a->w2= pleft;
			} else {
				a->w1= a->out[Output::LEFT_WHEEL_F] - a->out[Output::LEFT_WHEEL_B];
				a->w2= a->out[Output::RIGHT_WHEEL_F] - a->out[Output::RIGHT_WHEEL_B];
			}
		}
		a->red+= 0.2*(a->out[Output::RED]-a->red);
		a->gre+= 0.2*(a->out[Output::GRE]-a->gre);
		a->blu+= 0.2*(a->out[Output::BLU]-a->blu);
		if (a->jump<=0) a->boost= a->out[Output::BOOST]>0.5; //if jump height is zero, boost can change
		a->volume= a->out[Output::VOLUME];
		a->give= a->out[Output::GIVE];
		a->sexproject= a->out[Output::PROJECT]>=0.5 ? true : false;

		//spike length should slowly tend towards spike output
		float g= a->out[Output::SPIKE];
		if (a->spikeLength<g) { //its easy to retract spike, just hard to put it up
			a->spikeLength+=conf::SPIKESPEED;
			a->health-= conf::HEALTHLOSS_SPIKE_EXT;
			a->writeIfKilled("Killed by Spike Raising");
		} else if (a->spikeLength>g) a->spikeLength= g;

		//grab gets set
		a->grabbing= a->out[Output::GRAB];

		//jump gets set to 2*((jump output) - 0.5) if itself is zero (the bot is on the ground) and if jump output is greater than 0.5
		float height= (a->out[Output::JUMP] - 0.5)*2;
		if (a->jump==0 && height>0 && a->age>0) a->jump= height;

		//jaw *snap* mechanic
		float newjaw= cap(a->out[Output::JAW]-a->jawOldPos);

		if(a->jawPosition>0) a->jawPosition*= -1;
		else if (a->jawPosition<0) a->jawPosition= min(0.0, a->jawPosition + 0.05*(2 + a->jawPosition));
		else if (a->age>0) a->jawPosition= newjaw;
		a->jawOldPos= a->out[Output::JAW];

	}

	//move bots
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];

		a->jump-= conf::GRAVITYACCEL;
		if(a->jump<-1) a->jump= 0; //-1 because we will be nice and give a "recharge" time between jumps

		Vector2f v1(a->radius/2, 0);
		v1.rotate(a->angle + M_PI/2);

		float BW1= conf::BOTSPEED*a->metabolism*a->w1;
		float BW2= conf::BOTSPEED*a->metabolism*a->w2;
		if (a->boost) { //if boosting
			BW1=BW1*conf::BOOSTSIZEMULT;
			BW2=BW2*conf::BOOSTSIZEMULT;
		}

		//move bots
		Vector2f vv1= v1;
		Vector2f vv2= -v1;
		vv1.rotate(-BW1);
		vv2.rotate(BW2);
		a->pos= a->pos+(vv1 - v1)+(vv2 + v1);

		//angle bots
		if (a->jump<=0) {
			a->angle += BW2-BW1;
		}
		if (a->angle<-M_PI) a->angle= M_PI - (-M_PI-a->angle);
		if (a->angle>M_PI) a->angle= -M_PI + (a->angle-M_PI);

		//wrap around the map
		a->borderRectify();
	}

	//process interaction with cells
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];

		int scx= (int) capm(a->pos.x/conf::CZ, 0, conf::WIDTH/conf::CZ);
		int scy= (int) capm(a->pos.y/conf::CZ, 0, conf::HEIGHT/conf::CZ);

		if (a->health>0 && a->jump<=0){ //no interaction with these cells if jumping or dead
			//plant food
			float food= cells[Layer::PLANTS][scx][scy];
			float plantintake= 0;
			if (food>0 && a->health<2) {
				//agent eats the food
				float speedmul= 1-0.5*max(abs(a->w1), abs(a->w2));
				//Plant intake is proportional to plant stomach, inverse to meat & fruit stomach, and speed
				plantintake= min(food,conf::FOODINTAKE)*a->stomach[Stomach::PLANT]*
							 (1-a->stomach[Stomach::MEAT]/2)*(1-a->stomach[Stomach::FRUIT]/2)*speedmul;
				if (a->health>=conf::MINMOMHEALTH) a->repcounter -= a->metabolism*plantintake;
				a->health+= plantintake;
				cells[Layer::PLANTS][scx][scy]-= min(food,conf::FOODWASTE*plantintake/conf::FOODINTAKE);
			}

			//meat food
			float meat= cells[Layer::MEATS][scx][scy];
			float meatintake= 0;
			if (meat>0 && a->health<2) {
				//agent eats meat
				float speedmul= 1-0.5*max(abs(a->w1), abs(a->w2));
				meatintake= min(meat,conf::MEATINTAKE)*a->stomach[Stomach::MEAT]*
					(1-a->stomach[Stomach::PLANT]/2)*(1-a->stomach[Stomach::FRUIT]/2)*speedmul;
				if (a->health>=conf::MINMOMHEALTH) a->repcounter -= a->metabolism*meatintake;
				a->health += meatintake;
				cells[Layer::MEATS][scx][scy]-= min(meat,conf::MEATWASTE*meatintake/conf::MEATINTAKE);
			}

			//Fruit food
			float fruit= cells[Layer::FRUITS][scx][scy];
			float fruitintake= 0;
			if (fruit>0 && a->health<2) {
				//agent eats fruit
				float speedmul= 1-0.5*max(abs(a->w1), abs(a->w2));
				fruitintake= min(fruit,conf::FRUITINTAKE)*a->stomach[Stomach::FRUIT]*
					(1-a->stomach[Stomach::MEAT]/2)*(1-a->stomach[Stomach::PLANT]/2)*speedmul;
				if (a->health>=conf::MINMOMHEALTH) a->repcounter -= a->metabolism*fruitintake;
				a->health += fruitintake;
				cells[Layer::FRUITS][scx][scy]-= min(fruit,conf::FRUITWASTE*fruitintake/conf::FRUITINTAKE);
			}

			//hazards
			float hazard= cells[Layer::HAZARDS][scx][scy];
			if (hazard>0){
				a->health -= conf::HAZARDDAMAGE*hazard;
				a->writeIfKilled("Killed by a Hazard");
			}
			//agents fill up hazard cells only up to 9/10, because any greater can be reset to zero
			if (modcounter%5==0){
				if(hazard<conf::HAZARDMAX*9/10) hazard+= conf::HAZARDDEPOSIT;
				cells[Layer::HAZARDS][scx][scy]= capm(hazard, 0, conf::HAZARDMAX*9/10);
			}

			//land/water
			float air= cells[Layer::LAND][scx][scy];
			if (pow(air-a->lungs,2)>=0.01){
				a->health -= conf::HEALTHLOSS_BADAIR*pow(air-a->lungs,2);
				a->writeIfKilled("Killed by Suffocation .");
			}

			if (a->health>2){ //if health has been increased over the cap, convert the overflow into repcounter
				a->repcounter -= a->metabolism*(a->health-2);
				a->health= 2;
			}
		}
	}

	//process giving and receiving of food
	//CONSIDER: making this modcounter-based w/ timed reset
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].health<=0) continue; //skip dead agents
		if (agents[i].give>0.5) {
			for (int j=0; j<(int)agents.size(); j++) {
				if (agents[j].health<=0) continue; //skip dead agents
				float d= (agents[i].pos-agents[j].pos).length();
				float rd= (agents[i].give-0.5)*2*conf::FOOD_SHARING_DISTANCE;
				//rd is the max range allowed to agent j, so the agent i can determine how far it shares health
				if (d<rd) {
					//initiate transfer
					agents[j].health += conf::FOODTRANSFER;
					agents[i].health -= conf::FOODTRANSFER;
					agents[j].dfood += conf::FOODTRANSFER; //only for drawing
					agents[i].dfood -= conf::FOODTRANSFER;
					agents[i].writeIfKilled("Killed by Excessive Generosity");
				}
			}
		}
	}

	//process collision dynamics
	if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents
		#pragma omp parallel for schedule(dynamic)
		for (int i=0; i<(int)agents.size(); i++) {
			Agent* a= &agents[i];

			for (int j=0; j<(int)agents.size(); j++) {
				if (i==j) continue;
				if (agents[j].health<=0){
					if(a->grabbing>0.5 && a->grabID==agents[j].id) a->grabbing= -1;
					if(agents[j].health==0) continue; //health == because we want to weed out bots who died already via other causes
				}

				Agent* a2= &agents[j];

				float d= (a->pos-a2->pos).length();
				if (d>conf::DIST) continue;

				float sumrad= a->radius+a2->radius;

				//---COLLISIONS---///
				if (d<sumrad && a->jump<=0 && a2->jump<=0) {
					//if inside each others radii and neither are jumping, fix physics
					float ov= (sumrad-d);
					if (ov>0 && d>0.0001) {
						if (ov>conf::TOOCLOSE) {//if bots are too close, they get injured before being pushed away
							float DMG= ov*conf::HEALTHLOSS_BUMP;
							a->health-= DMG*sumrad/2/a->radius; //larger bots take less damage, bounce less
							a2->health-= DMG*sumrad/2/a2->radius;
							const char * fix= "Killed by a Collision";
							a->writeIfKilled(fix);
							a2->writeIfKilled(fix);// fix, because these two collisions are not being redd the same
							a->initEvent(15,0,0.5,1);
							a2->initEvent(15,0,0.5,1);
							
							a->freshkill= conf::FRESHKILLTIME; //this agent was hit this turn, giving full meat value
							a2->freshkill= conf::FRESHKILLTIME;
						}
						float ff1= capm(ov/d*a2->radius/a->radius*conf::REACTPOWER, 0, 2); //the radii come in here for inertia-like effect
						float ff2= capm(ov/d*a->radius/a2->radius*conf::REACTPOWER, 0, 2);
						a->pos.x-= (a2->pos.x-a->pos.x)*ff1;
						a->pos.y-= (a2->pos.y-a->pos.y)*ff1;
						a2->pos.x+= (a2->pos.x-a->pos.x)*ff2;
						a2->pos.y+= (a2->pos.y-a->pos.y)*ff2;

						a->borderRectify();
						a2->borderRectify();

//						printf("%f, %f, %f, %f, and %f\n", a->pos.x, a->pos.y, a2->pos.x, a2->pos.y, ov);
					}
				} //end collision mechanics

				//---GRABBING---//
				if(a->grabbing>0.5) {
					if(d<=conf::GRABBING_DISTANCE+a->radius){
						//check initializing grab
						if(a->grabID==-1){
							if(randf(0,1)<0.4) a->grabID= a2->id; //40% chance we'll grab any bot around us

						} else if (a->grabID==a2->id && d>(sumrad+1.0)) {
							//grab the coords of the other agent
							a->grabx= a2->pos.x; a->graby= a2->pos.y;
							//we have a grab target, and it is this agent. Pull us together!
							float dpref= sumrad + 1.0;
							float ff1= -dpref*a2->radius/a->radius*a->grabbing*0.005; //the radii come in here for inertia-like effect
							float ff2= -dpref*a->radius/a2->radius*a->grabbing*0.005;
							a->pos.x-= (a2->pos.x-a->pos.x)*ff1;
							a->pos.y-= (a2->pos.y-a->pos.y)*ff1;
							a2->pos.x+= (a2->pos.x-a->pos.x)*ff2;
							a2->pos.y+= (a2->pos.y-a->pos.y)*ff2;

							a->borderRectify();
							a2->borderRectify();
						}
					} else if (a->grabID==a2->id) {
						//grab distance exceeded, break the bond
						a->grabID= -1;
					}
				} else {
					//if we can't grab, clear the grab target
					a->grabID= -1;
				} //end grab mechanics

				const char * fix2= "Killed by a Murder";

				//---SPIKING---//
				//low speed doesn't count, nor does a small spike (duh). If the target is jumping in midair, can't attack either
				if(d<=(sumrad + conf::SPIKELENGTH*a->spikeLength) && a->w1>=0.1 && a->w2>=0.1 && a2->jump<=0){

					//these two are in collision and agent i has extended spike and is going decent fast!
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/8) {
						//bot i is also properly aligned!!! that's a hit
						float DMG= conf::SPIKEMULT*a->spikeLength*max(fabs(a->w1),fabs(a->w2));

						a2->health-= DMG;
						a2->writeIfKilled(fix2);

						a->spikeLength= 0; //retract spike back down

						a->initEvent(20*DMG,1,0.5,0); //orange event means bot has spiked the other bot. nice!
						if (a2->health<=0){ 
							//red event means bot has killed the other bot. Murderer!
							a->initEvent(20*DMG,1,0,0);
							a->killed++; //increment stat
						}

						/*Vector2f v2(1,0);
						v2.rotate(a2->angle);
						float adiff= v.angle_between(v2);
						if (fabs(adiff)<M_PI/2) {
							//this was attack from the back. Retract spike of the other agent (startle!)
							//this is done so that the other agent cant right away "by accident" attack this agent
							a2->spikeLength= 0;
						}*/
						
						a2->freshkill= conf::FRESHKILLTIME; //this agent was hit this turn, giving full meat value

						a->hits++; //increment stat
					}
				} //end spike mechanics

				//---JAWS---//
				if(a->jawPosition>0 && d<=(sumrad+5.0)) { //only bots that are almost touching may chomp
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/4) { //wide AOE
						float DMG= conf::HEALTHLOSS_JAWSNAP*a->jawPosition;

						a2->health-= DMG;
						a2->writeIfKilled(fix2);
						a2->freshkill= conf::FRESHKILLTIME; //this agent was hit this turn, giving full meat value

						a->initEvent(30*DMG,1,1,0); //yellow event means bot has chomped the other bot. ouch!
						if (a2->health<=0){ 
							//red event means bot has killed the other bot. Murderer!
							a->initEvent(30*DMG,1,0,0);
							a->killed++; //increment stat
						}
					}
				} //end jaw mechanics
			}
		}
	}
}

void World::healthTick()
{
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].health>0){
			//remove health from lack of "air"
			//straight up penalty to all bots for large population
			agents[i].health-= conf::HEALTHLOSS_NOOXYGEN*agents.size()/conf::TOOMANYBOTS;
			agents[i].writeIfKilled("Killed by LackOf Oxygen");
			if (agents[i].health==0) continue; //agent died, we must move on.

			//now for "natural" causes of death: age, metabolism, excesive brain activity
			//baseloss starts by penalizing fast movement (really should be energy...)
			float baseloss= conf::HEALTHLOSS_WHEELS*(fabs(agents[i].w1) + fabs(agents[i].w2))/2;

			//getting older reduces health.
			baseloss += agents[i].age/conf::MAXAGE*conf::HEALTHLOSS_AGING;

			//metabolism loss
			baseloss *= agents[i].metabolism/conf::MAXMETABOLISM;

			//if boosting, init (baseloss + age loss) * metab loss is multiplied by boost loss
			if (agents[i].boost) {
				baseloss *= conf::HEALTHLOSS_BOOSTMULT;
			}

			//brain activity reduces health
			baseloss += conf::HEALTHLOSS_BRAINUSE*agents[i].brainact;

			//baseloss reduces health
			agents[i].health -= baseloss;
			agents[i].writeIfKilled("Killed by Natural Causes"); //this method should be called after every reduction of health
			if (agents[i].health==0) continue; //agent died, we must move on!

			//process temperature preferences
			//calculate temperature at the agents spot. (based on distance from horizontal equator)
			float dd= 2.0*abs(agents[i].pos.y/conf::HEIGHT - 0.5);
			float discomfort= sqrt(abs(dd-agents[i].temperature_preference));
			if (discomfort<0.005) discomfort=0;
			agents[i].health -= conf::HEALTHLOSS_BADTEMP*discomfort; //subtract from health
			agents[i].writeIfKilled("Killed by Temp Discomfort");
		}
	}
}

void World::setSelection(int type) {
	int maxi= -1;

	if (modcounter%5==0){
		if (type==Select::NONE) {
			SELECTION= -1;
		} else if (type==Select::OLDEST) {
			int maxage= -1;
			for (int i=0; i<(int)agents.size(); i++) {
			   if (agents[i].age>maxage) { 
				   maxage= agents[i].age; 
				   maxi= i;
			   }
			}
		} else if(type==Select::BEST_GEN){
			int maxgen= 0;
			for (int i=0; i<(int)agents.size(); i++) {
			   if (agents[i].gencount>maxgen) { 
				   maxgen= agents[i].gencount; 
				   maxi= i;
			   }
			}
		} else if(type==Select::HEALTHY){
			float maxhealth= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				if (agents[i].health>maxhealth) {
					maxhealth= agents[i].health;
					maxi= i;
				}
			}
		} else if(type==Select::PRODUCTIVE){
			float maxprog= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				if(agents[i].age!=0){
					if (((float)agents[i].children/(float)agents[i].age)>maxprog){
						maxprog= (float)agents[i].children/(float)agents[i].age;
						maxi= i;
					}
				}
			}
		} else if (type==Select::AGGRESSIVE){
			float maxindex= 0;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= 0.25*agents[i].hits + agents[i].killed;
				if (index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		} else if (type==Select::SOCIAL){
			float maxindex= 0;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= agents[i].out[Output::GIVE]+0.25*agents[i].out[Output::PROJECT];
				if (index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		}
	}
			
	if (maxi!=-1 && agents[maxi].id!= SELECTION) {
		setSelectedAgent(maxi);
	}
}

void World::setSelectedAgent(int idx) {
	if (agents[idx].id==SELECTION) SELECTION= -1; //toggle selection if already selected
	else SELECTION= agents[idx].id; //otherwise, select as normal

	agents[idx].initEvent(10,1.0,1.0,1.0);
	setControl(false);
}

int World::getSelectedAgent() const {
	//retrieve array index of selected agent, returns -1 if none
	int idx= -1;
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].id==SELECTION) idx= i;
	}
	return idx;
}
int World::getSelection() const {
	//retrieve world->SELECTION
	return SELECTION;
}

void World::selectedHeal() {
	//heal selected agent
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].id==SELECTION) {
			agents[i].health= 2.0;
		}
	}
}

void World::selectedKill() {
	//kill (delete) selected agent
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].id==SELECTION) {
			agents[i].health= -0.01;
			agents[i].writeIfKilled("Killed by God (you)");
		}
	}
}

void World::selectedBabys() {
	//force selected agent to assexually reproduce
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].id==SELECTION) {
			reproduce(i, i);
		}
	}
}

void World::getFollowLocation(float &xi, float &yi) {
	int idx= getSelectedAgent();
	if (idx>=0){
		xi= agents[idx].pos.x;
		yi= agents[idx].pos.y;
	}
}

bool World::processMouse(int button, int state, int x, int y, float scale)
{
	//now returns true if an agent was selected manually
	if(button==0 && state==1){
		float mind= 1e10;
		int mini= -1;
		float d;

		for (int i=0; i<(int)agents.size(); i++) {
			d= pow(x-agents[i].pos.x,2)+pow(y-agents[i].pos.y,2);
				if (d<mind) {
					mind= d;
					mini= i;
				}
			}
		if (mind<3500/scale) {
			//toggle selection of this agent
			setSelectedAgent(mini);
			return true;
		} else return false;
	} else return false;
}
	 
void World::draw(View* view, int layer)
{
	//draw cell layer
	if(layer!=0) {
		for(int x=0;x<CW;x++) {
			for(int y=0;y<CH;y++) {
				float val= 0;
				if (layer==Layer::PLANTS+1) { 
					//plant food
					val= 0.5*cells[Layer::PLANTS][x][y]/conf::FOODMAX;
				} else if (layer==Layer::MEATS+1) { 
					//meat food
					val= 0.5*cells[Layer::MEATS][x][y]/conf::MEATMAX;
				} else if (layer==Layer::HAZARDS+1) {
					//hazards
					val = 0.5*cells[Layer::HAZARDS][x][y]/conf::HAZARDMAX;
				} else if (layer==Layer::FRUITS+1) {
					//fruit food
					val = 0.5*cells[Layer::FRUITS][x][y]/conf::FRUITMAX;
				} else if (layer==Layer::LAND+1) {
					//land
					val = cells[Layer::LAND][x][y];
				}
/*				} else if (layer==TEMPLAYER) { 
					//temperature
					val = cells[TEMPLAYER][x][y];
				}*/

				view->drawCell(x,y,val);
			}
		}
	}

	view->drawData();
	
	//draw all agents
	vector<Agent>::const_iterator it;
	for ( it = agents.begin(); it != agents.end(); ++it) {
		view->drawAgent(*it,it->pos.x,it->pos.y);
	}

	view->drawStatic();
}

void World::addAgents(int num, int set_stomach, float nx, float ny, bool set_lungs)
{
	for (int i=0;i<num;i++) {
		Agent a;

		if (set_stomach==Stomach::PLANT) a.setHerbivore(); //if told to predetermine stomach type
		else if (set_stomach==Stomach::MEAT) a.setCarnivore();
		else if (set_stomach==Stomach::FRUIT) a.setFrugivore();

		if (set_lungs){ //if told to fix lungs for agent's position
			int scx= (int) a.pos.x/conf::CZ;
			int scy= (int) a.pos.y/conf::CZ;
			a.lungs= cap(randn((float)cells[Layer::LAND][scx][scy],0.5));
		}

		if (nx!=-1 || ny!=-1){ //if custom location given
			a.pos.x= nx;
			a.pos.y= ny;
		}

		a.id= idcounter;
		idcounter++;
		agents.push_back(a);

	}
}

void World::reproduce(int i1, int i2)
{
	Agent mother= agents[i1]; //mother is reproducing agent. her health, her repcounter, and her base stats are divided/reset/used
	Agent father= agents[i2]; //father is a nearby agent that may have been sexting, or just the mother again. either way, repcounter is reset

	float MR= min(mother.MUTRATE1,father.MUTRATE1);
	float MR2= min(mother.MUTRATE2,father.MUTRATE2); //Using min because max, though correct, will lead to rampant mutation (is that perhaps wanted?) will test
	if (randf(0,1)<0.04) MR= MR*randf(1.1, 5);
	if (randf(0,1)<0.04) MR2= MR2*randf(1.1, 5);

	float healthloss= 0; //health lost by mother for assexual reproduction
	if (i1==i2){ //if assexual rep, father is just the mother again
		father= agents[i1];
		healthloss= agents[i1].radius/conf::MEANRADIUS*conf::HEALTHLOSS_ASSEX;

		agents[i1].initEvent(30,0,0.8,0); //green event means agent asexually reproduced

	}else{ //otherwise, it's sexual
		agents[i1].initEvent(30,0,0,0.8);
		agents[i2].initEvent(30,0,0,0.8); //blue events mean agents sexually reproduced.
	}

	float newhealth= agents[i1].health/(agents[i1].numbabies + 1) - mother.repcounter/conf::REPRATE;
	//repcounter should be negative or zero here, so its actually giving more health

	agents[i1].health= newhealth - healthloss;
	agents[i1].writeIfKilled("Killed by Child Birth");

	if (SELECTION==agents[i1].id) printf("The Selected Agent has Reproduced and had %i Babies!\n", agents[i1].numbabies);

	#pragma omp parallel for ordered schedule(dynamic) //allow the creation of each baby to happen on a new thread
	for (int i=0;i<agents[i1].numbabies;i++) {
		Agent daughter = mother.reproduce(father,MR,MR2);
		
		#pragma omp ordered //restore order and collapse threads
		{
		if (i1!=i2){
			daughter.hybrid= true; //if parents are not the same agent (sexual reproduction), then mark the child
			agents[i2].children++;
		}

		daughter.health= newhealth;

		agents[i1].children++;

		daughter.id= idcounter;
		idcounter++;
		agents.push_back(daughter);
		}
	}
}

void World::writeReport()
{
	printf("Writing Report, Epoch: %i\n", current_epoch);
	//save all kinds of nice data stuff
	int topcarngen= 0;
	int topfruggen= 0;
	int topherbgen= 0;
	int toplandgen= 0;
	int topwatergen= 0;
	int randgen= 0;
	int randspec= 0;
	int randseed= 0;
	//int happyspecies=0;
	//vector<int> species;
	//vector<int> members;
	//species.resize(1,0);
	//members.resize(1,0);

	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].isHerbivore() && agents[i].gencount>topherbgen) topherbgen= agents[i].gencount;
		if(agents[i].isCarnivore() && agents[i].gencount>topcarngen) topcarngen= agents[i].gencount;
		if(agents[i].isFrugivore() && agents[i].gencount>topfruggen) topfruggen= agents[i].gencount;
		if(agents[i].lungs>0.5 && agents[i].gencount>toplandgen) toplandgen= agents[i].gencount;
		if(agents[i].lungs<=0.5 && agents[i].gencount>topwatergen) topwatergen= agents[i].gencount;

		/*for(int s=0;s<(int)species.size();s++){ //for every logged species...

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
	randgen= agents[randi(0,agents.size())].gencount;
	randspec= agents[randi(0,agents.size())].species;
	int randagent= randi(0,agents.size());
	for (int i= 0; i<(int)agents[randagent].brain.boxes.size(); i++){
		if (agents[randagent].brain.boxes[i].seed>randseed) randseed=agents[randagent].brain.boxes[i].seed;
	}

/*	for(int s=0;s<(int)species.size();s++){
		if(members[s]>=3) happyspecies++; //3 agents with the same species id counts as a species
	}*/

	//death cause reporting
	std::vector<const char *> deathlog;
	std::vector<int> deathcounts;
	for (int d=0; d<(int)deaths.size(); d++) {
		bool added= false;
		for (int e=0; e<(int)deathlog.size(); e++) {
			if (added) continue;
			if (deaths[d]==deathlog[e]) {
				deathcounts[e]++;
				added= true;
			}
		}
		if (!added) {
			deathlog.push_back(deaths[d]);
			deathcounts.push_back(1);
		}
	}

	FILE* fr = fopen("report.txt", "a");
	//print basics: Epoch and Agent counts
	fprintf(fr, "Epoch: %i #Agents: %i #Herbi: %i #Frugi: %i #Carni: %i #Terra: %i #Aqua: %i #Hybrids: %i #Spikes: %i ",
		current_epoch, getAgents(), getHerbivores(), getFrugivores(), getCarnivores(), getLungLand(), getLungWater(), getHybrids(), getSpiked());
	//print world stats: cell counts
	fprintf(fr, "#0.75Plant: %i #0.5Meat: %i #0.5Hazard: %i #0.5Fruit: %i ",
		getFood(), getMeat(), getHazards(), getFruit());
	//print random selections: Genome, brain seeds, [[[generation]]]
	fprintf(fr, "RandGenome: %i RandSeed: %i RandGen: %i ",
		randspec, randseed, randgen);
	//print generations: Top Gen counts
	fprintf(fr, "TopHGen: %i TopFGen: %i TopCGen: %i TopLGen: %i TopAGen: %i ",
		topherbgen, topfruggen, topcarngen, toplandgen, topwatergen);
	//print deaths: Number and Causes
	fprintf(fr, "#Deaths: %i DEATHLOG: ",
		deaths.size());
	for (int e=0; e<(int)deathlog.size(); e++) {
		fprintf(fr, "%s: %i ", deathlog[e], deathcounts[e]);
	}
	fprintf(fr, "\n");
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
	for(int cx=0; cx<(int)CW; cx++){
		for(int cy=0; cy<(int)CH; cy++){
			for(int l=0;l<Layer::LAYERS;l++){
				cells[l][cx][cy]= 0;
			}
//			cells[TEMPLAYER][cx][cy]= 2.0*abs((float)cy/CH - 0.5); [old temperature indicating code]
		}
	}

	//open report file; null it up if it exists
	FILE* fr = fopen("report.txt", "w");
	fclose(fr);

	ptr=0;
}

void World::spawn()
{
	while(getFood()<conf::INITFOOD) {
		//pollinate plants
		int rx= randi(0,CW);
		int ry= randi(0,CH);
		if(cells[Layer::PLANTS][rx][ry]!=0) continue;
		cells[Layer::PLANTS][rx][ry] = conf::FOODMAX;
		if (getFruit()<conf::INITFRUIT) {
			//pollinate fruit
			cells[Layer::FRUITS][rx][ry] = conf::FRUITMAX;
		}
	}

	//spawn land masses
	cellsLandMasses();

	//add init agents
	addAgents(conf::NUMBOTS, Stomach::PLANT);
}

void World::setClosed(bool close)
{
	CLOSED = close;
}

bool World::isClosed() const
{
	return CLOSED;
}

int World::getHerbivores() const
{
	int count= 0;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].stomach[0]>=agents[i].stomach[1] && agents[i].stomach[0]>=agents[i].stomach[2]) count++;
	}
	return count;
}

int World::getCarnivores() const
{
	int count= 0;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].stomach[1]>=agents[i].stomach[0] && agents[i].stomach[1]>=agents[i].stomach[2]) count++;
	}
	return count;
}

int World::getFrugivores() const
{
	int count= 0;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].stomach[2]>=agents[i].stomach[0] && agents[i].stomach[2]>=agents[i].stomach[1]) count++;
	}
	return count;
}

int World::getLungLand() const
{
	int count= 0;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].lungs>0.5) count++;
	}
	return count;
}

int World::getLungWater() const
{
	int count= 0;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].lungs<=0.5) count++;
	}
	return count;
}

int World::getAgents() const
{
	return agents.size();
}

int World::getFood() const //count plant cells with 75% max or more
{
	int count=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float food= 0.5*cells[Layer::PLANTS][i][j]/conf::FOODMAX;
			if(food>conf::FOODMAX*3/4){
				count++;
			}
		}
	}
	return count;
}

int World::getFruit() const //count fruit cells with 50% max or more
{
	int count=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float fruit= 0.5*cells[Layer::FRUITS][i][j]/conf::FRUITMAX;
			if(fruit>conf::FRUITMAX/2){
				count++;
			}
		}
	}
	return count;
}

int World::getMeat() const //count meat cells with 50% max or more
{
	int count=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float meat= 0.5*cells[Layer::MEATS][i][j]/conf::MEATMAX;
			if(meat>conf::MEATMAX/2){
				count++;
			}
		}
	}
	return count;
}

int World::getHazards() const //count hazard cells with 50% max or more
{
	int count=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float hazard= 0.5*cells[Layer::HAZARDS][i][j]/conf::HAZARDMAX;
			if(hazard>conf::HAZARDMAX*0.5){
				count++;
			}
		}
	}
	return count;
}

int World::getHybrids() const //count number of agents that are hybrids
{
	int count= 0;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].hybrid) count++;
	}
	
	return count;
}

int World::getSpiked() const //counts number of agents with spikes
{
	int count= 0;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].spikeLength*conf::SPIKELENGTH>=agents[i].radius) count++;
	}
	return count;
}
/*
int World::getSelected() const //returns the ID of the selected agent
{
	int id= -1;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].selectflag) id= i;
	}
	return id;
}*/

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

bool World::isAutoselecting() const
{
	return AUTOSELECT;
}

void World::setAutoselect(bool state)
{
	AUTOSELECT= state;
}

void World::cellsRandomFill(int layer, float amount, int number)
{
	for (int i=0;i<number;i++) {
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[layer][cx][cy]= amount;
	}
}

void World::cellsLandMasses()
{
	//creates land masses for the layer given
	int leftcount= CW*CH;

	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			cells[Layer::LAND][i][j]= -1; //"null" all cells
		}
	}

	for (int i=0;i<1.5*(1-pow(conf::OCEANPERCENT,6))*(conf::CONTINENTS+1);i++) {
		//spawn init continents (land= 1)
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::LAND][cx][cy]= 1;
	}
	for (int i=0;i<1.5*(sqrt((float)CW*CH)/1000*pow((float)2.5,5*conf::OCEANPERCENT)*(conf::CONTINENTS+1))-1;i++) {
		//spawn oceans (water= 0)
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::LAND][cx][cy]= 0;
	}

	while(leftcount!=0){
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				//land spread
				if (cells[Layer::LAND][i][j]==1){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2);
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::LAND][ox][oy]==-1 && randf(0,1)<0.9) cells[Layer::LAND][ox][oy]= 1;
				}

				//water spread
				if (cells[Layer::LAND][i][j]==0){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2);
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::LAND][ox][oy]==-1 && randf(0,1)<0.9) cells[Layer::LAND][ox][oy]= 0;
				}
			}
		}
		
		leftcount= 0;
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				if (cells[Layer::LAND][i][j]==-1){
					leftcount++;
				}
			}
		}
	}
}

void World::setDebug(bool state)
{
	DEBUG = state;
}

bool World::isDebug() const
{
	return DEBUG;
}
