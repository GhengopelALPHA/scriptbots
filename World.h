#ifndef WORLD_H
#define WORLD_H

#include "View.h"
#include "Agent.h"
#include "settings.h"
//#include "ReadWrite.h"
#include <vector>
class World
{
public:
    World();
    ~World();
    
    void update();
    void reset();
	void spawn();
    
    void draw(View* view, int layer);
    
    bool isClosed() const;
    void setClosed(bool close);

	//debug stuff
	bool isDebug() const;
	void setDebug(bool state);
	std::vector<Vector2f> linesA;
	std::vector<Vector2f> linesB;

	//following and selected agent stuff
	void positionOfInterest(int type, float &xi, float &yi);

	int getSelection() const;

	int pinput1;
	float pleft;
	float pright;
	bool pcontrol;
	void setControl(bool state);

	void setSelection(int type);
	void setSelectedAgent(int id = -1);
	int getSelectedAgent() const;
	void selectedHeal();
	void selectedKill();
	void selectedBabys();
	void getFollowLocation(float &xi, float &yi);

	bool isAutoselecting() const;
	void setAutoselect(bool state);

    int epoch() const;
    
    //mouse interaction
    bool processMouse(int button, int state, int x, int y, float scale);

	void addAgents(int num, int set_stomach=-1, float nx=-1, float ny=-1, bool set_lungs=true);
    
	//graph stuff
    int numHerbivore[conf::RECORD_SIZE];
	int numCarnivore[conf::RECORD_SIZE];
	int numFrugivore[conf::RECORD_SIZE]; 
	int numHybrid[conf::RECORD_SIZE];
	int numTotal[conf::RECORD_SIZE];
    int ptr;

	//counters
	int modcounter;
    int current_epoch;
    int idcounter;

	//the agents!
	std::vector<Agent> agents;

	void setInputs();
	void brainsTick();  //takes in[] to out[] for every agent
    void processOutputs();

	void healthTick();

	std::vector<const char *> deaths; //record of all the causes of death this epoch

	//helper functions to give us counts of agents and cells
    int getHerbivores() const;
	int getCarnivores() const;
	int getFrugivores() const;
	int getLungLand() const;
	int getLungWater() const;
    int getAgents() const;
	int getHybrids() const;
	int getSpiked() const;
	int getFood() const;
	int getFruit() const;
	int getMeat() const;
	int getHazards() const;

	//cells; replaces food layer, can be expanded (4 layers currently)
	//[LAYER] represents current layer, see settings.h for ordering
	int CW;
	int CH;
	float cells[Layer::LAYERS][conf::WIDTH/conf::CZ][conf::HEIGHT/conf::CZ]; //[LAYER][CELL_X][CELL_Y]
    
private:
    void writeReport();
    
    void reproduce(int ai, int bi);

	void cellsRandomFill(int layer, float amount, int number);
	void cellsLandMasses();

    bool CLOSED; //if environment is closed, then no random bots or food are added per time interval
	bool DEBUG; //if debugging, collect additional data, print more feedback, and draw extra info
	bool AUTOSELECT; //if autoselecting, the agent which we are newly following gets selected
	int SELECTION; //id of selected agent
};

#endif // WORLD_H
