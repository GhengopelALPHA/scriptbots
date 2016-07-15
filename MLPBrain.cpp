#include "MLPBrain.h"
using namespace std;


MLPBox::MLPBox()
{

	w.resize(CONNS,0);
	id.resize(CONNS,0);
	type.resize(CONNS,0);

	//constructor
	for (int i=0;i<CONNS;i++) {
		w[i]= randf(-5,5);
		if(randf(0,1)<conf::BRAIN_DEADCONNS) w[i]=0; //we might want to simulate brain development over time, so set some conns to zero weight
		
		id[i]= randi(0,BRAINSIZE);
		if (randf(0,1)<conf::BRAIN_DIRECTINPUT) id[i]= randi(0,Input::INPUT_SIZE); //connect a portion of the brain directly to input.
		
		type[i]= 0;
		if(randf(0,1)<conf::BRAIN_CHANGECONNS) type[i] = 1; //some conns can be change sensitive synapses
		if(randf(0,1)<conf::BRAIN_MEMCONN) type[i] = 2; //some conns can be memory synapses
	}

	seed= 0;
	kp= randf(0.01,1);
	gw= randf(-2,2);
	bias= randf(-3,3);

	out= 0;
	oldout= 0;
	target= 0;
}

MLPBrain::MLPBrain()
{
	//constructor
	// do not OMP!!!
	for (int i=0;i<BRAINSIZE;i++) {
		MLPBox a; //make a random box and copy it over
		boxes.push_back(a);
		}

	//do other initializations
	init();
}

MLPBrain::MLPBrain(const MLPBrain& other)
{
	boxes = other.boxes;
}

MLPBrain& MLPBrain::operator=(const MLPBrain& other)
{
	if( this != &other )
		boxes = other.boxes;
	return *this;
}


void MLPBrain::init()
{

}

void MLPBrain::tick(vector< float >& in, vector< float >& out)
{
	//do a single tick of the brain
	for (int j=0; j<(int)boxes.size(); j++){
		MLPBox* abox= &boxes[j];
		
		if (j<Input::INPUT_SIZE) { //take first few boxes and set their out to in[]. (no need to do these separately, since thay are first)
			abox->out= in[j];
		} else { //then do a dynamics tick and set all targets
			float acc=abox->bias;

			for (int k=0;k<CONNS;k++) {
				int idx=abox->id[k];
				int type = abox->type[k];
				float val= boxes[idx].out;

				if(type==2){ //switch conn
					if(val>0.5){
						break;
						continue;
					}
					continue;
				}
				
				if(type==1){ //change sensitive conn
					val-= boxes[idx].oldout;
					val*=10;
				}

				acc+= val*abox->w[k];
			}
			
			acc*= abox->gw;
			
			//put through sigmoid
			acc= 1.0/(1.0+exp(-acc));
			
			abox->target= cap(acc);
		}
	}
	

	for (int j=0; j<(int)boxes.size(); j++){
		MLPBox* abox= &boxes[j];

		//back up current out for each box
		abox->oldout = abox->out;

		//make all boxes go a bit toward target
		if (j>=Input::INPUT_SIZE) abox->out+= (abox->target-abox->out)*abox->kp;
	}

	//finally set out[] to the last few boxes output
	for (int j=0;j<Output::OUTPUT_SIZE;j++) {
		//jump has different responce because we've made it into a change sensitive output
		if (j==Output::JUMP) out[j]= cap(10*(boxes[BRAINSIZE-1-j].out-boxes[BRAINSIZE-1-j].oldout));
		else out[j]= boxes[BRAINSIZE-1-j].out;
	}
}

float MLPBrain::getActivity()
{
	float sum= 0;
	for (int j=0; j<(int)boxes.size(); j++){
		MLPBox* abox= &boxes[j];
		sum+= fabs(abox->out - abox->oldout);
	}
	return sum/BRAINSIZE;
}

