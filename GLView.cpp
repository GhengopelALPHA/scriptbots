#include "GLView.h"
#include "ReadWrite.h"

#include <ctime>

#include "config.h"
#ifdef LOCAL_GLUT32
#include "glut.h"
#else
#include <GL/glut.h>
#endif

//#include "glui.h"

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
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
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
		//paused(false),
		//draw(true),
		//skipdraw(1),
		//agentvisual(1),
		//layer(1),
		modcounter(0),
		frames(0),
		lastUpdate(0)
{

	xtranslate= 0.0;
	ytranslate= 0.0;
	scalemult= 0.2;
	downb[0]=0;downb[1]=0;downb[2]=0;
	mousex=0;mousey=0;
	
	//following = false;
}

GLView::~GLView()
{

}
void GLView::changeSize(int w, int h)
{
	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,conf::WWIDTH,conf::WHEIGHT,0,0,1);
}

void GLView::processMouse(int button, int state, int x, int y)
{
	if(world->isDebug()) printf("MOUSE EVENT: button=%i state=%i x=%i y=%i\n", button, state, x, y);
	
	//have world deal with it. First translate to world coordinates though
	if(button==0){
		int wx= (int) ((x-conf::WWIDTH/2)/scalemult-xtranslate);
		int wy= (int) ((y-conf::WHEIGHT/2)/scalemult-ytranslate);

		world->processMouse(button, state, wx, wy);
	}
	
	mousex=x; mousey=y;
	downb[button]=1-state; //state is backwards, ah well
}

