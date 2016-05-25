#pragma once

#define NUMEYES 6
#define NUMEARS 2
#define BRAINSIZE 150
#define CONNS 5

#ifndef SETTINGS_H
#define SETTINGS_H


//defines for layer code. Changing order here changes cycle order and menu-listing order
namespace Layer{
enum {
	//NOTE that GLVeiw's use of these must be incremented by 1 because 0 is used for disable draw
	LAND= 0,
	PLANTS,
	FRUITS,
	MEATS,
	HAZARDS,	
	//TEMP,
	LIGHT,

	//Don't add beyond this entry!
	LAYERS
};};

namespace Visual{
enum {
	NONE= 0,
	RGB,
	STOMACH,
	DISCOMFORT,
	VOLUME,	
	SPECIES,
	CROSSABLE,
	HEALTH,

	//Don't add beyond this entry!
	VISUALS
};};

//defines for selection code. Changing order here changes cycle order and menu-listing order
namespace Select {
enum {
	NONE= 0,
	MANUAL,
	OLDEST,
	BEST_GEN,
	HEALTHY,
	PRODUCTIVE,
	AGGRESSIVE,
	SOCIAL,

	//Don't add beyond this entry!
	SELECT_TYPES
};};

//defines for brain input code. Changing order here changes input-to-brain order and and visualization order
namespace Input {
enum {
	// R1 G1 B1  R2 G2 B2  R3 G3 B3  R4 G4 B4 HEALTH CLOCK1 CLOCK2 SOUND HEARING SMELL BLOOD TEMP_DISCOMFORT PLAYER_INPUT1 
	// 0  1  2   3  4  5   6  7  8   9  10 11   12	  13	  14   15,16  17,18	  19	20	       21			   22
	// FOOD_SMELL MEAT_SMELL HAZARD_SMELL WATER_SMELL
	//     23         24          25           26

	RED1= 0,
	GRE1,
	BLU1,
	RED2,
	GRE2,
	BLU2,
	RED3,
	GRE3,
	BLU3,
	RED4,
	GRE4,
	BLU4,
	HEALTH,
	RANDOM,
	CLOCK1,
	CLOCK2,
	CLOCK3,
	HEARING1,
	HEARING2,
	BOT_SMELL,
	BLOOD,
	TEMP,
	PLAYER,
	FRUIT_SMELL,
	MEAT_SMELL,
	HAZARD_SMELL,
	WATER_SMELL,
	
	//Don't add beyond this entry!
	INPUT_SIZE
};};

//defines for brain output code. Changing order here changes brain-to-output order and visualization order
namespace Output {
enum {
	//LEFT RIGHT BOOST JUMP R G B VOLUME GIVING SPIKE CHOICE STIMULANT
	// 0	 1	   2    3   4 5 6   7	   8	  9	    10		11
	LEFT_WHEEL_F= 0,
	RIGHT_WHEEL_F,
	LEFT_WHEEL_B,
	RIGHT_WHEEL_B,
	BOOST,
	JUMP,
	RED,
	GRE,
	BLU,
	VOLUME,
	TONE,
	GIVE,
	SPIKE,
	JAW,
	GRAB,
	PROJECT,
	STIMULANT,
	CLOCKF3,

	//don't add beyond this entry!
	OUTPUT_SIZE
};};

namespace Stomach {
enum {
	PLANT= 0,
	MEAT,
	FRUIT,

	//don't add beyond this entry!
	FOOD_TYPES
};};

namespace conf {
	
	//SIM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SIM
	const int WIDTH = 10000;  //width and height of simulation
	const int HEIGHT = 8000;
	const int WWIDTH = 1100;  //initial window width and height
	const int WHEIGHT = 700;

	const float SNAP_SPEED = 0.2; //how fast snapping to an object of interest is; 1 is instant, 0.1 is smooth, 0 is pointless
	const float ZOOM_SPEED = 0.002; //how fast zoom actions change the magnification

	const int CZ= 50; //cell size in pixels, for food squares. Should divide well into Width, Height
	const int MINFOOD= 2000; //Minimum number of food cells which must have food durring simulation. 0 = off
	const float INITFOODDENSITY= 0.00012; //initial density of full food cells
	const int INITFOOD = (int) (INITFOODDENSITY*WIDTH*HEIGHT); //initial number of full food cells
	const float INITFRUITDENSITY= 0.00008; //initial density of full fruit cells
	const int INITFRUIT = (int) (INITFRUITDENSITY*WIDTH*HEIGHT); //initial number of full fruit cells
	const int NUMBOTS= 30; //initially, and minimally
	const int ENOUGHBOTS= 500; //number of bots where we no longer seed with random spawns
	const int TOOMANYBOTS= 1800; //number of bots at which the full NOAIR healthloss is applied

