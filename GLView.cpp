#include "GLView.h"

#include <ctime>

#include "config.h"
#ifdef LOCAL_GLUT32
#include "glut.h"
#else
#include <GL/glut.h>
#endif

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>

void gl_processNormalKeys(unsigned char key, int x, int y)
{
	GLVIEW->processNormalKeys(key, x, y);
}
void gl_processSpecialKeys(int key, int x, int y)
{
	GLVIEW->processSpecialKeys(key, x, y);
}
void gl_processReleasedKeys(unsigned char key, int x, int y)
{
	GLVIEW->processReleasedKeys(key, x, y);
}
void gl_menu(int key)
{
	GLVIEW->menu(key);
}
void gl_changeSize(int w, int h)
{
	GLVIEW->changeSize(w,h);
}
void gl_handleIdle()
{
	GLVIEW->handleIdle();
}
void gl_processMouse(int button, int state, int x, int y)
{
	GLVIEW->processMouse(button, state, x, y);
}
void gl_processMouseActiveMotion(int x, int y)
{
	GLVIEW->processMouseActiveMotion(x,y);
}
void gl_processMousePassiveMotion(int x, int y)
{
	GLVIEW->processMousePassiveMotion(x,y);
}
void gl_renderScene()
{
	GLVIEW->renderScene();
}
void glui_handleRW(int action)
{
	GLVIEW->handleRW(action);
}
void glui_handleCloses(int action)
{
	GLVIEW->handleCloses(action);
}


void RenderString(float x, float y, void *font, const char* string, float r, float g, float b)
{
	glColor3f(r,g,b);
	glRasterPos2f(x, y);
	int len = (int) strlen(string);
	for (int i = 0; i < len; i++)
		glutBitmapCharacter(font, string[i]);
}

void drawCircle(float x, float y, float r) {
	float n;
	for (int k=0;k<17;k++) {
		n = k*(M_PI/8);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
}

GLView::GLView(World *s) :
		world(world),
		modcounter(0),
		frames(0),
		lastUpdate(0),
		mousedrag(false)
{

	xtranslate= 0.0;
	ytranslate= 0.0;
	scalemult= 0.2;
	downb[0]=0;downb[1]=0;downb[2]=0;
	mousex=0;mousey=0;

}

GLView::~GLView()
{

}
void GLView::changeSize(int w, int h)
{
	//Tell GLUT that were changing the projection, not the actual view
	glMatrixMode(GL_PROJECTION);
	//Reset the coordinate system
	glLoadIdentity();
	//resize viewport to new size
	glViewport(0, 0, w, h);
	//reconcile projection (required to keep everything that's visible, visible
	glOrtho(0,w,h,0,0,1);
	//revert to normal view opperations
	glMatrixMode(GL_MODELVIEW);
}

void GLView::processMouse(int button, int state, int x, int y)
{
	if(world->isDebug()) printf("MOUSE EVENT: button=%i state=%i x=%i y=%i, scale=%f, mousedrag=%i\n", button, state, x, y, scalemult, mousedrag);
	
	if(!mousedrag && state==1){ //dont let mouse click do anything if drag flag is raised
		//have world deal with it. First translate to world coordinates though
		int wx= (int) ((x-glutGet(GLUT_WINDOW_WIDTH)/2)/scalemult-xtranslate);
		int wy= (int) ((y-glutGet(GLUT_WINDOW_HEIGHT)/2)/scalemult-ytranslate);

		if(world->processMouse(button, state, wx, wy, scalemult)) live_selection = Select::MANUAL;
	}

	mousex=x; mousey=y;
	mousedrag= false;

	downb[button]=1-state; //state is backwards, ah well
}

void GLView::processMouseActiveMotion(int x, int y)
{
	if(world->isDebug()) printf("MOUSE MOTION x=%i y=%i, %i %i %i\n", x, y, downb[0], downb[1], downb[2]);
	if (downb[0]==1) {
		//left mouse button drag: pan around
		xtranslate += (x-mousex)/scalemult;
		ytranslate += (y-mousey)/scalemult;
		if (abs(x-mousex)>6 || abs(x-mousex)>6) live_follow= 0;
		//for releasing follow if the mouse is used to drag screen, but there's a threshold
	}  
	if (downb[1]==1) {
		//mouse wheel. Change scale
		scalemult -= conf::ZOOM_SPEED*(y-mousey);
		if(scalemult<0.01) scalemult=0.01;
	}
/*	if(downb[2]==1){ //disabled
		//right mouse button.
	}*/
	if(abs(mousex-x)>4 || abs(mousey-y)>4) mousedrag= true; //mouse was clearly dragged, don't select agents after
	mousex=x; mousey=y;
}

void GLView::processMousePassiveMotion(int x, int y)
{
	//for mouse scrolling. [DISABLED]
/*	if(y<=30) ytranslate += 2*(30-y);
	if(y>=glutGet(GLUT_WINDOW_HEIGHT)-30) ytranslate -= 2*(y-(glutGet(GLUT_WINDOW_HEIGHT)-30));
	if(x<=30) xtranslate += 2*(30-x);
	if(x>=glutGet(GLUT_WINDOW_WIDTH)-30) xtranslate -= 2*(x-(glutGet(GLUT_WINDOW_WIDTH)-30));*/
}

void GLView::processNormalKeys(unsigned char key, int x, int y)
{
	menu(key);	
}

void GLView::processSpecialKeys(int key, int x, int y)
{
	menuS(key);	
}

void GLView::processReleasedKeys(unsigned char key, int x, int y)
{
	if (key==32) {//spacebar input [released]
			world->pinput1= 0;
	}
}

void GLView::menu(int key)
{
	ReadWrite* savehelper= new ReadWrite(); //for loading/saving
	if (key == 27) //[esc] quit
		exit(0);
	else if (key==9) { //[tab] reset
		world->reset();
		world->spawn();
		printf("World reset\n");
	} else if (key=='p') {
		//pause
		live_paused= !live_paused;
	} else if (key=='m') { //drawing
		live_fastmode= !live_fastmode;
	} else if (key==43) { //+
		live_skipdraw++;
	} else if (key==45) { //-
		live_skipdraw--;
	} else if (key=='l' || key=='k') { //layer switch; l= "next", k= "previous"
		if (key=='l') live_layersvis++;
		else live_layersvis--;
		if (live_layersvis>Layer::LAYERS) live_layersvis= 0;
		if (live_layersvis<0) live_layersvis= Layer::LAYERS;
	} else if (key=='z' || key=='x') { //change agent visual scheme; x= "next", z= "previous"
		if (key=='x') live_agentsvis++;
		else live_agentsvis--;
		if (live_agentsvis>Visual::VISUALS-1) live_agentsvis= Visual::NONE;
		if (live_agentsvis<Visual::NONE) live_agentsvis= Visual::VISUALS-1; //5 here and above because there are 6 options (0,1,2,3,4,5)
	} else if (key==1002) { //add agents
		world->addAgents(5);
	} else if (key==1003) { //add Carnivore agents
		world->addAgents(5, Stomach::MEAT);
	} else if (key==1004) { //add Herbivore agents
		world->addAgents(5, Stomach::PLANT);
	} else if (key=='c') {
		world->setClosed( !world->isClosed() );
		live_worldclosed= (int) world->isClosed();
/*		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->isClosed()) glutChangeToMenuEntry(4, "Open World", 'c');
		else glutChangeToMenuEntry(4, "Close World", 'c');
		glutSetMenu(m_id);*/
	} else if (key=='f') {
		if(live_follow==0) live_follow= 1; //toggle follow selected agent
		else live_follow= 0;
	} else if(key=='o') {
		if(live_selection!=Select::OLDEST) live_selection= Select::OLDEST; //follow oldest agent
		else live_selection= Select::NONE;
	} else if(key=='q') {
		//zoom and translocate to instantly see the whole world
		float scaleA= (float)conf::WWIDTH/(conf::WIDTH+2100);
		float scaleB= (float)conf::WHEIGHT/(conf::HEIGHT+150);
		if(scaleA>scaleB) scalemult= scaleB;
		else scalemult= scaleA;
		xtranslate= -(conf::WIDTH-2020)/2;
		ytranslate= -(conf::HEIGHT-80)/2;
	} else if(key=='g') {
		if(live_selection!=Select::BEST_GEN) live_selection= Select::BEST_GEN; //follow most advanced generation agent
		else live_selection= Select::NONE;
	} else if(key=='h') {
		if(live_selection!=Select::HEALTHY) live_selection= Select::HEALTHY; //follow healthiest
		else live_selection= Select::NONE;
	}else if (key==127) { //delete
		world->selectedKill();
	}else if (key==62) { //zoom+ >
		scalemult += 10*conf::ZOOM_SPEED;
	}else if (key==60) { //zoom- <
		scalemult -= 10*conf::ZOOM_SPEED;
		if(scalemult<0.01) scalemult=0.01;
	}else if (key==32) { //spacebar input [pressed]
		world->pinput1= 1;
	}else if (key=='/') { // / (heal selected)
		world->selectedHeal();
	}else if (key=='|') { // | reproduce selected
		world->selectedBabys();
	}else if (key==119) { //w (move faster)
		world->pcontrol= true;
		world->pleft= capm(world->pleft + 0.08, -1, 1);
		world->pright= capm(world->pright + 0.08, -1, 1);
	}else if (key==97) { //a (turn left)
		world->pcontrol= true;
		world->pleft= capm(world->pleft - 0.05 + (world->pright-world->pleft)*0.05, -1, 1); //this extra code helps with turning out of tight circles
		world->pright= capm(world->pright + 0.05 + (world->pleft-world->pright)*0.05, -1, 1);
	}else if (key==115) { //s (move slower)
		world->pcontrol= true;
		world->pleft= capm(world->pleft - 0.08, -1, 1);
		world->pright= capm(world->pright - 0.08, -1, 1);
	}else if (key==100) { //d (turn right)
		world->pcontrol= true;
		world->pleft= capm(world->pleft + 0.05 + (world->pright-world->pleft)*0.05, -1, 1);
		world->pright= capm(world->pright - 0.05 + (world->pleft-world->pright)*0.05, -1, 1);
	} else if (key==999) { //player control
		world->setControl(!world->pcontrol);
		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->pcontrol) glutChangeToMenuEntry(5, "Release Agent", 999);
		else glutChangeToMenuEntry(5, "Control Agent", 999);
		glutSetMenu(m_id);
	}else if (key==1005) { //menu only, debug mode
		world->setDebug( !world->isDebug() );
		live_debug= (int) world->isDebug();
/*		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->isDebug()){
			glutChangeToMenuEntry(18, "Exit Debug Mode", 1005);
			printf("Entered Debug Mode\n");
		} else glutChangeToMenuEntry(18, "Enter Debug Mode", 1005);
		glutSetMenu(m_id);*/
	} else if (world->isDebug()) {
		printf("Unknown key pressed: %i\n", key);
	}
	//other keys: '1':49, '2':50, ..., '0':48
}