void GLView::processMouseActiveMotion(int x, int y)
{
	if(world->isDebug()) printf("MOUSE MOTION x=%i y=%i, %i %i %i\n", x, y, downb[0], downb[1], downb[2]);
	if (downb[0]==1) {
		//left mouse button drag: pan around
		xtranslate += (x-mousex)/scalemult;
		ytranslate += (y-mousey)/scalemult;
		if (abs(x-mousex)>6 || abs(x-mousex)>6) live_following= 0;
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
   
	mousex=x; mousey=y;
}

void GLView::processMousePassiveMotion(int x, int y)
{
	//for mouse scrolling. [DISABLED]
/*	if(y<=30) ytranslate += 2*(30-y);
	if(y>=conf::WHEIGHT-30) ytranslate -= 2*(y-(conf::WHEIGHT-30));
	if(x<=30) xtranslate += 2*(30-x);
	if(x>=conf::WWIDTH-30) xtranslate -= 2*(x-(conf::WWIDTH-30));*/
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
		if (live_layersvis>LAYERS) live_layersvis= 0;
		if (live_layersvis<0) live_layersvis= LAYERS;
	} else if (key=='z' || key=='x') { //change agent visual scheme; x= "next", z= "previous"
		if (key=='x') live_agentsvis++;
		else live_agentsvis--;
		if (live_agentsvis>4) live_agentsvis= 0;
		if (live_agentsvis<0) live_agentsvis= 4; //4 here and above because there are 5 options (0,1,2,3,4)
	} else if (key==1002) {
		world->addRandomBots(5);
	} else if (key==1003) {
		world->addRandomBots(5, 2);
	} else if (key==1004) {
		world->addRandomBots(5, 1);
	} else if (key=='c') {
		world->setClosed( !world->isClosed() );
		live_worldclosed= (int) world->isClosed();
/*		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->isClosed()) glutChangeToMenuEntry(4, "Open World", 'c');
		else glutChangeToMenuEntry(4, "Close World", 'c');
		glutSetMenu(m_id);*/
	} else if (key=='f') {
		if(live_following==0) live_following= 2; //follow selected agent
		else live_following= 0;
	} else if(key=='o') {
		if(live_following==0) live_following= 1; //follow oldest agent
		else live_following= 0;
	} else if(key=='g') {
		if(live_following==0) live_following= 3; //follow most advanced generation agent
		else live_following= 0;
	} else if(key=='h') {
		if(live_following==0) live_following= 4; //follow healthiest
		else live_following= 0;
	}else if (key==127) { //delete
		world->deleting= 1;
	}else if (key==62) { //zoom+ >
		scalemult += 10*conf::ZOOM_SPEED;
	}else if (key==60) { //zoom- <
		scalemult -= 10*conf::ZOOM_SPEED;
		if(scalemult<0.01) scalemult=0.01;
	}else if (key==32) { //spacebar input [pressed]
		world->pinput1= 1;
	}else if (key==119) { //w (move faster)
		world->pcontrol= true;
		world->pleft= cap(world->pleft + 0.08);
		world->pright= cap(world->pright + 0.08);
	}else if (key==97) { //a (turn left)
		world->pcontrol= true;
		world->pleft= cap(world->pleft - 0.05 + (world->pright-world->pleft)*0.05); //this extra code helps with turning out of tight circles
		world->pright= cap(world->pright + 0.05 + (world->pleft-world->pright)*0.05);
	}else if (key==115) { //s (move slower)
		world->pcontrol= true;
		world->pleft= cap(world->pleft - 0.08);
		world->pright= cap(world->pright - 0.08);
	}else if (key==100) { //d (turn right)
		world->pcontrol= true;
		world->pleft= cap(world->pleft + 0.05 + (world->pright-world->pleft)*0.05);
		world->pright= cap(world->pright - 0.05 + (world->pleft-world->pright)*0.05);
	} else if (key==999) { //player control
		world->setControl(!world->pcontrol);
		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->pcontrol) glutChangeToMenuEntry(5, "Release Agent", 999);
		else glutChangeToMenuEntry(5, "Control Agent", 999);
		glutSetMenu(m_id);
	}else if (key==1000) { //menu only, save world
		printf("SAVING WORLD\n");
		printf("Type a valid file name (ex: WORLD.SCB): ");
		scanf("%s", filename);
		savehelper->saveWorld(world, xtranslate, ytranslate, filename);
	}else if (key==1001) { //menu only, load world
		printf("LOADING WORLD\n");
		printf("Type a valid file name (ex: WORLD.SCB): ");
		scanf("%s", filename);
		savehelper->loadWorld(world, xtranslate, ytranslate, filename);
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

void GLView::menuS(int key) // (GPA) movement control
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
	glutAddMenuEntry("Spawn Herbivores", 1003);
	glutAddMenuEntry("Spawn Carnivores", 1004);
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
	//must set our live vars to something. Might as well do it here
	live_worldclosed= 0;
	live_paused= 0;
	live_fastmode= 0;
	live_skipdraw= 1;
	live_agentsvis= 1;
	live_layersvis= 1;
	live_following= 0;
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
	new GLUI_RadioButton(group_layers,"Plants");
	new GLUI_RadioButton(group_layers,"Meat");
	new GLUI_RadioButton(group_layers,"Waste");
	new GLUI_RadioButton(group_layers,"Fruit");
	new GLUI_RadioButton(group_layers,"Map");
	Menu->add_column_to_panel(rollout_vis,true);
	GLUI_RadioGroup *group_agents= new GLUI_RadioGroup(rollout_vis,&live_agentsvis);
	new GLUI_StaticText(rollout_vis,"Agents");
	new GLUI_RadioButton(group_agents,"off");
	new GLUI_RadioButton(group_agents,"RGB");
	new GLUI_RadioButton(group_agents,"Stomach");
	new GLUI_RadioButton(group_agents,"Comfort");
	new GLUI_RadioButton(group_agents,"Voice");

	GLUI_Panel *panel_xyl= new GLUI_Panel(Menu,"Following");
	GLUI_RadioGroup *group_follow= new GLUI_RadioGroup(panel_xyl,&live_following);
	new GLUI_RadioButton(group_follow,"off");
	new GLUI_RadioButton(group_follow,"Oldest");
	new GLUI_RadioButton(group_follow,"Selected");
	new GLUI_RadioButton(group_follow,"Best Gen.");
	new GLUI_RadioButton(group_follow,"Healthiest");
	new GLUI_RadioButton(group_follow,"Productive");

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
	}
}

