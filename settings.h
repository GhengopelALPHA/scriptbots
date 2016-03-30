#define INPUTSIZE 23
#define OUTPUTSIZE 12
#define NUMEYES 4
#define NUMEARS 2
#define BRAINSIZE 100
#define CONNS 5
#define LAYERS 5

#ifndef SETTINGS_H
#define SETTINGS_H

namespace conf {
	
	//SIM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SIM
	const int WIDTH = 6000;  //width and height of simulation
	const int HEIGHT = 5000;
	const int WWIDTH = 1100;  //window width and height
	const int WHEIGHT = 700;

	const float SNAP_SPEED = 0.2; //how fast snapping to an object of interest is; 1 is instant, 0.1 is smooth, 0 is pointless
	const float ZOOM_SPEED = 0.002; //how fast zoom actions change the magnification

	const int CZ = 50; //cell size in pixels, for food squares. Should divide well into Width Height
	const int MINFOOD = 2000; //Minimum number of food cells which must have food durring simulation. 0 = off
	const int INITFOOD = 3500; //initial number of full food cells
	const int NUMBOTS=30; //initially, and minimally

	const int REPORTS_PER_EPOCH = 10; // number of times to record data per epoch. 0 for off. (David Coleman)
	const int FRAMES_PER_EPOCH = 10000; //number of frames before epoch is incremented by 1.
	const int RECORD_SIZE = 200; // number of data points stored for the graph. Units is in reports, the frequency of which are defined above

	const int CONTINENTS= 6; //number of continents generated on the land layer
	const float OCEANPERCENT= 0.8; //decimal percent of land layer which will be ocean
	const float GRAVITYACCEL= 0.01; //how fast a bot will "fall" after jumping [0= weightless (I don't recommend), 0.5 or more= super-gravity]
	const float SPIKEMULT= 2; //strength of spike injury

	//BOTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BOTS
	const float BOTSPEED= 0.3; //fastest possible speed of agents
	const float BOOSTSIZEMULT=2; //how much boost do agents get? when boost neuron is on
	const float TOOCLOSE=5; //how much two agents can be overlapping before they take damage
	const float REACTPOWER= 0.1; //how strong is the restoring force between two colliding agents?
	const float MEANRADIUS=10; //average agent radius (only applies to random agents)
	const float SPIKELENGTH=30; //full spike length
	const float SPIKESPEED= 0.005; //how quickly can attack spike go up?
	const int BABIES=2; //number of babies per agent when they reproduce
	const float MINMOMHEALTH=0.65; //minimum amount of health required for an agent to have a child
	const float MAXMETABOLISM=5; //agent metabolism is limited to [0,this]
	const int MAXAGE=1000; //Age at which the full HEALTHLOSS_AGING amount is applied to an agent
	const float REPRATE=30; //amount of food required to be consumed for an agent to reproduce
	const float MAXDEVIATION=5; //maximum difference a crossover reproducing agent will be willing to tolerate

	const float DIST= 200; //how far can the senses can detect other bots or cells
	const float METAMUTRATE1= 0.001; //what is the change in MUTRATE1 and 2 on reproduction? lol
	const float METAMUTRATE2= 0.005;
	const float LEARNRATE= 0.02; //0.0005 (pre-alt) //how quickly a conn weight can change from use
	const float FOODTRANSFER= 0.01; //how much is transfered between two agents trading food? per iteration
	const float FOOD_SHARING_DISTANCE= 50; //how far away is food shared between bots?

	const float HEALTHLOSS_WHEELS = 0.0; //How much health is lost for a bot driving at full speed
	const float HEALTHLOSS_BOOSTMULT=3.8; //how much boost costs in health (set to 1 to nullify boost cost; its a multiplier)
	const float TEMPERATURE_DISCOMFORT = 0.003; //how quickly does health drain in nonpreferred temperatures (0= disabled. 0.003 is decent value)
	const float HEALTHLOSS_AGING = 0.0001;
	const float HEALTHLOSS_BRAINUSE= 0.0005; //0.001 //how much health is reduced for each box in the brain being active
	const float HEALTHLOSS_BUMP= 0.02; //how much health is lost upon collision
	const float HEALTHLOSS_SPIKE_EXT= 0; //how much health a bot looses for extending spike
	const float HEALTHLOSS_BADAIR= 0.01; // how much health is lost if in totally opposite environment