void GLView::menuS(int key) // movement control
{
	if (key == GLUT_KEY_UP) {
	   ytranslate += 20/scalemult;
	} else if (key == GLUT_KEY_LEFT) {
		xtranslate += 20/scalemult;
	} else if (key == GLUT_KEY_DOWN) {
		ytranslate -= 20/scalemult;
	} else if (key == GLUT_KEY_RIGHT) {
		xtranslate -= 20/scalemult;
	}
}

void GLView::glCreateMenu()
{
	m_id = glutCreateMenu(gl_menu); //right-click context menu
	glutAddMenuEntry("Control Selected (w,a,s,d)", 999); //line contains mode-specific text, see menu function above
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Spawn Agents", 1002);
	glutAddMenuEntry("Spawn Herbivores", 1004);
	glutAddMenuEntry("Spawn Carnivores", 1003);
	glutAddMenuEntry("Heal Agent (/)", '/');
	glutAddMenuEntry("Delete Agent (del)", 127);
	glutAddMenuEntry("Save World",1000);
	glutAddMenuEntry("Load World",1001);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Enter Debug Mode", 1005); //line contains mode-specific text, see menu function above
	glutAddMenuEntry("Reset Agents (tab)", 9);
	glutAddMenuEntry("Exit (esc)", 27);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void GLView::gluiCreateMenu()
{
	//GLUI menu. Slimy, yet satisfying.
	//must set our live vars to something. Might as well do it here
	live_worldclosed= 0;
	live_paused= 0;
	live_fastmode= 0;
	live_skipdraw= 1;
	live_agentsvis= Visual::RGB;
	live_layersvis= Layer::LAND+1;
	live_selection= Select::MANUAL;
	live_follow= 0;
	live_debug= 0;

	//create GLUI and add the options, be sure to connect them all to their real vals later
	Menu = GLUI_Master.create_glui("Menu",0,20,20);
	Menu->add_checkbox("Closed world",&live_worldclosed);
	Menu->add_checkbox("Pause",&live_paused);

	new GLUI_Button(Menu,"Load World",1, glui_handleRW);
	new GLUI_Button(Menu,"Save World",2, glui_handleRW);
	new GLUI_Button(Menu,"New World",3, glui_handleRW);

	GLUI_Panel *panel_speed= new GLUI_Panel(Menu,"Speed Control");
	Menu->add_checkbox_to_panel(panel_speed,"Fast Mode",&live_fastmode);
	Menu->add_spinner_to_panel(panel_speed,"Speed",GLUI_SPINNER_INT,&live_skipdraw);

	GLUI_Rollout *rollout_vis= new GLUI_Rollout(Menu,"Visuals");
	GLUI_RadioGroup *group_layers= new GLUI_RadioGroup(rollout_vis,&live_layersvis);
	new GLUI_StaticText(rollout_vis,"Layer");
	new GLUI_RadioButton(group_layers,"off");
	for(int i=0; i<Layer::LAYERS; i++){
		char text[16]= "";
		if(i==Layer::PLANTS) strcpy(text, "Plant");
		else if(i==Layer::MEATS) strcpy(text, "Meat");
		else if(i==Layer::HAZARDS) strcpy(text, "Hazard");
		else if(i==Layer::FRUITS) strcpy(text, "Fruit");
		else if(i==Layer::LAND) strcpy(text, "Map");

		new GLUI_RadioButton(group_layers,text);
	}
	Menu->add_column_to_panel(rollout_vis,true);
	GLUI_RadioGroup *group_agents= new GLUI_RadioGroup(rollout_vis,&live_agentsvis);
	new GLUI_StaticText(rollout_vis,"Agents");
	for(int i=0; i<Visual::VISUALS; i++){
		char text[16]= "";
		if(i==Visual::NONE) strcpy(text, "off");
		else if(i==Visual::RGB) strcpy(text, "RGB");
		else if(i==Visual::STOMACH) strcpy(text, "Stomach");
		else if(i==Visual::DISCOMFORT) strcpy(text, "Discomfort");
		else if(i==Visual::VOLUME) strcpy(text, "Volume");
		else if(i==Visual::SPECIES) strcpy(text, "Species ID");
		else if(i==Visual::CROSSABLE) strcpy(text, "Crossable");

		new GLUI_RadioButton(group_agents,text);
	}
	
	GLUI_Rollout *rollout_xyl= new GLUI_Rollout(Menu,"Selection");
	GLUI_RadioGroup *group_select= new GLUI_RadioGroup(rollout_xyl,&live_selection);
	for(int i=0; i<Select::SELECT_TYPES; i++){
		char text[16]= "";
		if(i==Select::OLDEST) strcpy(text, "Oldest");
		else if(i==Select::BEST_GEN) strcpy(text, "Best Gen.");
		else if(i==Select::HEALTHY) strcpy(text, "Healthiest");
		else if(i==Select::PRODUCTIVE) strcpy(text, "Productive");
		else if(i==Select::AGGRESSIVE) strcpy(text, "Aggressive");
		else if(i==Select::NONE) strcpy(text, "off");
		else if(i==Select::MANUAL) strcpy(text, "Manual");
		else if(i==Select::SOCIAL) strcpy(text, "Social");

		new GLUI_RadioButton(group_select,text);
	}
	Menu->add_checkbox_to_panel(rollout_xyl, "Follow Selected", &live_follow);
	Menu->add_button_to_panel(rollout_xyl, "Save Selected", 4, glui_handleRW);

	Menu->add_checkbox("DEBUG",&live_debug);

	//set to main graphics window
	Menu->set_main_gfx_window(win1);
}

void GLView::handleRW(int action) //glui callback for saving/loading worlds
{
	live_paused= 1;

	//action= 1: loading, action= 2: saving, action= 3: new (reset) world
	if (action==1){ //load option selected
		Loader = GLUI_Master.create_glui("Load World",0,50,50);

		Filename= new GLUI_EditText(Loader,"File Name (e.g, 'WORLD.SCB'):");
		Filename->set_w(300);
		new GLUI_Button(Loader,"Load",1, glui_handleCloses);

		Loader->set_main_gfx_window(win1);

	} else if (action==2){ //save option
		Saver = GLUI_Master.create_glui("Save World",0,50,50);

		Filename= new GLUI_EditText(Saver,"File Name (e.g, 'WORLD.SCB'):");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save",2, glui_handleCloses);

		Saver->set_main_gfx_window(win1);
	} else if (action==3){ //new world (old reset)
		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert,"Are you sure? This will");
		new GLUI_StaticText(Alert,"erase the current world."); 
		new GLUI_Button(Alert,"Okay",3, glui_handleCloses);
		new GLUI_Button(Alert,"Cancel",4, glui_handleCloses);

		Alert->set_main_gfx_window(win1);
	} else if (action==4){ //save selected agent
		Saver = GLUI_Master.create_glui("Save Agent",0,50,50);

		Filename= new GLUI_EditText(Saver,"File Name (e.g, 'AGENT.BOT'):");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save",6, glui_handleCloses);
	}
}

