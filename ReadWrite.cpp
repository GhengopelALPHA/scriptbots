//Read/Write module. Built by Julian R. Hershey, 2012. Use as you wish. Beware of human-made errors!

//The way this works is it includes agent.h, world.h, and glview.h, and is included by glview.cpp.
//I was unable to have this between world.h and glview.h. If you can find a better way to use this,
//please do so and let me know @ julian.hershey@gmail.com

//Currently, GLView has to create an instance of this class whenever saving or loading is needed (see GLView::menu)
//once that is done, the world and other data must be fed to the functions of ReadWrite in order to fully save all data
//It's not pretty, but it works

//Data can be saved in any file format, as the data is put into text form. I tried to emulate XML, but I did not
//strictly follow those rules. There must be a space between the equals sign and the data being loaded, or there
//will be failures. I've made use of a mode var to better control when certain objects and data should be expected.

//For loading agents: I've used <agent> and </agent> tags to outline individual agents. Upon loading, a fake agent
//is created, all attributes which have data to be retrieved are set, and the </agent> tag signals the copy of the
//fake agent's variables into a real agent placed in the world array.

//IN THE FILES: Order does not matter, as each line is read individually from the others. HOWEVER, cell data MUST
//be contained inside <cells>, and agent data MUST be included in its own <agent>. If something goes wrong and 
//two lines modify the same attribute, the latter one will be used.

//VERSION CHECKING: As of now, the system seems to deal with missing data quite harmlessly;
//in the case of agent vars missing, init data values are assigned; in the case of
//world data, nothing can break down because those values are set by the program. Cells may break since it's more 
//complex, so beware. Once loading and saving of cell sizes, world sizes, and brain specifications become enabled, 
//then I forsee possible problems, but for now, I've not had any major errors.
//Even too much data (eg some old data that is no longer used by the program) isn't a
//big issue; simply remove the already removed variable and the line check for it.
//I strongly suggest that one study this code before modifying it. If help is needed on the definitions of built-in
//functions, I found http://www.cplusplus.com/reference/ to be a great reference for such things.

#include "ReadWrite.h"

#include "settings.h"
#include "helpers.h"
#include <stdio.h>
#include <iostream>

using namespace std;

ReadWrite::ReadWrite()
{
	ourfile= "WORLD.SCB";
}

void ReadWrite::loadSettings(const char *filename)
{
	char line[64], *pos;
	char var[16];
	char dataval[16];

	//if no filename given, use default
	if(filename==""){
		filename= "settings.txt";
		printf("No filename given. Loading default settings.txt instead.\n");
	}

	//open the file
	FILE* sf = fopen(filename, "r");

	if(sf){
		printf("file exists! loading");
		while(!feof(sf)){
			fgets(line, sizeof(line), sf);
			pos= strtok(line,"\n");
			sscanf(line, "%s%s", var, dataval);
		
		}
	}
}