	const int REPORTS_PER_EPOCH = 10; // number of times to record data per epoch. 0 for off. (David Coleman)
	const int FRAMES_PER_EPOCH = 10000; //number of frames before epoch is incremented by 1.
	const int RECORD_SIZE = 200; // number of data points stored for the graph. Units is in reports, the frequency of which are defined above

	const int CONTINENTS= 2; //number of "continents" generated on the land layer
	const float OCEANPERCENT= 0.65; //decimal percent of terrain layer which will be ocean

	const int FRAMES_PER_DAY= 2500; //number of frames it takes for the daylight cycle to go completely around
	
	const float GRAVITYACCEL= 0.01; //how fast a bot will "fall" after jumping [0= weightless (I don't recommend), 0.5 or more= super-gravity]
	const float REACTPOWER= 0.13; //how strong is the restoring force between two colliding agents?
	const float SPIKEMULT= 2; //strength of spike injury

	//BOTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BOTS
	const float BOTSPEED= 0.3; //fastest possible speed of agents
	const float BOOSTSIZEMULT=2; //how much boost do agents get? when boost neuron is on
	const float SOUNDPITCHRANGE= 0.1; //range below hearhigh and above hearlow within any external sounds fade in
	const float FOODTRANSFER= 0.01; //how much is transfered between two agents trading food? per iteration
	const float MEANRADIUS=10; //average agent radius (only applies to random agents)
	const float SPIKESPEED= 0.005; //how quickly can attack spike go up?
	const int FRESHKILLTIME= 10; //number of ticks after a spike, collision, or bite that a bot will still drop full meat
	const float MINMOMHEALTH=0.15; //minimum amount of health required for an agent to have a child
	const float MAXMETABOLISM=1.5; //agent metabolism is limited to [0,this]
	const float REPRATE=20; //amount of food required to be consumed for an agent to reproduce
	const float LEARNRATE= 0.001; // 0.02 (high-gen feedback) //how quickly a conn weight can change from use
	const float MAXDEVIATION=10; //maximum difference a crossover reproducing agent will be willing to tolerate
	const float METAMUTRATE1= 0.002; //what is the change in MUTRATE1 and 2 on reproduction? lol
	const float METAMUTRATE2= 0.0001;
	const int MAXAGE=1000; //Age at which the full HEALTHLOSS_AGING amount is applied to an agent

	//distances
	const float DIST= 400; //how far can the senses can detect other bots or cells
	const float SPIKELENGTH=30; //full spike length. MUST be less than DIST!!!
	const float TOOCLOSE=14; //how much two agents can be overlapping before they take damage
	const float FOOD_SHARING_DISTANCE= 60; //how far away can food be shared between bots?
	const float SEXTING_DISTANCE= 60; //how far away can two bots sexual reproduce?
	const float GRABBING_DISTANCE= 40; //how far away can a bot grab another? MUST be less than DIST!!!

	const float HEALTHLOSS_WHEELS = 0.00005; //0.00001 //How much health is lost for a bot driving at full speed
	const float HEALTHLOSS_BOOSTMULT=4; //how much boost costs (set to 1 to nullify boost cost; its a multiplier)
	const float HEALTHLOSS_BADTEMP = 0.003; //0.0025 //how quickly does health drain in nonpreferred temperatures (0= disabled. 0.003 is decent value)
	const float HEALTHLOSS_AGING = 0.0001;
	const float HEALTHLOSS_BRAINUSE= 0.001; //0.001 //how much health is reduced for each box in the brain being active
	const float HEALTHLOSS_BUMP= 0.005; //how much health is lost upon collision
	const float HEALTHLOSS_SPIKE_EXT= 0; //how much health a bot looses for extending spike
	const float HEALTHLOSS_BADAIR= 0.01; //how much health is lost if in totally opposite environment
	const float HEALTHLOSS_NOOXYGEN= 0.0003; //how much bots are penalized when total agents = TOOMANYBOTS, applied exponentially
	const float HEALTHLOSS_ASSEX= 0.325; //multiplier for radius/meanradius penalty on mother for asexual reproduction
	const float HEALTHLOSS_JAWSNAP= 0.6; //when jaws snap fully (0->1), this is the damage applied to bots in front