void GLView::handleCloses(int action) //GLUI callback for handling window closing
{
	live_paused= 0;

	ReadWrite* savehelper= new ReadWrite(); //for loading/saving

	if (action==1){ //loading
		strcpy(filename,Filename->get_text());
		if (debug) printf("File: '\\saves\\%s'\n",filename);
		if (!filename || filename==""){
			printf("ERROR: empty filename; returning to program.\n");

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert,"No file name given.");
			new GLUI_StaticText(Alert,"Returning to main program.");
			new GLUI_Button(Alert,"Okay");

		} else {
			savehelper->loadWorld(world, xtranslate, ytranslate, filename);
		}
		Loader->hide();

	} else if (action==2){ //saving
		const char *tempname= Filename->get_text();
		strcpy(filename, tempname);
		if (debug) printf("File: '\\saves\\%s'",filename);
		if (!filename || filename==""){
			printf("ERROR: empty filename; returning to program.\n");

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert, "No file name given." );
			new GLUI_StaticText(Alert, "Returning to main program." );
			new GLUI_Button(Alert,"Okay");

		} else {
			//check the filename given to see if it exists yet
			char address[32];
			strcpy(address,"saves\\");
			strcat(address,tempname);

			FILE* ck = fopen(address, "r");
			if(ck){
				if (debug) printf("WARNING: %s already exists!\n", filename);

				Alert = GLUI_Master.create_glui("Alert",0,50,50);
				Alert->show();
				new GLUI_StaticText(Alert, "The file name given already exists." );
				new GLUI_StaticText(Alert, "Would you like to overwrite?" );
				new GLUI_Button(Alert,"Okay",5, glui_handleCloses);
				new GLUI_Button(Alert,"Cancel",4, glui_handleCloses);
				live_paused= 1;
				fclose(ck);
			} else {
				fclose(ck);
				savehelper->saveWorld(world, xtranslate, ytranslate, filename);
			}
		}
		Saver->hide();

	} else if (action==3){ //resetting
		world->reset();
		world->spawn();
		printf("World reset\n");
		Alert->hide();
	} else if (action==4){ //Alert cancel/continue
		Alert->hide();
	} else if (action==5){ //Alert from above saving
		savehelper->saveWorld(world, xtranslate, ytranslate, filename);
		Alert->hide();
	} else if (action==6){ //saving agents
		const char *tempname= Filename->get_text();
		strcpy(filename, tempname);
		if (debug) printf("File: '\\saved_agents\\%s'",filename);
		if (!filename || filename==""){
			printf("ERROR: empty filename; returning to program.\n");

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert, "No file name given." );
			new GLUI_StaticText(Alert, "Returning to main program." );
			new GLUI_Button(Alert,"Okay");

		} else {
			//check the filename given to see if it exists yet
			char address[32];
			strcpy(address,"saved_agents\\");
			strcat(address,tempname);

//			FILE* ck = fopen(address, "r");
//			if(ck){
//				if (debug) printf("WARNING: %s already exists!\n", filename);
//
//				Alert = GLUI_Master.create_glui("Alert",0,50,50);
//				Alert->show();
//				new GLUI_StaticText(Alert, "The file name given already exists." );
//				new GLUI_StaticText(Alert, "Would you like to overwrite?" );
//				new GLUI_Button(Alert,"Okay",7, glui_handleCloses);
//				new GLUI_Button(Alert,"Cancel",4, glui_handleCloses);
//				live_paused= 1;
//				fclose(ck);
//			} else {
//				fclose(ck);
				FILE* sa = fopen(address, "w");
				int sidx= world->getSelectedAgent();
				if (sidx>=0){
					Agent *a= &world->agents[sidx];
					savehelper->saveAgent(a, sa);
				}
				fclose(sa);
//			}
		}
		Saver->hide();
