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
	
	//DEFAULTS: All of what follows are defaults, and if settings.cfg exists, are subsituted with that file's values
	//SIM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SIM
	const int WIDTH = 10000;  //width and height of simulation
	const int HEIGHT = 8000;
	const int WWIDTH = 1100;  //initial window width and height
	const int WHEIGHT = 700;

	const float SNAP_SPEED = 0.2; //how fast snapping to an object of interest is; 1 is instant, 0.1 is smooth, 0 is pointless
	const float ZOOM_SPEED = 0.002; //how fast zoom actions change the magnification

	const int CZ= 50; //cell size in pixels, for food squares. Should divide well into Width, Height
	const int MINFOOD= 2000; //(.cfg)
	const float INITFOODDENSITY= 0.00012; //(.cfg)
	const float INITFRUITDENSITY= 0.00008; //(.cfg)
	const int NUMBOTS= 30; //(.cfg)
	const int ENOUGHBOTS= 500; //(.cfg)
	const int TOOMANYBOTS= 1800; //(.cfg)

	const int REPORTS_PER_EPOCH = 10; //(.cfg)
	const int FRAMES_PER_EPOCH = 10000; //(.cfg)
	const int FRAMES_PER_DAY= 2500; //(.cfg)
	const int RECORD_SIZE = 200; // number of data points stored for the graph. Units is in reports, the frequency of which are defined above

	const int CONTINENTS= 2; //(.cfg)
	const float OCEANPERCENT= 0.65; //(.cfg)
	const float GRAVITYACCEL= 0.01; //(.cfg)
	const float REACTPOWER= 0.13; //(.cfg)
	const float SPIKEMULT= 2; //(.cfg)

	//BOTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BOTS
	const float BOTSPEED= 0.3; //(.cfg)
	const float BOOSTSIZEMULT=2; //(.cfg)
	const float SOUNDPITCHRANGE= 0.1; //(.cfg)
	const float FOODTRANSFER= 0.01; //(.cfg)
	const float MEANRADIUS=10; //average agent radius (only applies to random agents)
	const float SPIKESPEED= 0.01; //(.cfg)
	const int FRESHKILLTIME= 10; //(.cfg)
	const int TENDERAGE= 10; //(.cfg)
	const float MINMOMHEALTH=0.15; //(.cfg)
	const float MAXMETABOLISM=1.5; //agent metabolism is limited to [0,this]
	const float REPRATE=20; //amount of food required to be consumed for an agent to reproduce
	const float LEARNRATE= 0.001; // 0.02 (high-gen feedback) //how quickly a conn weight can change from use
	const float MAXDEVIATION=10; //(.cfg)
	const float METAMUTRATE1= 0.002; //what is the change in MUTRATE1 and 2 on reproduction? lol
	const float METAMUTRATE2= 0.0001;
	const int MAXAGE=1000; //(.cfg)

	//distances
	const float DIST= 400; //(.cfg)
	const float SPIKELENGTH=30; //(.cfg)
	const float TOOCLOSE=14; //(.cfg)
	const float FOOD_SHARING_DISTANCE= 60; //(.cfg)
	const float SEXTING_DISTANCE= 60; //(.cfg)
	const float GRABBING_DISTANCE= 40; //(.cfg)

	const float HEALTHLOSS_WHEELS = 0.00005; //(.cfg)
	const float HEALTHLOSS_BOOSTMULT=4; //(.cfg)
	const float HEALTHLOSS_BADTEMP = 0.003; //(.cfg)
	const float HEALTHLOSS_AGING = 0.0001; //(.cfg)
	const float HEALTHLOSS_BRAINUSE= 0.001; //(.cfg)
	const float HEALTHLOSS_BUMP= 0.005; //(.cfg)
	const float HEALTHLOSS_SPIKE_EXT= 0; //(.cfg)
	const float HEALTHLOSS_BADAIR= 0.01; //(.cfg)
	const float HEALTHLOSS_NOOXYGEN= 0.0003; //(.cfg)
	const float HEALTHLOSS_ASSEX= 0.325; //(.cfg)
	const float HEALTHLOSS_JAWSNAP= 0.6; //(.cfg)

	const float BRAIN_DIRECTINPUT= 0.3; //probability of random brain conns on average which will connect directly to inputs
	const float BRAIN_DEADCONNS= 0.3; //probability of random brain conns which are "dead" (that is, weight = 0)
	const float BRAIN_CHANGECONNS= 0.15; //probablility of random brain conns which are change sensitive
	const float BRAIN_MEMCONN= 0.01; //probablility of random brain conns which are memory type

	//LAYERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LAYERS
	const float FOODINTAKE= 0.008; //(.cfg)
	const float FOODDECAY = 0.000005; //(.cfg)
	const float FOODGROWTH= 0.0000051; //(.cfg)
	const float FOODWASTE= 0.0004; //(.cfg)
	const float FOODMAX= 0.5; //how much food per cell can there be at max?
	const int FOODADDFREQ= 250; //(.cfg)
	const float FOODSPREAD= 0.0001; //(.cfg)
	const int FOODRANGE= 2; //(.cfg)
	//Plant food is the simplest and most plentiful form of nutrition, but it takes a long time to consume enough

	const float FRUITINTAKE = 0.013; //(.cfg)
	const float FRUITDECAY = 0.0005; //(.cfg)
	const float FRUITWASTE = 0.0005; //(.cfg)
	const float FRUITMAX = 0.5;
	const int FRUITADDFREQ = 5; //(.cfg)
	const float FRUITREQUIRE= 0.0; //(.cfg)
	//Fruit is a quick and easy alternative to plants. Also randomly populated, harkening back to ScriptBots origins

	const float MEATINTAKE= 0.08; //(.cfg)
	const float MEATDECAY= 0.000001; //(.cfg)
	const float MEATWASTE= 0.002; //(.cfg)
	const float MEATMAX= 0.5; //how much meat per cell can there be at max?
	const float MEATVALUE= 1; //(.cfg)
	//Meat currently has these stats:
	//0.5 full meat / 0.0015 meat eaten = 333 ticks to consume full meat
	//333 ticks * 0.04 meat intake = 13 real intake from 1 full meat cell (for perfect carnivore)
	//13 real intake / 30 reprate = 4/9ths of reproduction (very roughly)
	//Meat comes from dead bots, and is the fastest form of nutrition, IF bots can learn to find it...

	const int HAZARDFREQ= 20; //(.cfg)
	const float HAZARDDECAY= 0.000002; //(.cfg)
	const float HAZARDDEPOSIT= 0.0015; //(.cfg)
	const float HAZARDDAMAGE= 0.015; //(.cfg)
	const float HAZARDMAX= 0.5; //how much hazard per cell can there be at max? more than 9/10 of this qualifies as an instant hazard
	}

#endif
