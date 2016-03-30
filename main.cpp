#include "GLView.h"
#include "World.h"

#include <ctime>

#include "config.h"

#ifdef LOCAL_GLUT32
	#include "glut.h"
#else
	#include <GL/glut.h>
#endif

#include "glui.h"

#include <stdio.h>


GLView* GLVIEW = new GLView(0);
int main(int argc, char **argv) {
	srand(time(0));

	//load settings. This will be different...
	//import readwrite for this?
//	int window;
//	GLUI *menu;

	if (conf::WIDTH%conf::CZ!=0 || conf::HEIGHT%conf::CZ!=0) printf("CAREFUL! The cell size variable conf::CZ should divide evenly into  both conf::WIDTH and conf::HEIGHT! It doesn't right now!");
	
	printf("Controls:\n p= pause\n m= toggle drawing (for faster computation)\n += faster, -= slower\n delete= delete selected agent\n tab= reset world\n");
	printf("Pan around by dragging with left mouse button or the arrow keys, zoom by dragging middle mouse button or the '<' and '>' keys. Right-click on the world for options\n");
	printf( "GLUI version: %3.2f\n", GLUI_Master.get_version() );

	World* world = new World();
	GLVIEW->setWorld(world);

	//GLUT SETUP
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(30,30);
	glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);
	GLVIEW->win1= glutCreateWindow("Scriptbots");

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	GLUI_Master.set_glutDisplayFunc(gl_renderScene);
	GLUI_Master.set_glutIdleFunc(gl_handleIdle);
	GLUI_Master.set_glutReshapeFunc(gl_changeSize);

	glutKeyboardFunc(gl_processNormalKeys);
	glutSpecialFunc(gl_processSpecialKeys);
	glutKeyboardUpFunc(gl_processReleasedKeys);
	glutMouseFunc(gl_processMouse);
	glutMotionFunc(gl_processMouseActiveMotion);
	glutPassiveMotionFunc(gl_processMousePassiveMotion);

	//New menu window. Will it work THIS time?
	GLVIEW->gluiCreateMenu();

	//create right click context menu
	GLVIEW->glCreateMenu();

	try{
		glutMainLoop();
	} catch( std::bad_alloc &){
		printf("Out of memory!\n");
	} catch( std::bad_exception &){
		printf("Severe error!\n");
	}
	return 0;
}