//	} else if (action==7){ //overwrite agent save
//		FILE* sa = fopen(address, "w");
//		int sidx= world->getSelectedAgent();
//		if (sidx>=0){
//			savehelper->saveAgent(world->agents[sidx], sa);
//		}
//		fclose(sa);
	}

}

void GLView::handleIdle()
{
	//set proper window (we don't want to draw on nothing, now do we?!)
	if (glutGetWindow() != win1) glutSetWindow(win1); 

	GLUI_Master.sync_live_all();

	//after syncing all the live vars with GLUI_Master, set the vars they represent to their proper values.
	world->setClosed(live_worldclosed);
	world->setDebug((bool) live_debug);

	modcounter++;
	if (!live_paused) world->update();

	//show FPS and other stuff
	int currentTime = glutGet( GLUT_ELAPSED_TIME );
	frames++;
	if ((currentTime - lastUpdate) >= 1000) {
		sprintf( buf, "FPS: %d speed: %d Total Agents: %d Herbivores: %d Carnivores: %d Frugivores: %d Epoch: %d",
			frames, live_skipdraw, world->getAgents(), world->getHerbivores(), world->getCarnivores(), world->getFrugivores(), world->epoch() );
		glutSetWindowTitle( buf );
		frames = 0;
		lastUpdate = currentTime;
	}

	if (!live_fastmode) {
		if (live_skipdraw>0) {
			if (modcounter%live_skipdraw==0) renderScene();	//increase fps by skipping drawing
		}
		else { //we will decrease fps by waiting using clocks
			clock_t endwait;
			float mult=-0.005*(live_skipdraw-1); //ugly, ah well
			endwait = clock () + mult * CLOCKS_PER_SEC ;
			while (clock() < endwait) {}
			renderScene();
		}
	}
}

void GLView::renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();

	glTranslatef(glutGet(GLUT_WINDOW_WIDTH)/2, glutGet(GLUT_WINDOW_HEIGHT)/2, 0.0f);	
	glScalef(scalemult, scalemult, 1.0f);

	//handle world agent selection interface
	world->setSelection(live_selection);
	if (world->getSelection()==-1 && live_selection!=Select::MANUAL && modcounter%5==0) live_selection= Select::NONE;

	if(live_follow==1){ //if we're following,
		float xi=0, yi=0;
		world->getFollowLocation(xi, yi); //get the follow location from the world (location of selected agent)

		if(xi!=0 && yi!=0){
			// if we got to move the screen completly accross the world, jump instead
			if(abs(-xi-xtranslate)>0.95*conf::WIDTH || abs(-yi-ytranslate)>0.95*conf::HEIGHT){
				xtranslate= -xi; ytranslate= -yi;
			} else {
				float speed= conf::SNAP_SPEED;
				if(scalemult>0.5) speed= cap(speed*pow(scalemult + 0.5,3));
				xtranslate+= speed*(-xi-xtranslate); ytranslate+= speed*(-yi-ytranslate);
			}
		}
	}

	glTranslatef(xtranslate, ytranslate, 0.0f);
	
	world->draw(this, live_layersvis);

	glPopMatrix();
	glutSwapBuffers();
}