void GLView::handleCloses(int action) //GLUI callback for handling window closing
{
	live_paused= 0;

	ReadWrite* savehelper= new ReadWrite(); //for loading/saving

	if (action==1){ //loading
		strcpy(filename,Filename->get_text());
		if (debug) printf("Filename: '%s'",filename);
		if (!filename || filename==""){
			printf("ERROR: empty filename; returning to program./n");

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
		strcpy(filename,Filename->get_text());
		if (debug) printf("Filename: '%s'",filename);
		if (!filename || filename==""){
			printf("ERROR: empty filename; returning to program./n");

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert, "No file name given." );
			new GLUI_StaticText(Alert, "Returning to main program." );
			new GLUI_Button(Alert,"Okay");

		} else {
			//check the filename given to see if it exists yet
			FILE* ck = fopen(filename, "r");
			if(ck){
				if (debug) printf("WARNING: %s already exists!\n", filename);

				Alert = GLUI_Master.create_glui("Alert",0,50,50);
				Alert->show();
				new GLUI_StaticText(Alert, "The file name given already exists." );
				new GLUI_StaticText(Alert, "Would you like to overwrite?" );
				new GLUI_Button(Alert,"Okay",5, glui_handleCloses);
				new GLUI_Button(Alert,"Cancel",4, glui_handleCloses);
				live_paused= 1;
			} else {
				savehelper->saveWorld(world, xtranslate, ytranslate, filename);
			}
		}
		Saver->hide();

	} else if (action==3){ //resetting
		world->reset();
		printf("World reset\n");
		Alert->hide();
	} else if (action==4){ //Alert cancel/continue
		Alert->hide();
	} if (action==5){ //Alert from above saving
		savehelper->saveWorld(world, xtranslate, ytranslate, filename);
		Alert->hide();
	}
}

void GLView::handleIdle()
{
	//set proper window (we don't want to draw on nothing, now do we?!)
	if (glutGetWindow() != win1) glutSetWindow(win1); 

	GLUI_Master.sync_live_all();

	//after syncing all the live vars with GLUI_Master, set the vars they represent to their proper values.
	if (live_worldclosed==1) world->setClosed(1);
	else world->setClosed(0);
	debug= (bool) live_debug;
	world->setDebug(debug);
	
	modcounter++;
	if (!live_paused) world->update();

	//show FPS
	int currentTime = glutGet( GLUT_ELAPSED_TIME );
	frames++;
	if ((currentTime - lastUpdate) >= 1000) {
		sprintf( buf, "FPS: %d speed: %d Total Agents: %d Herbivores: %d Carnivores: %d Frugivores: %d Epoch: %d",
			frames, live_skipdraw, world->getAgents(), world->getHerbivores(), world->getCarnivores(), world->getFrugivores(), world->epoch() );
		glutSetWindowTitle( buf );
		frames = 0;
		lastUpdate = currentTime;
	}
	if (live_skipdraw<=0 && !live_fastmode) {
		clock_t endwait;
		float mult=-0.005*(live_skipdraw-1); //ugly, ah well
		endwait = clock () + mult * CLOCKS_PER_SEC ;
		while (clock() < endwait) {}
	}

	if (!live_fastmode) {
		if (live_skipdraw>0) {
			if (modcounter%live_skipdraw==0) renderScene();	//increase fps by skipping drawing
		}
		else renderScene(); //we will decrease fps by waiting using clocks
	}
}

void GLView::renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();

	glTranslatef(conf::WWIDTH/2, conf::WHEIGHT/2, 0.0f);	
	glScalef(scalemult, scalemult, 1.0f);
	
	if(live_following!=0){
		float xi=0, yi=0;
		world->positionOfInterest(live_following, xi, yi);

		if(xi!=0 && yi!=0){
			xtranslate+= conf::SNAP_SPEED*(-xi-xtranslate); ytranslate+= conf::SNAP_SPEED*(-yi-ytranslate);
		}
	}
	glTranslatef(xtranslate, ytranslate, 0.0f);
	
	world->draw(this, live_layersvis);

	glPopMatrix();
	glutSwapBuffers();
}