void ReadWrite::saveWorld(World *world, float xpos, float ypos, const char *filename)
{
	printf("Filename %s given. Saving...\n", filename);
	//if no filename given, use default
	if(filename==""){
		filename= ourfile;
		printf("CAUTION: No filename given. Loading default %s instead.\n", ourfile);
	}

	//checking for the presence of the file has been relocated to GLView.cpp

	//open save file, write over any previous
	FILE* fs = fopen(filename, "w");
//	strcpy(ourfile,filename);
//	strcpy(ourfile,".txt");
	
	//start with saving world vars and food layer
	float f= 0;
	fprintf(fs,"<world>\n");
/*	fprintf(fs,"inputs= %i\n", INPUTSIZE);
	fprintf(fs,"outputs= %i\n", OUTPUTSIZE);
	fprintf(fs,"brainsize= %i\n", BRAINSIZE);
	fprintf(fs,"connections= %i\n", CONNS);
	fprintf(fs,"width= %i\n", conf::WIDTH);
	fprintf(fs,"height= %i\n", conf::HEIGHT);
	fprintf(fs,"cellsize= %i\n", conf::CZ);*/
	fprintf(fs,"epoch= %i\n", world->current_epoch);
	fprintf(fs,"mod= %i\n", world->modcounter);
	fprintf(fs,"xpos= %f\n", xpos);
	fprintf(fs,"ypos= %f\n", ypos); //GLView xtranslate and ytranslate
	for(int cx=0;cx<world->CW;cx++){ //start with the layers
		for(int cy=0;cy<world->CH;cy++){
			float food= world->cells[0][cx][cy];
			float meat= world->cells[1][cx][cy];
			float hazard= world->cells[2][cx][cy];
			float fruit= world->cells[3][cx][cy];
			int land= world->cells[4][cx][cy];
			fprintf(fs, "<cell>\n"); //signals the writting of a cell
			fprintf(fs, "cx= %i\n", cx);
			fprintf(fs, "cy= %i\n", cy);
			if (food>0) fprintf(fs, "food= %f\n", food); //to further save space, we needn't report a value of a layer for the cell if it's zero
			if (meat>0) fprintf(fs, "meat= %f\n", meat);
			if (hazard>0) fprintf(fs, "hazard= %f\n", hazard);
			if (fruit>0) fprintf(fs, "fruit= %f\n", fruit);
			fprintf(fs, "land= %i\n", land);
			fprintf(fs,"</cell>\n");
		}
	}

	int mini= -1;
	for(int i=0;i<world->agents.size();i++) {
		// here we save all agents. All simulation-significant data must be stored, from the pos and angle, to the change in food the bot was experiencing
		Agent* a= &world->agents[i];

		fprintf(fs, "<agent>\n"); //signals the writing of a new agent
	//	fprintf(fs, "id= %i\n", a->id); //id not loaded
		fprintf(fs, "posx= %f\nposy= %f\n", a->pos.x, a->pos.y);
		fprintf(fs, "angle= %f\n", a->angle);
		fprintf(fs, "health= %f\n", a->health);
	//	fprintf(fs, "red= %f\ngre= %f\nblu= %f\n", a->red, a->gre, a->blu);
	//	fprintf(fs, "w1= %f\nw2= %f\n", w1, w2);
	//	fprintf(fs, "boost= %i\n", (int) a->boost);
		fprintf(fs, "herbivore= %f\n", a->stomach[0]);
		fprintf(fs, "carnivore= %f\n", a->stomach[1]);
		fprintf(fs, "frugivore= %f\n", a->stomach[2]);
		fprintf(fs, "species= %i\n", a->species);
		fprintf(fs, "radius= %f\n", a->radius);
		fprintf(fs, "spike= %f\n", a->spikeLength);
		fprintf(fs, "jump= %f\n", a->jump); //version 6 addition
		fprintf(fs, "dfood= %f\n", a->dfood);
		fprintf(fs, "age= %i\n", a->age);
		fprintf(fs, "gen= %i\n", a->gencount);
		fprintf(fs, "hybrid= %i\n", (int) a->hybrid);
		fprintf(fs, "cl1= %f\ncl2= %f\n", a->clockf1, a->clockf2);
		fprintf(fs, "smellmod= %f\n", a->smell_mod);
		fprintf(fs, "soundmod= %f\n", a->sound_mod);
		fprintf(fs, "hearmod= %f\n", a->hear_mod);
		fprintf(fs, "bloodmod= %f\n", a->blood_mod);
		fprintf(fs, "eyesensemod= %f\n", a->eye_see_agent_mod);
		fprintf(fs, "eyecellmod= %f\n", a->eye_see_cell_mod);
		for(int q=0;q<NUMEYES;q++) {
			fprintf(fs, "<eye>\n");
			fprintf(fs, "eye#= %i\n", q);
			fprintf(fs, "eyedir= %f\n", a->eyedir[q]);
			fprintf(fs, "eyefov= %f\n", a->eyefov[q]);
			fprintf(fs, "</eye>\n");
		}
		for(int q=0;q<NUMEARS;q++) {
			fprintf(fs, "<ear>\n");
			fprintf(fs, "ear#= %i\n", q);
			fprintf(fs, "eardir= %f\n", a->eardir[q]);
			fprintf(fs, "</ear>\n");
		}
		fprintf(fs, "metabolism= %f\n", a->metabolism);
		fprintf(fs, "repcounter= %f\n", a->repcounter);
		fprintf(fs, "killed= %i\n", (int) a->spiked);
		fprintf(fs, "temppref= %f\n", a->temperature_preference);
		fprintf(fs, "lungs= %f\n", a->lungs);
	//	fprintf(fs, "indicator= %f\n", a->indicator);
	//	fprintf(fs, "ir= %f\nig= %f\nib= %f\n", a->ir, a->ig, a->ib);
	//	fprintf(fs, "give= %f\n", a->give);
		fprintf(fs, "mutrate1= %f\nmutrate2= %f\n", a->MUTRATE1, a->MUTRATE2);
		fprintf(fs, "<brain>\n"); //signals the writing of the brain (more for organization than proper loading)

		for(int b=0;b<BRAINSIZE;b++){
			fprintf(fs, "<box>\n"); //signals the writing of a specific numbered box
			fprintf(fs, "box#= %i\n", b);
			fprintf(fs, "kp= %f\n", a->brain.boxes[b].kp);
			fprintf(fs, "bias= %f\n", a->brain.boxes[b].bias);
			fprintf(fs, "globalw= %f\n", a->brain.boxes[b].gw);
			fprintf(fs, "target= %f\n", a->brain.boxes[b].target);
			fprintf(fs, "out= %f\n", a->brain.boxes[b].out);
			fprintf(fs, "oldout= %f\n", a->brain.boxes[b].oldout);
			for(int c=0;c<CONNS;c++){
				fprintf(fs, "<conn>\n"); //signals the writing of a specific connection for a specific box
				fprintf(fs, "conn#= %i\n", c);
				fprintf(fs, "type= %i\n", a->brain.boxes[b].type[c]);
				fprintf(fs, "w= %f\n", a->brain.boxes[b].w[c]);
				fprintf(fs, "cid= %i\n", a->brain.boxes[b].id[c]);
				fprintf(fs, "</conn>\n"); //end of connection
			}
			fprintf(fs, "</box>\n"); //end of box
		}
		fprintf(fs, "</brain>\n"); //end of brain
		fprintf(fs, "</agent>\n"); //end of agent
		if(a->selectflag) mini= i;
	}
	fprintf(fs,"selected= %i\n", mini); //write down which bot was selected
	fprintf(fs,"</world>");
	fclose(fs);
	
	printf("World Saved!\n");
}