void GLView::drawAgent(const Agent& agent, float x, float y, bool ghost)
{
	float n;
	float r= agent.radius;
	float rp= agent.radius+2.5;

	if (live_agentsvis!=Visual::NONE) {

		//color assignment
		float red= 0,gre= 0,blu= 0;
		float discomfort= 0;

		//first, calculate colors which also have indicator boxes
		float dd= 2.0*abs(agent.pos.y/conf::HEIGHT - 0.5);
		discomfort= cap(sqrt(abs(dd-agent.temperature_preference)));
		if (discomfort<0.08) discomfort=0;

		float stomach_red= cap(pow(agent.stomach[Stomach::MEAT],2)+pow(agent.stomach[Stomach::FRUIT],2)-pow(agent.stomach[Stomach::PLANT],2));
		float stomach_gre= cap(pow(agent.stomach[Stomach::PLANT],2)+pow(agent.stomach[Stomach::FRUIT],2)-pow(agent.stomach[Stomach::MEAT],2));

		//now colorize agents and other things
		if (live_agentsvis==Visual::RGB){ //real rgb values
			red= agent.red; gre= agent.gre; blu= agent.blu;

		} else if (live_agentsvis==Visual::STOMACH){ //stomach
			red= stomach_red;
			gre= stomach_gre;
			blu= 0;
		
		} else if (live_agentsvis==Visual::DISCOMFORT){ //temp discomfort
			red= discomfort; gre= (2-discomfort)/2; blu= 1-discomfort;

		} else if (live_agentsvis==Visual::VOLUME) { //volume
			red= agent.volume;
			gre= agent.volume;
			blu= agent.volume;

		} else if (live_agentsvis==Visual::SPECIES){ //species 
			red= (cos((float)agent.species/330*M_PI)+1.0)/2.0;
			gre= (sin((float)agent.species/100*M_PI)+1.0)/2.0;
			blu= (cos((float)agent.species/43*M_PI)+1.0)/2.0;
		
		} else if (live_agentsvis==Visual::CROSSABLE){ //crossover-compatable to selection
			//all other agents look grey if unrelated or if none is selected, b/c then we don't have a reference
			red= 0.8;
			gre= 0.8;
			blu= 0.8;

			if(world->getSelectedAgent()>=0){
				float deviation= abs(agent.species - world->agents[world->getSelectedAgent()].species); //species deviation check
				if (deviation==0) {
					red= 0.2;
				} else if (deviation<=conf::MAXDEVIATION) {
					red= 0;
					gre= 0;
				} else if (deviation<=2*conf::MAXDEVIATION) {
					red= 0.6;
					gre= 0.4;
				}
			}
		}


		//handle selected agent
		if (agent.id==world->getSelection() && !ghost) {
			//draw selection
			glBegin(GL_POLYGON);
			glColor3f(1,1,1);
			drawCircle(x, y, agent.radius+5);
			glEnd();

			glPushMatrix();
			glTranslatef(x-80,y+20,0);
			//draw inputs, outputs
			float col;
			float yy=15;
			float xx=15;
			float ss=16;
			glBegin(GL_QUADS);
			for (int j=0;j<Input::INPUT_SIZE;j++) {
				col= agent.in[j];
				glColor3f(col,col,col);
				glVertex3f(0+ss*j, 0, 0.0f);
				glVertex3f(xx+ss*j, 0, 0.0f);
				glVertex3f(xx+ss*j, yy, 0.0f);
				glVertex3f(0+ss*j, yy, 0.0f);
			}
			yy+=5;
			for (int j=0;j<Output::OUTPUT_SIZE;j++) {
				col= agent.out[j];
				glColor3f(col,col,col);
				glVertex3f(0+ss*j, yy, 0.0f);
				glVertex3f(xx+ss*j, yy, 0.0f);
				glVertex3f(xx+ss*j, yy+ss, 0.0f);
				glVertex3f(0+ss*j, yy+ss, 0.0f);
			}
			yy+=ss*2;

			//draw brain.
			
			float offx=0;
			ss=8;
			xx=ss;
			for (int j=0;j<BRAINSIZE;j++) {
				col = agent.brain.boxes[j].out;
				glColor3f(col,col,col);
				
				glVertex3f(offx+0+ss*j, yy, 0.0f);
				glVertex3f(offx+xx+ss*j, yy, 0.0f);
				glVertex3f(offx+xx+ss*j, yy+ss, 0.0f);
				glVertex3f(offx+ss*j, yy+ss, 0.0f);
				
				if ((j+1)%30==0) {
					yy+=ss;
					offx-=ss*30;
				}
			}
			/*
			//draw lines connecting connected brain boxes, but only in debug mode (NEEDS SIMPLIFICATION)
			if(world->isDebug()){
				glEnd();
				glBegin(GL_LINES);
				float offx=0;
				ss=30;
				xx=ss;
				for (int j=0;j<BRAINSIZE;j++) {
					for(int k=0;k<CONNS;k++){
						int j2= agent.brain.boxes[j].id[k];
						
						//project indices j and j2 into pixel space
						float x1= 0;
						float y1= 0;
						if(j<Input::INPUT_SIZE) { x1= j*ss; y1= yy; }
						else { 
							x1= ((j-Input::INPUT_SIZE)%30)*ss;
							y1= yy+ss+2*ss*((int) (j-Input::INPUT_SIZE)/30);
						}
						
						float x2= 0;
						float y2= 0;
						if(j2<Input::INPUT_SIZE) { x2= j2*ss; y2= yy; }
						else { 
							x2= ((j2-Input::INPUT_SIZE)%30)*ss;
							y2= yy+ss+2*ss*((int) (j2-Input::INPUT_SIZE)/30);
						}
						
						float ww= agent.brain.boxes[j].w[k];
						if(ww<0) glColor3f(-ww, 0, 0);
						else glColor3f(0,0,ww);
						
						glVertex3f(x1,y1,0);
						glVertex3f(x2,y2,0);
					}
				}
			}*/
			

			glEnd();
			glPopMatrix();
		}

		//draw indicator of this agent... used for various events
		if (agent.indicator>0) {
			glBegin(GL_POLYGON);
			glColor4f(agent.ir,agent.ig,agent.ib,0.75);
			drawCircle(x, y, r+((int)agent.indicator));
			glEnd();
		}

		//draw giving/receiving
		if(agent.dfood!=0){
			glBegin(GL_POLYGON);
			float mag=cap(abs(agent.dfood)/conf::FOODTRANSFER/3);
			if(agent.dfood>0) glColor3f(0,mag,0);
			else glColor3f(mag,0,0); //draw sharing as a thick green or red outline
			for (int k=0;k<17;k++){
				n = k*(M_PI/8);
				glVertex3f(x+rp*sin(n),y+rp*cos(n),0);
				n = (k+1)*(M_PI/8);
				glVertex3f(x+rp*sin(n),y+rp*cos(n),0);
			}
			glEnd();
		}
		
		if (scalemult > .1 || ghost) { //dont render eyes, ears, or boost if zoomed too far out, but always render them on ghosts
			//draw eyes
			for(int q=0;q<NUMEYES;q++) {
				glBegin(GL_LINES);
				glColor4f(0.5,0.5,0.5,0.75);
				glVertex3f(x,y,0);
				float aa= agent.angle+agent.eyedir[q];
				glVertex3f(x+(r+30)*cos(aa), y+(r+30)*sin(aa), 0);
				glEnd();
			}

			//ears
			for(int q=0;q<NUMEARS;q++) {
				glBegin(GL_POLYGON);
				glColor4f(0.6,0.6,0,0.5);
				float aa= agent.angle + agent.eardir[q];
				drawCircle(x+r*cos(aa), y+r*sin(aa), 2.0);
				glEnd();
			}

			//boost blur
			if (agent.boost){
				for(int q=1;q<4;q++){
					Vector2f displace(r/4*q*(agent.w1+agent.w2), 0);
					displace.rotate(agent.angle+M_PI);

					glBegin(GL_POLYGON); 
					glColor4f(red,gre,blu,0.25);
					drawCircle(x+displace.x, y+displace.y, r);
					glEnd();
				}
			}

			//vis grabbing
			if((scalemult > .3 || ghost) && agent.grabbing>0.5){
				glLineWidth(2);
				glBegin(GL_LINES);
				
				glColor4f(0.0,0.7,0.7,0.75);
				glVertex3f(x,y,0);
				float mult= agent.grabID==-1 ? 1 : 0;
				float aa= agent.angle+M_PI/8*mult;
				float ab= agent.angle-M_PI/8*mult;
				glVertex3f(x+(conf::GRABBING_DISTANCE+r)*cos(aa), y+(conf::GRABBING_DISTANCE+r)*sin(aa), 0);
				glVertex3f(x,y,0);
				glVertex3f(x+(conf::GRABBING_DISTANCE+r)*cos(ab), y+(conf::GRABBING_DISTANCE+r)*sin(ab), 0);
				glEnd();
				glLineWidth(1);
			}

		}
		
		glBegin(GL_POLYGON); 
		//body
		glColor3f(red,gre,blu);
		drawCircle(x, y, r);
		glEnd();

		glBegin(GL_LINES);

		//outline and spikes are effected by the zoom magnitude
		float blur= cap(4.5*scalemult-0.5);
		if (ghost) blur= 1; //disable effect for static rendering

		//jaws
		if (scalemult > .08 || ghost) { //dont render jaws if zoomed too far out, but always render them on ghosts
			glColor4f(0.9,0.9,0,blur);
			float mult= 1-powf(abs(agent.jawPosition),0.5);
			glVertex3f(x+r*cos(agent.angle),y+r*sin(agent.angle),0);
			glVertex3f(x+(10+r)*cos(agent.angle+M_PI/8*mult), y+(10+r)*sin(agent.angle+M_PI/8*mult), 0);
			glVertex3f(x+r*cos(agent.angle),y+r*sin(agent.angle),0);
			glVertex3f(x+(10+r)*cos(agent.angle-M_PI/8*mult), y+(10+r)*sin(agent.angle-M_PI/8*mult), 0);
		}

		//outline
		float out_red= 0,out_gre= 0,out_blu= 0;
		if (agent.jump>0) { //draw jumping as yellow outline
			out_red= 0.8;
			out_gre= 0.8;
		}
		
		glColor3f(cap(out_red*blur + (1-blur)*red), cap(out_gre*blur + (1-blur)*gre), cap(out_blu*blur + (1-blur)*blu));

		for (int k=0;k<17;k++)
		{
			n = k*(M_PI/8);
			glVertex3f(x+r*sin(n),y+r*cos(n),0);
			n = (k+1)*(M_PI/8);
			glVertex3f(x+r*sin(n),y+r*cos(n),0);
		}

		//sound waves!
		if(live_agentsvis==Visual::VOLUME && !ghost && agent.volume>0){
			float volume= agent.volume;
			float count= volume*10;
			//LATER: will bring frequency in here, for now, count is proportional to volume
			for (int l=0; l<=(int)count; l++){
				float dist= conf::DIST*(l/count)+4*(world->modcounter%(int)((conf::DIST)/4));
				if (dist>conf::DIST) dist-= conf::DIST;
				glColor4f(volume, volume, volume, cap((1-dist/conf::DIST)*volume));

				for (int k=0;k<32;k++)
				{
					n = k*(M_PI/16);
					glVertex3f(x+dist*sin(n),y+dist*cos(n),0);
					n = (k+1)*(M_PI/16);
					glVertex3f(x+dist*sin(n),y+dist*cos(n),0);
				}
			}
		}

		//and spike
		if (scalemult > .08 || ghost) { //dont render spike if zoomed too far out, but always render it on ghosts
			glColor4f(0.7,0,0,blur);
			glVertex3f(x,y,0);
			glVertex3f(x+(conf::SPIKELENGTH*agent.spikeLength)*cos(agent.angle),
					   y+(conf::SPIKELENGTH*agent.spikeLength)*sin(agent.angle),
					   0);
		}
		glEnd();

		if(!ghost){ //only draw extra infos if not a ghost

			if(scalemult > .3) {//hide extra visual data if really far away

				//debug sight lines
				if(world->isDebug()) {
					for (int i=0;i<(int)world->linesA.size();i++) {
						glBegin(GL_LINES);
						glColor3f(1,1,1);
						glVertex3f(world->linesA[i].x,world->linesA[i].y,0);
						glVertex3f(world->linesB[i].x,world->linesB[i].y,0);
						glEnd();
					}
					world->linesA.resize(0);
					world->linesB.resize(0);
				}

				//health
				int xo=18;
				int yo=-15;
				glBegin(GL_QUADS);
				glColor3f(0,0,0);
				glVertex3f(x+xo,y+yo,0);
				glVertex3f(x+xo+5,y+yo,0);
				glVertex3f(x+xo+5,y+yo+40,0);
				glVertex3f(x+xo,y+yo+40,0);

				glColor3f(0,0.8,0);
				glVertex3f(x+xo,y+yo+20*(2-agent.health),0);
				glVertex3f(x+xo+5,y+yo+20*(2-agent.health),0);
				glVertex3f(x+xo+5,y+yo+40,0);
				glVertex3f(x+xo,y+yo+40,0);

				//hybrid marker
				if (agent.hybrid) {
					glColor3f(0,0,0.8);
					glVertex3f(x+xo+6,y+yo,0);
					glVertex3f(x+xo+12,y+yo,0);
					glVertex3f(x+xo+12,y+yo+10,0);
					glVertex3f(x+xo+6,y+yo+10,0);
				}

				//stomach type indicator
				glColor3f(stomach_red,stomach_gre,0);
				glVertex3f(x+xo+6,y+yo+12,0);
				glVertex3f(x+xo+12,y+yo+12,0);
				glVertex3f(x+xo+12,y+yo+22,0);
				glVertex3f(x+xo+6,y+yo+22,0);

				//sound volume indicator
				glColor3f(agent.volume,agent.volume,agent.volume);
				glVertex3f(x+xo+6,y+yo+24,0);
				glVertex3f(x+xo+12,y+yo+24,0);
				glVertex3f(x+xo+12,y+yo+34,0);
				glVertex3f(x+xo+6,y+yo+34,0);

				//temp discomfort indicator
				glColor3f(discomfort,(2-discomfort)/2,(1-discomfort));
				glVertex3f(x+xo+6,y+yo+36,0);
				glVertex3f(x+xo+12,y+yo+36,0);
				glVertex3f(x+xo+12,y+yo+46,0);
				glVertex3f(x+xo+6,y+yo+46,0);

				//land/water lungs requirement indicator
				glColor3f(0.2,0.4*agent.lungs+0.3,0.6*(1-agent.lungs)+0.2); //land (0.2,0.7,0.2), amph (0.2,0.5,0.5), water (0.2,0.3,0.8)
				glVertex3f(x+xo+14,y+yo,0);
				glVertex3f(x+xo+20,y+yo,0);
				glVertex3f(x+xo+20,y+yo+10,0);
				glVertex3f(x+xo+14,y+yo+10,0);
			}

			glEnd();

			//print stats
			if(scalemult > .7) { // hide the number stats when zoomed out
				//generation count
				sprintf(buf2, "%i", agent.gencount);
				RenderString(x-rp*1.414, y+rp*1.414, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0f, 1.0f);

				//age
				sprintf(buf2, "%i", agent.age);
				float red = cap((float) agent.age/conf::MAXAGE); //will be redder the closer it is to MAXAGE
				RenderString(x-rp*1.414, y+rp*1.414+12, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0-red, 1.0-red);

				//health
				sprintf(buf2, "%.2f", agent.health);
				RenderString(x-rp*1.414, y+rp*1.414+24, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0f, 1.0f);

				//repcounter
				float dr = agent.metabolism/conf::MAXMETABOLISM; //red if high metabolism, blue if low 
				sprintf(buf2, "%.2f", agent.repcounter);
				RenderString(x-rp*1.414, y+rp*1.414+36, GLUT_BITMAP_HELVETICA_12, buf2, dr/2+0.5, dr/2+0.5, (1.0-dr)/2+0.5);
			}
		}
	}
}