void GLView::drawAgent(const Agent& agent)
{
	float n;
	float r= agent.radius;
	float rp= agent.radius+2.5;

	if (live_agentsvis!=0) {

		//calculate temperature at the agents spot. (based on distance from horizontal equator) Orange is bad, white is good
		//done here because also used in coloring bots with agent visual set to temp discomfort
		float dd= 2.0*abs(agent.pos.y/conf::HEIGHT - 0.5);
		float discomfort= cap(sqrt(abs(dd-agent.temperature_preference)));

		//handle selected agent
		if (agent.selectflag>0) {

			//draw selection
			glBegin(GL_POLYGON);
			glColor3f(1,1,1);
			drawCircle(agent.pos.x, agent.pos.y, agent.radius+5);
			glEnd();

			glPushMatrix();
			glTranslatef(agent.pos.x-80,agent.pos.y+20,0);
			//draw inputs, outputs
			float col;
			float yy=15;
			float xx=15;
			float ss=16;
			glBegin(GL_QUADS);
			for (int j=0;j<INPUTSIZE;j++) {
				col= agent.in[j];
				glColor3f(col,col,col);
				glVertex3f(0+ss*j, 0, 0.0f);
				glVertex3f(xx+ss*j, 0, 0.0f);
				glVertex3f(xx+ss*j, yy, 0.0f);
				glVertex3f(0+ss*j, yy, 0.0f);
			}
			yy+=5;
			for (int j=0;j<OUTPUTSIZE;j++) {
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
					if(j<INPUTSIZE) { x1= j*ss; y1= yy; }
					else { 
						x1= ((j-INPUTSIZE)%30)*ss;
						y1= yy+ss+2*ss*((int) (j-INPUTSIZE)/30);
					}
					
					float x2= 0;
					float y2= 0;
					if(j2<INPUTSIZE) { x2= j2*ss; y2= yy; }
					else { 
						x2= ((j2-INPUTSIZE)%30)*ss;
						y2= yy+ss+2*ss*((int) (j2-INPUTSIZE)/30);
					}
					
					float ww= agent.brain.boxes[j].w[k];
					if(ww<0) glColor3f(-ww, 0, 0);
					else glColor3f(0,0,ww);
					
					glVertex3f(x1,y1,0);
					glVertex3f(x2,y2,0);
				}
			}
			*/

			glEnd();
			glPopMatrix();
		}

		//draw giving/receiving
		if(agent.dfood!=0){
			glBegin(GL_POLYGON);
			float mag=cap(abs(agent.dfood)/conf::FOODTRANSFER/3);
			if(agent.dfood>0) glColor3f(0,mag,0);
			else glColor3f(mag,0,0); //draw sharing as a thick green or red outline
			for (int k=0;k<17;k++){
				n = k*(M_PI/8);
				glVertex3f(agent.pos.x+rp*sin(n),agent.pos.y+rp*cos(n),0);
				n = (k+1)*(M_PI/8);
				glVertex3f(agent.pos.x+rp*sin(n),agent.pos.y+rp*cos(n),0);
			}
			glEnd();
		}

		//draw indicator of this agent... used for various events
		if (agent.indicator>0) {
			glBegin(GL_POLYGON);
			glColor3f(agent.ir,agent.ig,agent.ib);
			drawCircle(agent.pos.x, agent.pos.y, agent.radius+((int)agent.indicator));
			glEnd();
		}
		
		
		//draw eyes
		for(int q=0;q<NUMEYES;q++) {
			glBegin(GL_LINES);
			glColor3f(0.5,0.5,0.5);
			glVertex3f(agent.pos.x,agent.pos.y,0);
			float aa= agent.angle+agent.eyedir[q];
			glVertex3f(agent.pos.x+(agent.radius+30)*cos(aa),
					   agent.pos.y+(agent.radius+30)*sin(aa),
					   0);
			glEnd();
		}

		//ears
		for(int q=0;q<NUMEARS;q++) {
			glBegin(GL_POLYGON);
			glColor3f(0.6,0.6,0);
			float aa= agent.angle + agent.eardir[q];
			drawCircle(agent.pos.x+agent.radius*cos(aa),
					   agent.pos.y+agent.radius*sin(aa),
					   1.5);
			glEnd();
		}
		
		glBegin(GL_POLYGON); 
		//body
		if (live_agentsvis==1) glColor3f(agent.red,agent.gre,agent.blu);
		else if (live_agentsvis==2) glColor3f(cap(agent.stomach[1]+agent.stomach[2]),cap(agent.stomach[0]+agent.stomach[2]),0);
		else if (live_agentsvis==3) glColor3f(1,(2-discomfort)/2,(1-discomfort));
		else if (live_agentsvis==4) glColor3f(agent.soundmul,agent.soundmul,agent.soundmul);
		drawCircle(agent.pos.x, agent.pos.y, agent.radius);
		glEnd();

		glBegin(GL_LINES);
		//outline
		glColor3f(0,0,0);
		if (live_agentsvis==1) {
			if (agent.boost) glColor3f(0.8,0,0); //draw boost as red outline
			if (agent.jump>0) glColor3f(0.8,0.8,0); //draw jumping as yellow outline. overrides boost
		}
		else if (live_agentsvis==2) glColor3f(cap(agent.stomach[1]+agent.stomach[2]),cap(agent.stomach[0]+agent.stomach[2]),0);
		else if (live_agentsvis==3) glColor3f(1,(2-discomfort)/2,(1-discomfort));
		else if (live_agentsvis==4) glColor3f(agent.soundmul,agent.soundmul,agent.soundmul);
		for (int k=0;k<17;k++)
		{
			n = k*(M_PI/8);
			glVertex3f(agent.pos.x+r*sin(n),agent.pos.y+r*cos(n),0);
			n = (k+1)*(M_PI/8);
			glVertex3f(agent.pos.x+r*sin(n),agent.pos.y+r*cos(n),0);
		}
		//and spike
		glColor3f(0.5,0,0);
		glVertex3f(agent.pos.x,agent.pos.y,0);
		glVertex3f(agent.pos.x+(conf::SPIKELENGTH*agent.spikeLength)*cos(agent.angle),
				   agent.pos.y+(conf::SPIKELENGTH*agent.spikeLength)*sin(agent.angle),
				   0);
		glEnd();

		if(scalemult > .3) {//hide extra visual data if really far away

			//debug sight lines
			if(world->isDebug()) {
				for (int i=0;i<world->linesA.size();i++) {
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
			glVertex3f(agent.pos.x+xo,agent.pos.y+yo,0);
			glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo,0);
			glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo+40,0);
			glVertex3f(agent.pos.x+xo,agent.pos.y+yo+40,0);

			glColor3f(0,0.8,0);
			glVertex3f(agent.pos.x+xo,agent.pos.y+yo+20*(2-agent.health),0);
			glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo+20*(2-agent.health),0);
			glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo+40,0);
			glVertex3f(agent.pos.x+xo,agent.pos.y+yo+40,0);

			//hybrid marker
			if (agent.hybrid) {
				glColor3f(0,0,0.8);
				glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo,0);
				glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo,0);
				glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+10,0);
				glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+10,0);
			}

			//stomach type indicator
			glColor3f(cap(agent.stomach[1]+agent.stomach[2]),cap(agent.stomach[0]+agent.stomach[2]),0);
			glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+12,0);
			glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+12,0);
			glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+22,0);
			glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+22,0);

			//sound volume indicator
			glColor3f(agent.soundmul,agent.soundmul,agent.soundmul);
			glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+24,0);
			glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+24,0);
			glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+34,0);
			glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+34,0);

			//temp discomfort indicator
			if (discomfort<0.08) discomfort=0;
			glColor3f(1,(2-discomfort)/2,(1-discomfort));
			glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+36,0);
			glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+36,0);
			glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+46,0);
			glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+46,0);

			//land/water lungs requirement indicator
			glColor3f(0.2,0.4*agent.lungs+0.3,0.6*(1-agent.lungs)+0.2);
			glVertex3f(agent.pos.x+xo+14,agent.pos.y+yo,0);
			glVertex3f(agent.pos.x+xo+20,agent.pos.y+yo,0);
			glVertex3f(agent.pos.x+xo+20,agent.pos.y+yo+10,0);
			glVertex3f(agent.pos.x+xo+14,agent.pos.y+yo+10,0);
		}

		glEnd();

		//print stats
		if(scalemult > .7) { // hide the number stats when zoomed out
			//generation count
			sprintf(buf2, "%i", agent.gencount);
			RenderString(agent.pos.x-agent.radius*1.5,
						 agent.pos.y+agent.radius*1.8,
						 GLUT_BITMAP_TIMES_ROMAN_24,
						 buf2, 0.8f, 1.0f, 1.0f);

			//age
			sprintf(buf2, "%i", agent.age);
			float x = cap((float) agent.age/conf::MAXAGE); //will be redder the closer it is to MAXAGE
			RenderString(agent.pos.x-agent.radius*1.5,
						 agent.pos.y+agent.radius*1.8+12,
						 GLUT_BITMAP_TIMES_ROMAN_24,
						 buf2, 0.8f, 1.0-x, 1.0-x);

			//health
			sprintf(buf2, "%.2f", agent.health);
			RenderString(agent.pos.x-agent.radius*1.5,
						 agent.pos.y+agent.radius*1.8+24,
						 GLUT_BITMAP_TIMES_ROMAN_24,
						 buf2, 0.8f, 1.0f, 1.0f);

			//repcounter
			float dr = agent.metabolism/conf::MAXMETABOLISM; //red if high metabolism, blue if low 
			sprintf(buf2, "%.2f", agent.repcounter);
			RenderString(agent.pos.x-agent.radius*1.5,
						 agent.pos.y+agent.radius*1.8+36,
						 GLUT_BITMAP_TIMES_ROMAN_24,
						 buf2, dr/2+0.5, dr/2+0.5, (1.0-dr)/2+0.5);
		}
	}
}

