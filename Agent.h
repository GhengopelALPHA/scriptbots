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
	float brainActivity;
	void setActivity();
    Agent reproduce(Agent that, float MR, float MR2);
    
    Vector2f pos;

    float health; //in [0,2]. I cant remember why.
    float angle; //of the bot
    
    float red;
    float gre;
    float blu;
    
    float w1; //wheel speeds
    float w2;
    bool boost; //is this agent boosting
	float jump; //what "height" this bot is at after jumping

    float spikeLength;
    int age;

    bool spiked;    
    
    std::vector<float> in; //input: 4 eyes, sensors for R,G,B each, Sound, Smell, Health, Temp discomfort, 
    std::vector<float> out; //output: Left, Right forward motion, R, G, B, SPIKE, Share, Brainmod

    float repcounter; //when repcounter gets to 0, this bot reproduces
	float metabolism; //rate modifier for food to repcounter conversion
    int gencount; //generation counter
    bool hybrid; //is this agent result of crossover?
    float clockf1, clockf2; //the frequencies of the two clocks of this bot
    float soundmul; //sound multiplier of this bot. It can scream, or be very sneaky. This is actually always set to output 8
    
    //variables for drawing purposes
    float indicator;
    float ir;float ig;float ib; //indicator colors
    int selectflag; //is this agent selected?
    float dfood; //what is change in health of this agent due to giving/receiving?

    float give;    //is this agent attempting to give food to other agent?

    int id; 

    //inhereted static stuff
	int species; //if two bots are of significantly different species, then they can't crossover
	float radius; //radius of bot
	float stomach[3]; //stomach: #0 is herbivore, #1 is carnivore, #2 is frugivore

    float MUTRATE1; //how often do mutations occur?
    float MUTRATE2; //how significant are they?
    float temperature_preference; //what temperature does this agent like? [0 to 1]
	float lungs; //what type of environment does this agent need? [0 for water, 1 for land]
    
	//numbers of sensors. UNUSED
	int numears;
	int numeyes;

	float smell_mod;
	float sound_mod;
	float hear_mod;
	float eye_see_agent_mod;
	float eye_see_cell_mod;
	float blood_mod;
    
	std::vector<float> eardir; //position of ears

    std::vector<float> eyefov; //field of view for each eye
    std::vector<float> eyedir; //direction of each eye
    
	//THE BRAIN!!!!
    MLPBrain brain;
	int numinputs; //number of inputs and brain boxes. UNUSED
	int numboxes;
    
    //stats: mutations, children, etc
    std::vector<std::string> mutations;
	int children; //how many kids did you say you had again?
};

#endif // AGENT_H