void GLView::drawData()
{
	float mm = 3;
	//draw misc info
	glBegin(GL_LINES);
	glColor3f(0,0,0); //border around graphs and feedback

	glVertex3f(conf::WIDTH,0,0);
	glVertex3f(-2020,0,0);

	glVertex3f(-2020,0,0);
	glVertex3f(-2020,conf::HEIGHT,0);

	glVertex3f(0,0,0);
	glVertex3f(0,conf::HEIGHT,0);

	glVertex3f(-2020,conf::HEIGHT,0);
	glVertex3f(conf::WIDTH,conf::HEIGHT,0);

	glVertex3f(conf::WIDTH,conf::HEIGHT,0);
	glVertex3f(conf::WIDTH,0,0);

	glColor3f(0,0,0.8); //hybrid count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10, conf::HEIGHT - mm*world->numHybrid[q],0);
		glVertex3f(-2020 +(q+1)*10, conf::HEIGHT - mm*world->numHybrid[q+1],0);
	}
	glColor3f(0,1,0); //herbivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numHerbivore[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numHerbivore[q+1],0);
	}
	glColor3f(1,0,0); //carnivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numCarnivore[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numCarnivore[q+1],0);
	}
	glColor3f(0.7,0.7,0); //frugivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numFrugivore[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numFrugivore[q+1],0);
	}
	glColor3f(0,0,0); //total count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numTotal[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numTotal[q+1],0);
	}
	glVertex3f(-2020 + world->ptr*10,conf::HEIGHT,0);
	glVertex3f(-2020 + world->ptr*10,conf::HEIGHT -mm*world->getAgents(),0);
	glEnd();
	sprintf(buf2, "%i agents", world->getAgents());
	RenderString(-2016 + world->ptr*10,conf::HEIGHT -mm*world->getAgents(), GLUT_BITMAP_HELVETICA_12, buf2, 0.0f, 0.0f, 0.0f);
	
	RenderString(-2020, -80, GLUT_BITMAP_HELVETICA_12, "Useful Keybindings: 'm' disables drawing, 'p' pauses the sim, 'l' & 'k' switch layer view, and 'z' & 'x' switch agent view. 'f' follows the selected bot.", 0.0f, 0.0f, 0.0f);
	RenderString(-2020, -20, GLUT_BITMAP_HELVETICA_12, "Try clicking on a bot. Use 'w,a,s,d' to control it. '/' heals it, 'delete' kills it. 'spacebar' triggers a special input.", 0.0f, 0.0f, 0.0f);
}