void GLView::drawData()
{
	float mm = 3;
	//draw misc info
	glBegin(GL_LINES);
	glColor3f(0,0,0); //border around graphs and feedback
	glVertex3f(0,0,0);
	glVertex3f(-2020,0,0);
	glVertex3f(-2020,0,0);
	glVertex3f(-2020,conf::HEIGHT,0);
	glVertex3f(-2020,conf::HEIGHT,0);
	glVertex3f(0,conf::HEIGHT,0);

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
	RenderString(-2020 + world->ptr*10 + 3,conf::HEIGHT -mm*world->getAgents(), GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
	
	RenderString(2700, -80, GLUT_BITMAP_TIMES_ROMAN_24, "Useful Keybindings: 'm' disables drawing, 'p' pauses the sim, 'l' & 'k' swtich layer view, and 'z' & 'x' switch agent view.", 0.0f, 0.0f, 0.0f);
	RenderString(2700, -20, GLUT_BITMAP_TIMES_ROMAN_24, "Try clicking on a bot. Use 'w,a,s,d' to control it. 'delete' deletes it. 'spacebar' triggers an input.", 0.0f, 0.0f, 0.0f);
	if(live_paused) RenderString(-2010, 50, GLUT_BITMAP_TIMES_ROMAN_24, "PAUSED", 0.0f, 0.0f, 0.0f);
	if(live_following!=0) RenderString(-xtranslate+(10-conf::WWIDTH/2)/scalemult, -ytranslate+(30-conf::WHEIGHT/2)/scalemult, GLUT_BITMAP_TIMES_ROMAN_24, "FOLLOWING", 0.5f, 0.5f, 0.5f);
	if(world->isClosed()) RenderString(-1500, 50, GLUT_BITMAP_TIMES_ROMAN_24, "CLOSED WORLD", 0.0f, 0.0f, 0.0f);
}

void GLView::drawCell(int x, int y, float quantity)
{
	if (live_layersvis!=0) { //none: white
		glBegin(GL_QUADS);
		if (live_layersvis==1) { //plant food: green w/ navy blue background
			glColor3f(0.0,quantity,0.1);
		} else if (live_layersvis==2) { //meat food: red/burgundy w/ navy blue bg
			glColor3f(quantity,0.0,0.1);
		} else if (live_layersvis==3) { //hazards: purple/magenta w/ navy blue bg
			glColor3f(quantity,0.0,quantity*0.9+0.1);
		} else if (live_layersvis==4) { //fruit: yellow w/ navy blue bg
			glColor3f(quantity,quantity,0.1);
		} else if (live_layersvis==5) { //land: green if 1, blue if 0, black otherwise (debug)
			if (quantity==1) glColor3f(0.3,0.7,0.3);
			else if (quantity==0) glColor3f(0.3,0.3,0.9);
			else glColor3f(0,0,0);
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
