#ifndef GLVIEW_H
#define GLVIEW_H


#include "View.h"
#include "World.h"
#include "glui.h"

class GLView;

extern GLView* GLVIEW;

void gl_processNormalKeys(unsigned char key, int x, int y);
void gl_processSpecialKeys(int key, int x, int y);
void gl_processReleasedKeys(unsigned char key, int x, int y);
void gl_menu(int key);
void gl_processMouse(int button, int state, int x, int y);
void gl_processMouseActiveMotion(int x, int y);
void gl_processMousePassiveMotion(int x, int y);
void gl_changeSize(int w, int h);
void gl_handleIdle();
void gl_renderScene();
void glui_handleRW(int action);
void glui_handleCloses(int action);

class GLView : public View
{

public:
    GLView(World* w);
    virtual ~GLView();
    
    virtual void drawAgent(const Agent &a);
    virtual void drawCell(int x, int y, float quantity);
    virtual void drawData();
    
    void setWorld(World* w);
    
    //GLUT functions
    void processNormalKeys(unsigned char key, int x, int y);
	void processSpecialKeys(int key, int x, int y);
	void processReleasedKeys(unsigned char key, int x, int y);
	void processMouse(int button, int state, int x, int y);
    void processMouseActiveMotion(int x, int y);
	void processMousePassiveMotion(int x, int y);
	void menu(int key);
	void menuS(int key);
    void changeSize(int w, int h);
    void handleIdle();
    void renderScene();
	void handleRW(int action); //callback function for glui loading/saving
	void handleCloses(int action); //callback function for closing glui's

	void glCreateMenu();
	int m_id; //main context menu
	int win1;
	void gluiCreateMenu();
    
private:
    
    World *world;
	int live_worldclosed; //live variable support via glui
	int live_paused; //are we paused?
	int live_fastmode; //are we drawing?
	int live_skipdraw; //are we skipping some frames?
	int live_agentsvis; //are we drawing agents? If so, what's the scheme? 0= agents hidden, 1= normal, 2= stomach, 3= temp discomfort, 4= sound
	int live_layersvis; //what cell layer is currently active? 0= off, 1= plant food, 2= meat, 3= hazards, 4= fruit, 5= land
	int live_following;
	int live_debug; //are we debugging?
	bool debug;
	GLUI * Menu;
//	GLUI_FileBrowser * fb;
	GLUI * Loader;
	GLUI * Saver;
	GLUI * Alert;
	GLUI_EditText * Filename;
	char filename[30];

    char buf[100];
    char buf2[10];
    int modcounter; //tick counter
    int lastUpdate;
    int frames;
    
    float scalemult;
    float xtranslate, ytranslate;
    int downb[3];
    int mousex, mousey;
};

#endif // GLVIEW_H
