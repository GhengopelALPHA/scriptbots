#ifndef AGENT_H
#define AGENT_H

#include "DWRAONBrain.h"
#include "AssemblyBrain.h"
#include "MLPBrain.h"
#include "vmath.h"

#include <vector>
#include <string>

class Agent
{
//IMPORTANT: if ANY variables are added/removed, you MUST check ReadWrite.cpp to see how loading and saving will be effected!!!
public:
	Agent();

	void printSelf();
	//for drawing purposes
	void initEvent(float size, float r, float g, float b);
	
	void tick();
	void setActivity();
	void writeIfKilled(const char * cause);

	Agent reproduce(Agent that, float MR, float MR2);

	//random agent creation tweakers
	void setHerbivore();
	void setCarnivore();
	void setFrugivore();
	void setPos(float x, float y);
	void borderRectify();

	bool isHerbivore();
	bool isCarnivore();
	bool isFrugivore();
	
	//Variables
	//bot basics
	int id;
	Vector2f pos;
	float health; //in range [0,2]. I cant remember why.
	float angle; //of the bot
	float w1; //wheel speeds. in range [-1,1]
	float w2;
	float radius; //radius of bot
	
	//bot colors
	float red;
	float gre;
	float blu;
	
	//abilities
	bool boost; //is this agent boosting?
	float jump; //what "height" this bot is at after jumping
	float spikeLength; //"my, what a long spike you have!"
	float jawPosition; //what "position" the jaw is in. 0 for open, 1 for closed
	float jawOldPos; //the previous "position" of the jaw
	int grabID; //id of agent this agent is "glued" to
	float grabbing; //is this agent attempting to grab another? If already grabbed, how far are we willing to let them go?
	float give;	//is this agent attempting to give food to other agent?
	float volume; //sound volume of this bot. It can scream, or be very sneaky.

	//environment-related
	float stomach[Stomach::FOOD_TYPES]; //stomach: #0 is herbivore, #1 is carnivore, #2 is frugivore
	float temperature_preference; //what temperature does this agent like? [0 to 1]
	float lungs; //what type of environment does this agent need? [0 for "water", 1 for "land"]

	//reproduction
	bool sexproject; //is this bot trying to give out its genetic data?
	int numbabies; //number of children this bot creates with every reproduction event
	float repcounter; //when repcounter gets to 0, this bot reproduces
	float metabolism; //rate modifier for food to repcounter conversion, also, a factor of max bot speed
	int species; //if two bots are of significantly different species, then they can't crossover

	//numbers of sensors. UNUSED
//	int numears;
//	int numeyes;

	//sensors and inputs
	float eye_see_agent_mod;
	float eye_see_cell_mod;
	std::vector<float> eyefov; //field of view for each eye
	std::vector<float> eyedir; //direction of each eye

	float sound_mod;
	float hear_mod;
	std::vector<float> eardir; //position of ears

	float smell_mod;
	
	float clockf1, clockf2; //the frequencies of the two clocks of this bot

	float blood_mod;
	

	//THE BRAIN!!!!
	MLPBrain brain;
	std::vector<float> in; //input: 4 eyes, sensors for R,G,B each, Sound, Smell, Health, Temp discomfort, 
	std::vector<float> out; //output: Left, Right forward motion, R, G, B, SPIKE, Share, Brainmod
	float brainact; //records the activity of the brain
//	int numinputs; //number of inputs and brain boxes. UNUSED
//	int numboxes;
	

	//stats: generation, mutations, children, etc
	int age; //how old is the agent
	int gencount; //generation counter

	float MUTRATE1; //how often do mutations occur?
	float MUTRATE2; //how significant are they?
	std::vector<std::string> mutations;

	int freshkill; //were you just stabbed/collided with? how long ago?
	int children; //how many kids did you say you had again?
	int killed; //how many bots have you murdered???
	int hits; //how much fighting have you done?
	bool hybrid; //is this agent result of crossover?

	const char * death; //the cause of death of this agent

	//variables for drawing purposes
	float indicator;
	float ir, ig, ib; //indicator colors
	float dfood; //what is change in health of this agent due to giving/receiving?
	float dfoodx; //x and y location of other bot
	float dfoody;
	float grabx; //x and y location of the grab target
	float graby;
};

#endif // AGENT_H