void ReadWrite::loadWorld(World *world, float &xtranslate, float &ytranslate, const char *filename)
{
	char line[64], *pos;
	char var[16];
	char dataval[16];
	int cxl;
	int cyl;
	int mode= -1;//loading mode: -1= off, 0= world, 1= cell, 2= agent, 3= box, 4= connection, 5= eyes, 6= ears

	Agent xa; //mock agent. should get moved and deleted after loading

	int eyenum= -1; //counters
	int earnum= -1;
	int boxnum= -1;
	int connnum= -1;
	int i; //integer buffer
	float f; //float buffer

	//if no filename given, use default
	if(filename==""){
		filename= ourfile;
		printf("CAUTION: No filename given. Loading default %s instead.\n", ourfile);
	}

	FILE *fl;
	fl= fopen(filename, "r");
	if(fl){
		printf("file exists! loading");
		while(!feof(fl)){
			fgets(line, sizeof(line), fl);
			pos= strtok(line,"\n");
			sscanf(line, "%s%s", var, dataval);

			if(mode==-1){ //mode @ -1 = off
				if(strcmp(var, "<world>")==0){ //strcmp= 0 when the arguements equal
					//if we find a <world> tag, enable world loading and reset. simple
					mode= 0;
					world->reset();
				}else if(strcmp(var, "<agent>")==0){
					//if we find an <agent> tag, jump right to agent loading (for spawning saved agents)
					mode= 2;
				}
			}else if(mode==0){ //mode @ 0 = world
//				}else if(strcmp(var, "inputs=")==0){
					//inputs count; NOT CURRENTLY USED
//				}else if(strcmp(var, "outputs=")==0){
					//outputs count; NOT CURRENTLY USED
//				}else if(strcmp(var, "brainsize=")==0){
					//brainsize count; NOT CURRENTLY USED
//				}else if(strcmp(var, "connections=")==0){
					//conns count; NOT CURRENTLY USED
//				}else if(strcmp(var, "width=")==0){
					//world width; NOT CURRENTLY USED
//					sscanf(dataval, "%i", &i);
//					conf::WIDTH= i;
//				}else if(strcmp(var, "height=")==0){
					//world height; NOT CURRENTLY USED
//					sscanf(dataval, "%i", &i);
//					conf::HEIGHT= i;
//				}else if(strcmp(var, "cellsize=")==0){
					//CZ; NOT CURRENTLY USED
//					sscanf(dataval, "%i", &i);
//					conf::CZ= i;
				if(strcmp(var, "epoch=")==0){
					//epoch
					sscanf(dataval, "%i", &i);
					world->current_epoch= i;
				}else if(strcmp(var, "mod=")==0){
					//mod count
					sscanf(dataval, "%i", &i);
					world->modcounter= i;
				}else if(strcmp(var, "xpos=")==0){
					//veiw screen location x
					sscanf(dataval, "%f", &f);
					xtranslate= f;
				}else if(strcmp(var, "ypos=")==0){
					//veiw screen location y
					sscanf(dataval, "%f", &f);
					ytranslate= f;
				}else if(strcmp(var, "selected=")==0){
					//selected agent
					sscanf(dataval, "%i", &i);
					if (i>=0 && i<world->agents.size()) world->agents[i].selectflag= true;
				}else if(strcmp(var, "<cell>")==0){ //version 5 (9/2012) has changed cell loading to follow the same pattern as that of agents
					//cells tag activates cell reading mode
					mode= 1;
				}else if(strcmp(var, "<agent>")==0){
					//agent tag activates agent reading mode
					mode= 2;
				}
			}else if(mode==1){ //mode @ 1 = cell
				if(strcmp(var, "</cell>")==0){
					//end_cell tag is checked for first, because of else condition
					mode= 0;
				}else if(strcmp(var, "cx=")==0){
					sscanf(dataval, "%i", &i);
					cxl= i;
				}else if(strcmp(var, "cy=")==0){
					sscanf(dataval, "%i", &i);
					cyl= i;
				}else if(strcmp(var, "food=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[0][cxl][cyl]= f;
				}else if(strcmp(var, "meat=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[1][cxl][cyl]= f;
				}else if(strcmp(var, "hazard=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[2][cxl][cyl]= f;
				}else if(strcmp(var, "fruit=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[3][cxl][cyl]= f;
				}else if(strcmp(var, "land=")==0){
					sscanf(dataval, "%i", &i);
					world->cells[4][cxl][cyl]= i;
				}			
			}else if(mode==2){ //mode @ 2 = agent
				if(strcmp(var, "</agent>")==0){
					//end_agent tag is checked for, and when found, copies agent xa
					mode= 0;
					Agent loadee = xa;
					loadee.id= world->idcounter;
					world->idcounter++;
					world->agents.push_back(loadee);
				}else if(strcmp(var, "posx=")==0){
					sscanf(dataval, "%f", &f);
					xa.pos.x= f;
				}else if(strcmp(var, "posy=")==0){
					sscanf(dataval, "%f", &f);
					xa.pos.y= f;
				}else if(strcmp(var, "angle=")==0){
					sscanf(dataval, "%f", &f);
					xa.angle= f;
				}else if(strcmp(var, "health=")==0){
					sscanf(dataval, "%f", &f);
					xa.health= f;
				}else if(strcmp(var, "herbivore=")==0){
					sscanf(dataval, "%f", &f);
					xa.stomach[0]= f;
				}else if(strcmp(var, "carnivore=")==0){
					sscanf(dataval, "%f", &f);
					xa.stomach[1]= f;
				}else if(strcmp(var, "frugivore=")==0){
					sscanf(dataval, "%f", &f);
					xa.stomach[2]= f;
				}else if(strcmp(var, "species=")==0){
					sscanf(dataval, "%i", &i);
					xa.species= i;
				}else if(strcmp(var, "radius=")==0){
					sscanf(dataval, "%f", &f);
					xa.radius= f;
				}else if(strcmp(var, "spike=")==0){
					sscanf(dataval, "%f", &f);
					xa.spikeLength= f;
				}else if(strcmp(var, "jump=")==0){
					sscanf(dataval, "%f", &f);
					xa.jump= f;
				}else if(strcmp(var, "dfood=")==0){
					sscanf(dataval, "%f", &f);
					xa.dfood= f;
				}else if(strcmp(var, "age=")==0){
					sscanf(dataval, "%i", &i);
					xa.age= i;
				}else if(strcmp(var, "gen=")==0){
					sscanf(dataval, "%i", &i);
					xa.gencount= i;
				}else if(strcmp(var, "hybrid=")==0){
					sscanf(dataval, "%i", &i);
					if(i==1) xa.hybrid= true;
					else xa.hybrid= false;
				}else if(strcmp(var, "cl1=")==0){
					sscanf(dataval, "%f", &f);
					xa.clockf1= f;
				}else if(strcmp(var, "cl2=")==0){
					sscanf(dataval, "%f", &f);
					xa.clockf2= f;
				}else if(strcmp(var, "smellmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.smell_mod= f;
				}else if(strcmp(var, "soundmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.sound_mod= f;
				}else if(strcmp(var, "hearmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.hear_mod= f;
				}else if(strcmp(var, "bloodmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.blood_mod= f;
				}else if(strcmp(var, "eyesensemod=")==0){
					sscanf(dataval, "%f", &f);
					xa.eye_see_agent_mod= f;
				}else if(strcmp(var, "eyecellmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.eye_see_cell_mod= f;
				}else if(strcmp(var, "metabolism=")==0){
					sscanf(dataval, "%f", &f);
					xa.metabolism= f;
				}else if(strcmp(var, "repcounter=")==0){
					sscanf(dataval, "%f", &f);
					xa.repcounter= f;
				}else if(strcmp(var, "killed=")==0){
					sscanf(dataval, "%i", &i);
					if (i==1) xa.spiked= true;
					else xa.spiked= false;
				}else if(strcmp(var, "temppref=")==0){
					sscanf(dataval, "%f", &f);
					xa.temperature_preference= f;
				}else if(strcmp(var, "lungs=")==0){
					sscanf(dataval, "%f", &f);
					xa.lungs= f;
				}else if(strcmp(var, "mutrate1=")==0){
					sscanf(dataval, "%f", &f);
					xa.MUTRATE1= f;
				}else if(strcmp(var, "mutrate2=")==0){
					sscanf(dataval, "%f", &f);
					xa.MUTRATE2= f;
				}else if(strcmp(var, "<eye>")==0){
					mode= 5;
				}else if(strcmp(var, "<box>")==0){
					mode= 3;
				}else if(strcmp(var, "<ear>")==0){
					mode= 6;
				}
			}else if(mode==5){ //mode @ 5 = eye (of agent)
				if(strcmp(var, "</eye>")==0){
					mode= 2;
				}else if(strcmp(var, "eye#=")==0){
					sscanf(dataval, "%i", &i);
					eyenum= i;
				}else if(strcmp(var, "eyedir=")==0){
					sscanf(dataval, "%f", &f);
					xa.eyedir[eyenum]= f;
				}else if(strcmp(var, "eyefov=")==0){
					sscanf(dataval, "%f", &f);
					xa.eyefov[eyenum]= f;
				}
			}else if(mode==6){ //mode @ 6 = ear (of agent)
				if(strcmp(var, "</ear>")==0){
					mode= 2;
				}else if(strcmp(var, "ear#=")==0){
					sscanf(dataval, "%i", &i);
					earnum= i;
				}else if(strcmp(var, "eardir=")==0){
					sscanf(dataval, "%f", &f);
					xa.eardir[earnum]= f;
				}
			}else if(mode==3){ //mode @ 3 = brain box (of agent)
				if(strcmp(var, "</box>")==0){
					mode= 2;
				}else if(strcmp(var, "box#=")==0){
					sscanf(dataval, "%i", &i);
					boxnum= i;
				}else if(strcmp(var, "kp=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].kp= f;
				}else if(strcmp(var, "bias=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].bias= f;
				}else if(strcmp(var, "globalw=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].gw= f;
				}else if(strcmp(var, "target=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].target= f;
				}else if(strcmp(var, "out=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].out= f;
				}else if(strcmp(var, "oldout=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].oldout= f;
				}else if(strcmp(var, "<conn>")==0){
					mode= 4;
				}
			}else if(mode==4){ //mode @ 4 = connection (of brain box of agent)
				if(strcmp(var, "</conn>")==0){
					mode= 3;
				}else if(strcmp(var, "conn#=")==0){
					sscanf(dataval, "%i", &i);
					connnum= i;
				}else if(strcmp(var, "type=")==0){
					sscanf(dataval, "%i", &i);
					xa.brain.boxes[boxnum].type[connnum]= i;
				}else if(strcmp(var, "w=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].w[connnum]= f;
				}else if(strcmp(var, "cid=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].id[connnum]= f;
				}
			}
		}
		fclose(fl);

		printf("World Loaded Successfully!\n");

		world->setInputs();
		world->brainsTick();

	} else { //DOH! the file doesn't exist!
		printf("ERROR: Save file specified (%s) doesn't exist!\n", filename);
	}
}