	//LAYERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LAYERS
	const float FOODINTAKE= 0.008; //how much plant food can feed an agent per tick?
	const float FOODGROWTH= -0.000001; //-0.000005 (too empty) //how much does food grow/decay on a cell which already has food?
	const float FOODWASTE= 0.0003; //0.0002 (too much) 0.0005 (no survivors) //how much food disapears if agent eats?
	const float FOODMAX= 0.5; //how much food per cell can there be at max?
	const int FOODADDFREQ= 120; //how often does random square get set to full food?
	const float FOODSPREAD= 0.00015; //probability of a food cell seeding food to a nearby cell when it has high plant life
	const int FOODRANGE= 3; //distance that single cell of food can seed. in cells.
	//Food currently has these stats:
	//0.5 full food / 0.0003 food eaten = 1667 ticks to consume full food
	//1667 ticks * 0.006 food intake = 10 real intake from 1 full food cell (for perfect herbivore)
	//10 real intake / 30 reprate = 1/3rd of reproduction (very roughly)
	//Plant food is the simplest and most plentiful form of nutrition, but it takes a long time to consume enough

	const float FRUITINTAKE = 0.015; //how much fruit can feed an agent per tick?
	const float FRUITDECAY = 0.0002; //0.001 (too fast)//how much fruit decays on a cell with low plant life?
	const float FRUITWASTE = 0.00075; //how much fruit disapears if agent eats?
	const float FRUITMAX = 0.5;
	const int FRUITADDFREQ = 20; //how often does a high-plant-food cell get set to full fruit?
	const float FRUITREQUIRE= 0.25;
	//Fruit currently has these stats:
	//0.5 full fruit / 0.00075 fruit eaten = 667 ticks to consume full fruit
	//667 ticks * 0.015 fruit intake = 10 real intake from 1 full fruit cell (for perfect frugivore)
	//10 real intake / 30 reprate = 1/3rd of reproduction (very roughly)
	//Fruit is a quick and easy alternative to plants. Also randomly populated, harkening back to ScriptBots origins

	const float MEATINTAKE= 0.04; //how much meat can feed an agent per tick?
	const float MEATDECAY= 0.000001; //0.000002 (not enough) //how much meat decays/grows on a cell? through MEATDECAY/[meat_present]
	const float MEATWASTE= 0.0015; // 0.0004 (lasts too long) 0.002 (not enough) //how much meat disapears if agent eats?
	const float MEATMAX= 0.5; //how much meat per cell can there be at max?
	const float MEATVALUE= 1; //how much meat a bot's body is worth? in range [0,1]
	//Meat currently has these stats:
	//0.5 full meat / 0.0015 meat eaten = 333 ticks to consume full meat
	//333 ticks * 0.04 meat intake = 13 real intake from 1 full meat cell (for perfect carnivore)
	//13 real intake / 30 reprate = 4/9ths of reproduction (very roughly)
	//Meat comes from dead bots, and is the fastest form of nutrition, IF bots can learn to find it...

	const int HAZARDFREQ= 20; //how often an instant hazard appears?
	const float HAZARDDECAY= 0.000002; //how much non-event hazard decays/grows on a cell per tick?
	const float HAZARDDEPOSIT= 0.001; //how much hazard is placed by a bot per tick?
	const float HAZARDDAMAGE= 0.006; // 0.001 (not enough loss) 0.003 (bit too much) //how much health a bot looses while on a filled hazard cell per tick?
	const float HAZARDMAX= 0.5; //how much hazard per cell can there be at max? more than 9/10 of this qualifies as an instant hazard
	}

#endif