	const float BRAIN_DIRECTINPUT= 0.3; //probability of random brain conns on average which will connect directly to inputs
	const float BRAIN_DEADCONNS= 0.3; //probability of random brain conns which are "dead" (that is, weight = 0)
	const float BRAIN_CHANGECONNS= 0.15; //probablility of random brain conns which are change sensitive
	const float BRAIN_MEMCONN= 0.01; //probablility of random brain conns which are memory type

	//LAYERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LAYERS
	const float FOODINTAKE= 0.008; //how much plant food can feed an agent per tick?
	const float FOODDECAY = 0.000005; //how much does food decay(+)/grow(-) on a cell which already has food?
	const float FOODGROWTH= 0.0000051; //how much does food increase by on a cell with both plant and hazard? (fertilizer)
	const float FOODWASTE= 0.0004; //0.0003 (too much) //how much food disapears if agent eats?
	const float FOODMAX= 0.5; //how much food per cell can there be at max?
	const int FOODADDFREQ= 250; //how often does random square get set to full food?
	const float FOODSPREAD= 0.0001; //0.0002 speedy recovery //probability of a fruit cell seeding food to a nearby cell
	const int FOODRANGE= 2; //distance that single cell of food can seed. in cells.
	//Food currently has these stats:
	//0.5 full food / 0.0004 food eaten = 1250 ticks to consume full food
	//1250 ticks * 0.008 food intake = 10 real intake from 1 full food cell (for perfect herbivore)
	//10 real intake / 30 reprate = 1/3rd of reproduction (very roughly)
	//Plant food is the simplest and most plentiful form of nutrition, but it takes a long time to consume enough

	const float FRUITINTAKE = 0.013; //how much fruit can feed an agent per tick?
	const float FRUITDECAY = 0.0005; //0.001 (too fast)//how much fruit decays on a cell with low plant life?
	const float FRUITWASTE = 0.0005; //0.0008 //how much fruit disapears if agent eats?
	const float FRUITMAX = 0.5;
	const int FRUITADDFREQ = 5; //how often does a high-plant-food cell get set to full fruit?
	const float FRUITREQUIRE= 0.1; //minimum plant health required for fruit to persist on the cell
	//Fruit currently has these stats:
	//0.5 full fruit / 0.00075 fruit eaten = 667 ticks to consume full fruit
	//667 ticks * 0.015 fruit intake = 10 real intake from 1 full fruit cell (for perfect frugivore)
	//10 real intake / 30 reprate = 1/3rd of reproduction (very roughly)
	//Fruit is a quick and easy alternative to plants. Also randomly populated, harkening back to ScriptBots origins

	const float MEATINTAKE= 0.1; //how much meat can feed an agent per tick?
	const float MEATDECAY= 0.0000005; //0.000002 (not enough) //how much meat decays/grows on a cell? through MEATDECAY/[meat_present]
	const float MEATWASTE= 0.002; // 0.0004 (lasts too long) 0.002 (not enough) //how much meat disapears if agent eats?
	const float MEATMAX= 0.5; //how much meat per cell can there be at max?
	const float MEATVALUE= 1; //how much meat a bot's body is worth? in range [0,1]
	//Meat currently has these stats:
	//0.5 full meat / 0.0015 meat eaten = 333 ticks to consume full meat
	//333 ticks * 0.04 meat intake = 13 real intake from 1 full meat cell (for perfect carnivore)
	//13 real intake / 30 reprate = 4/9ths of reproduction (very roughly)
	//Meat comes from dead bots, and is the fastest form of nutrition, IF bots can learn to find it...

	const int HAZARDFREQ= 20; //how often an instant hazard appears?
	const float HAZARDDECAY= 0.000002; //how much non-event hazard decays/grows on a cell per tick?
	const float HAZARDDEPOSIT= 0.0015; //how much hazard is placed by a bot per tick?
	const float HAZARDDAMAGE= 0.016; //how much health a bot looses while on a filled hazard cell per tick?
	const float HAZARDMAX= 0.5; //how much hazard per cell can there be at max? more than 9/10 of this qualifies as an instant hazard
	}

#endif