void MLPBrain::initMutate(float MR, float MR2)
{
	//for mutations which may occur at conception
	for (int j=0; j<(int)boxes.size(); j++){
		MLPBox* abox= &boxes[j];
		if (randf(0,1)<MR/50) {
			//randomize synapse type
			int rc= randi(0, CONNS);
			abox->type[rc] = randi(0,2);
//		  a2.mutations.push_back("synapse type randomized\n");
			abox->seed= 0;
		}

		if (randf(0,1)<MR/40) {
			//copy box
			int k= randi(0,BRAINSIZE);
			if(k!=j) {
				abox->type= boxes[k].type;
				abox->id= boxes[k].id;
				abox->bias= boxes[k].bias;
				abox->kp= boxes[k].kp;
				abox->type= boxes[k].type;
				abox->w= boxes[k].w;
//				a2.mutations.push_back("box coppied\n");
				abox->seed= 0;
			}
		}

		if (randf(0,1)<MR/20) {
			//randomize connection
			int rc= randi(0, CONNS);
			int ri= randi(0,BRAINSIZE);
			abox->id[rc]= ri;
//		  a2.mutations.push_back("connection randomized\n");
			abox->seed= 0;
		}

		if (randf(0,1)<MR/10) {
			//swap two input sources
			int rc1= randi(0, CONNS);
			int rc2= randi(0, CONNS);
			int temp= abox->id[rc1];
			abox->id[rc1]= abox->id[rc2];
			abox->id[rc2]= temp;
//			 a2.mutations.push_back("inputs swapped\n");
			abox->seed= 0;
		}

		// more likely changes here
		if (randf(0,1)<MR/2) {
			//jiggle global weight
			abox->gw+= randn(0, MR2);
			if (abox->gw<0) abox->gw=0;
//			 a2.mutations.push_back("global weight jiggled\n");
//			abox->seed= 0;
		}

		if (randf(0,1)<MR) {
			//jiggle bias
			abox->bias+= randn(0, MR2);
//			 a2.mutations.push_back("bias jiggled\n");
//			abox->seed= 0;
		}

		if (randf(0,1)<MR) {
			//jiggle dampening
			abox->kp+= randn(0, MR2);
			if (abox->kp<0.01) abox->kp=0.01;
			if (abox->kp>1) abox->kp=1;
//			 a2.mutations.push_back("kp jiggled\n");
//			abox->seed= 0;
		}
		
		if (randf(0,1)<MR) {
			//jiggle weight
			int rc= randi(0, CONNS);
			abox->w[rc]+= randn(0, MR2);
//		  a2.mutations.push_back("weight jiggled\n");
//			abox->seed= 0;
		}
	}
}

void MLPBrain::liveMutate(float MR, float MR2, vector<float>& out)
{
	//for mutations which may occur while the bot is live
	int j= randi(0,BRAINSIZE);
	MLPBox* abox= &boxes[j];

	if (randf(0,1)<MR/30) {
		//"neurons that fire together, wire together"
		int rc= randi(0, CONNS);
		int b= -1;
		while (b<=-1){
			b-= 1;
			int rb= randi(0,BRAINSIZE);
			if (abs(boxes[rb].oldout-abox->out)<=0.01) b= rb;
			if (b<=-100) break;
		}
		if (b>=0){
			abox->id[rc]= b;
	//		  a2.mutations.push_back("connection Hebb'ed\n");
			abox->seed= 0;
		}
	}

	if (randf(0,1)<MR/20) {
		//stimulate box weight
		float stim= out[Output::STIMULANT];
		if(stim>0.5){
			for (int k=0;k<CONNS;k++) {
				//modify weights based on matching old output and new input, if stimulant is active
				float val= boxes[abox->id[k]].out;
				abox->w[k]+= conf::LEARNRATE*stim*(abox->oldout-(1-val));
			}
		}
//		a2.mutations.push_back("weight stimulated\n");
//		abox->seed= 0;
	}

	if (randf(0,1)<MR/10) {
		//jiggle bias
		abox->bias+= randn(0, MR2);
//		 a2.mutations.push_back("bias jiggled\n");
//		abox->seed= 0;
	}

	if (randf(0,1)<MR/10) {
		//jiggle dampening
		abox->kp+= randn(0, MR2);
		if (abox->kp<0.01) abox->kp=0.01;
		if (abox->kp>1) abox->kp=1;
//		 a2.mutations.push_back("kp jiggled\n");
//		abox->seed= 0;
	}
}

MLPBrain MLPBrain::crossover(const MLPBrain& other)
{
	MLPBrain newbrain(*this);
	
	#pragma omp parallel for
	for (int i=0; i<(int)newbrain.boxes.size(); i++){
		int s1= this->boxes[i].seed;
		int s2= other.boxes[i].seed;
		//function which offers pobability of which parent to use, based on relative seed counters
		float threshold= ((s1-s2)/(1+abs(s1-s2))+1)/2;

		if(randf(0,1)<threshold){
			newbrain.boxes[i].bias= this->boxes[i].bias;
			newbrain.boxes[i].gw= this->boxes[i].gw;
			newbrain.boxes[i].kp= this->boxes[i].kp;
			newbrain.boxes[i].seed= this->boxes[i].seed + 1;
//			this->boxes[i].seed += 1; //reward the copied box
			for (int j=0; j<CONNS; j++){
				newbrain.boxes[i].id[j] = this->boxes[i].id[j];
				newbrain.boxes[i].w[j] = this->boxes[i].w[j];
				newbrain.boxes[i].type[j] = this->boxes[i].type[j];
			}
		
		} else {
			newbrain.boxes[i].bias= other.boxes[i].bias;
			newbrain.boxes[i].gw= other.boxes[i].gw;
			newbrain.boxes[i].kp= other.boxes[i].kp;
			newbrain.boxes[i].seed= other.boxes[i].seed + 1;
//			other.boxes[i].seed += 1;
			for (int j=0; j<CONNS; j++){
				newbrain.boxes[i].id[j] = other.boxes[i].id[j];
				newbrain.boxes[i].w[j] = other.boxes[i].w[j];
				newbrain.boxes[i].type[j] = other.boxes[i].type[j];
			}
		}
	}
	return newbrain;
}