void GLView::drawStatic()
{
	/*start setup*/
	glMatrixMode(GL_PROJECTION);
	// save previous matrix which contains the
	//settings for the perspective projection
	glPushMatrix();
	glLoadIdentity();

	// set a 2D orthographic projection
	gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));
	// invert the y axis, down is positive
	glScalef(1, -1, 1);
	// move the origin from the bottom left corner
	// to the upper left corner
	glTranslatef(0, -glutGet(GLUT_WINDOW_HEIGHT), 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	/*end setup*/

	//begin things that we actually want to draw staticly
	if(live_paused) RenderString(10, 20, GLUT_BITMAP_HELVETICA_12, "Paused", 0.5f, 0.5f, 0.5f);
	if(live_follow!=0) {
		if(world->getSelectedAgent()>=0) RenderString(10, 40, GLUT_BITMAP_HELVETICA_12, "Following", 0.5f, 0.5f, 0.5f);
		else RenderString(10, 40, GLUT_BITMAP_HELVETICA_12, "No Follow Target", 1.0f, 0.5f, 0.5f);
	}
	if(world->isClosed()) RenderString(10, 60, GLUT_BITMAP_HELVETICA_12, "Closed World", 0.5f, 0.5f, 0.5f);
	if(world->isDebug()) {
		sprintf(buf, "75%% Plant: %i", world->getFood());
		RenderString(5, 80, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "50%% Fruit: %i", world->getFruit());
		RenderString(5, 100, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "50%% Meat: %i", world->getMeat());
		RenderString(5, 120, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "50%% Hazard: %i", world->getHazards());
		RenderString(5, 140, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "modcounter: %i", world->modcounter);
		RenderString(5, 160, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "GL modcounter: %i", modcounter);
		RenderString(5, 180, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
	}

	//center axis markers
	glBegin(GL_LINES);
	glColor4f(0,1,0,0.3); //green y-axis
	glVertex3f(glutGet(GLUT_WINDOW_WIDTH)/2,glutGet(GLUT_WINDOW_HEIGHT)/2,0);
	glVertex3f(glutGet(GLUT_WINDOW_WIDTH)/2,glutGet(GLUT_WINDOW_HEIGHT)/2+15,0);

	glColor4f(1,0,0,0.3); //red x-axis
	glVertex3f(glutGet(GLUT_WINDOW_WIDTH)/2,glutGet(GLUT_WINDOW_HEIGHT)/2,0);
	glVertex3f(glutGet(GLUT_WINDOW_WIDTH)/2+15,glutGet(GLUT_WINDOW_HEIGHT)/2,0);
	glEnd();

	//selected agent overlay
	if(world->getSelectedAgent()>=0){
		//get agent
		Agent selected= world->agents[world->getSelectedAgent()];
		//slightly transparent background
		glBegin(GL_QUADS);
		glColor4f(0,0.4,0.5,0.6);
		glVertex3f(glutGet(GLUT_WINDOW_WIDTH)-10,10,0);
		glVertex3f(glutGet(GLUT_WINDOW_WIDTH)-10,150,0);
		glVertex3f(glutGet(GLUT_WINDOW_WIDTH)-400,150,0);
		glVertex3f(glutGet(GLUT_WINDOW_WIDTH)-400,10,0);
		glEnd();

		//draw Ghost Agent
		drawAgent(selected, glutGet(GLUT_WINDOW_WIDTH)-350, 80, true);

		//write text and values
		//Agent ID
		sprintf(buf, "ID: %d", selected.id);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-380,
					 25, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Health
		sprintf(buf, "Health: %.2f/2", selected.health);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 25, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Repcounter
		sprintf(buf, "Child: %.2f/%.0f", selected.repcounter, conf::REPRATE);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 25, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stomach
		sprintf(buf, "H%.1f F%.1f C%.1f", selected.stomach[Stomach::PLANT], selected.stomach[Stomach::FRUIT], 
				selected.stomach[Stomach::MEAT]);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 25, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		if(selected.isHerbivore()) sprintf(buf, "\"Herbivore\"");
		else if(selected.isFrugivore()) sprintf(buf, "\"Frugivore\"");
		else if(selected.isCarnivore()) sprintf(buf, "\"Carnivore\"");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 40, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//age
		sprintf(buf, "Age: %d", selected.age);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 40, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Generation
		sprintf(buf, "Gen: %d", selected.gencount);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 40, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Temperature Preference
		if(selected.temperature_preference<0.3) sprintf(buf, "Heat-loving(%.3f)", selected.temperature_preference);
		else if (selected.temperature_preference>0.7) sprintf(buf, "Cold-loving(%.3f)", selected.temperature_preference);
		else sprintf(buf, "Temperate(%.3f)", selected.temperature_preference);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 55, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Lung-type
		if(selected.lungs<0.3) sprintf(buf, "Aquatic(%.3f)", selected.lungs);
		else if (selected.lungs>0.7) sprintf(buf, "Terrestrial(%.3f)", selected.lungs);
		else sprintf(buf, "Amphibious(%.3f)", selected.lungs);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 55, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Radius
		sprintf(buf, "Radius: %.2f", selected.radius);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 55, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);
					 

		//Mutrates
		sprintf(buf, "Mutrate1: %.3f", selected.MUTRATE1);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 70, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		sprintf(buf, "Mutrate2: %.3f", selected.MUTRATE2);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 70, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Metabolism
		sprintf(buf, "Metab: %.2f", selected.metabolism);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 70, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Hybrid
		if(selected.hybrid) sprintf(buf, "Hybrid");
		else sprintf(buf, "Budded");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 85, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Giving
		if(selected.out[Output::GIVE]>0.5) sprintf(buf, "Generous");
		else sprintf(buf, "Selfish");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 85, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Spiked
		if(selected.spikeLength*conf::SPIKELENGTH>=selected.radius){
			float val= conf::SPIKEMULT*selected.spikeLength;
			sprintf(buf, "Spikey(DMG%.2f)", val);
		} else sprintf(buf, "Not Spikey");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 85, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Children
		sprintf(buf, "Children: %d", selected.children);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 100, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Stimulated
		if(selected.spikeLength*conf::SPIKELENGTH>=0.5) sprintf(buf, "Stim?");
		else sprintf(buf, "No Stim?");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 100, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Killed
		sprintf(buf, "Killed: %d", selected.killed);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 100, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Trait: Num Babies
		sprintf(buf, "Num Babies: %d", selected.numbabies);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 115, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Output: Sexual projection
		if(selected.sexproject) sprintf(buf, "Sexting");
		else sprintf(buf, "Not Sexting");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 115, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Species ID (Genome)
		sprintf(buf, "Genome: %d", selected.species);
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 115, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Jumping status
		if(selected.jump<=0) sprintf(buf, "Grounded");
		else sprintf(buf, "Airborne!");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-300,
					 130, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Grab status
		if(selected.grabbing>0.5){
			if(selected.grabID==-1) sprintf(buf, "Seeking");
			else sprintf(buf, "Hold: %d", selected.grabID);
		} else sprintf(buf, "Isolated");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-200,
					 130, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Biting
		if(selected.jawPosition!=0) sprintf(buf, "Biting %.2f", selected.jawPosition);
		else sprintf(buf, "Not Biting");
		RenderString(glutGet(GLUT_WINDOW_WIDTH)-100,
					 130, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

	}

	/*start clean up*/
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	// restore previous projection matrix
	glPopMatrix();

	// get back to modelview mode
	glMatrixMode(GL_MODELVIEW);
	/*end clean up*/
}


void GLView::drawCell(int x, int y, float quantity)
{
	if (live_layersvis!=0) { //0: white
		glBegin(GL_QUADS);
		if (live_layersvis==Layer::PLANTS+1) { //plant food: green w/ navy blue background
			glColor4f(0.0,quantity,0.1,1);
		} else if (live_layersvis==Layer::MEATS+1) { //meat food: red/burgundy w/ navy blue bg
			glColor4f(quantity,0.0,0.1,1);
		} else if (live_layersvis==Layer::HAZARDS+1) { //hazards: purple/magenta w/ navy blue bg
			glColor4f(quantity,0.0,quantity*0.9+0.1,1);
		} else if (live_layersvis==Layer::FRUITS+1) { //fruit: yellow w/ navy blue bg
			glColor4f(quantity*0.8,quantity*0.8,0.1,1);
		} else if (live_layersvis==Layer::LAND+1) { //land: green if 1, blue if 0, black otherwise (debug)
			if (quantity==1) glColor4f(0.3,0.7,0.3,1);
			else if (quantity==0) glColor4f(0.3,0.3,0.9,1);
			else glColor4f(0,0,0,1);
		}
		glVertex3f(x*conf::CZ,y*conf::CZ,0);
		glVertex3f(x*conf::CZ+conf::CZ,y*conf::CZ,0);
		glVertex3f(x*conf::CZ+conf::CZ,y*conf::CZ+conf::CZ,0);
		glVertex3f(x*conf::CZ,y*conf::CZ+conf::CZ,0);
		glEnd();
	}
}

void GLView::setWorld(World* w)
{
	world = w;
}
