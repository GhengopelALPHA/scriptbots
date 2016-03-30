#include "MLPBrain.h"
using namespace std;


MLPBox::MLPBox()
{

	w.resize(CONNS,0);
	id.resize(CONNS,0);
	type.resize(CONNS,0);

	//constructor
	for (int i=0;i<CONNS;i++) {
		w[i]= randf(-2,2); //caution: tuning this to beyond -2,2 will cause bots to be "slow" and unresponsive. -1.5,1.5 may be good
		if(randf(0,1)<0.1) w[i]= -1;
		if(randf(0,1)<0.1) w[i]= 1; //make equal valued connections more frequent
		if(randf(0,1)<0.3) w[i]=0; //make brains sparse
		
		id[i]= randi(0,BRAINSIZE);
		if (randf(0,1)<0.4) id[i]= randi(0,INPUTSIZE); //40% of the brain AT LEAST should connect to input.
		
		type[i] = 0;
		if(randf(0,1)<0.1) type[i] = 1; //make 10% be change sensitive synapses
		if(randf(0,1)<0.05) type[i] = 2; //make 5% be memory synapses
	}

	kp=randf(0.01,1);
	gw= randf(0,3);
	bias= randf(-1,1);

	out=0;
	oldout=0;
	target=0;
}

MLPBrain::MLPBrain()
{

	//constructor
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
	for (int i=0;i<BRAINSIZE;i++) {
		MLPBox* abox= &boxes[i];
		
		if (i<INPUTSIZE) { //take first few boxes and set their out to in[]. (no need to do these separately, since thay are first)
			abox->out= in[i];
		} else { //then do a dynamics tick and set all targets
			float acc=abox->bias;

			for (int j=0;j<CONNS;j++) {
				int idx=abox->id[j];
				int type = abox->type[j];
				float val= boxes[idx].out;

				if(type==2){
					if(val>0.5){
						break;
						continue;
					}
					continue;
				}
				
				if(type==1){
					val-= boxes[idx].oldout;
					val*=10;
				}

				if (j==CONNS-1) acc+= val*abox->w[j]*(out[9]*2-1);//last connection is affected by to the 10th output, choice
				else acc+= val*abox->w[j];
			}
			
			acc*= abox->gw;
			
			//put through sigmoid
			acc= 1.0/(1.0+exp(-acc));
			
			abox->target= cap(acc);
		}
	}
	

	for (int i=0;i<BRAINSIZE;i++) {
		MLPBox* abox= &boxes[i];

		//back up current out for each box
		abox->oldout = abox->out;

		//make all boxes go a bit toward target
		if (i>=INPUTSIZE) abox->out+= (abox->target-abox->out)*abox->kp;
	}

	//finally set out[] to the last few boxes output
	for (int i=0;i<OUTPUTSIZE;i++) {
		out[i]= boxes[BRAINSIZE-1-i].out;
	}
}

float MLPBrain::getActivity()
{
	float sum= 0;
	for (int j=0;j<BRAINSIZE;j++) {
		MLPBox* abox= &boxes[j];
		sum+= abs(abox->out - abox->oldout);
	}
	return sum/BRAINSIZE;
}

void MLPBrain::initMutate(float MR, float MR2)
{
	//for mutations which may occur at conception
	for (int j=0;j<BRAINSIZE;j++) {
		MLPBox* abox= &boxes[j];
		if (randf(0,1)<MR/10) {
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
				continue; //cancel all further mutations to this box
			}
		}

		if (randf(0,1)<MR/2) {
			//randomize synapse type
			int rc= randi(0, CONNS);
			abox->type[rc] = randi(0,2);
//		  a2.mutations.push_back("synapse type randomized\n");
			continue; //cancel all further mutations to this box
		}

		if (randf(0,1)<MR/2) {
			//randomize connection
			int rc= randi(0, CONNS);
			int ri= randi(0,BRAINSIZE);
			abox->id[rc]= ri;
//		  a2.mutations.push_back("connection randomized\n");
			continue; //cancel all further mutations to this box
		}

		// more likely changes here
		if (randf(0,1)<MR*2) {
			int rc1= randi(0, CONNS);
			int rc2= randi(0, CONNS);
			int temp= abox->id[rc1];
			abox->id[rc1]= abox->id[rc2];
			abox->id[rc2]= temp;
//			 a2.mutations.push_back("inputs swapped\n");
		}

		if (randf(0,1)<MR*5) {
			//jiggle bias
			abox->bias+= randn(0, MR2);
//			 a2.mutations.push_back("bias jiggled\n");
		}

		if (randf(0,1)<MR*2) {
			abox->kp+= randn(0, MR2);
			if (abox->kp<0.01) abox->kp=0.01;
			if (abox->kp>1) abox->kp=1;
//			 a2.mutations.push_back("kp jiggled\n");
		}
		
		if (randf(0,1)<MR) {
			abox->gw+= randn(0, MR2);
			if (abox->gw<0) abox->gw=0;
//			 a2.mutations.push_back("global weight jiggled\n");
		}

		if (randf(0,1)<MR*5) {
			int rc= randi(0, CONNS);
			abox->w[rc]+= randn(0, MR2);
//		  a2.mutations.push_back("weight jiggled\n");
		}
	}
}

void MLPBrain::liveMutate(float MR, float MR2, vector<float>& out)
{
	//for mutations which may occur while the bot is live
	int j= randi(0,BRAINSIZE);
	MLPBox* abox= &boxes[j];

	if (randf(0,1)<MR*2) {
		//"neurons that fire together, wire together"
		int rc= randi(0, CONNS);
		int b= -1;
		while (b==-1){
			int rb= randi(0,BRAINSIZE);
			if (boxes[rb].out-abox->out>=-0.1 && boxes[rb].out-abox->out<=0.1) b= rb;
		}
		abox->id[rc]= b;
//		  a2.mutations.push_back("connection Hebb'ed\n");
	}

	if (randf(0,1)<MR*2) {
		//stimulate box weight
		float stim= out[10];
		for (int k=0;k<CONNS;k++) {
			//modify weights based on matching old output and new input, if stimulant is active
			float val= boxes[abox->id[k]].out;
			abox->w[k]+= conf::LEARNRATE*stim*(abox->oldout-(1-val));
		}
//		a2.mutations.push_back("weight stimulated\n");
	}

	if (randf(0,1)<MR*2) {
		//jiggle bias
		abox->bias+= randn(0, MR2);
//		 a2.mutations.push_back("bias jiggled\n");
	}

	if (randf(0,1)<MR*2) {
		//jiggle dampening
		abox->kp+= randn(0, MR2);
		if (abox->kp<0.01) abox->kp=0.01;
		if (abox->kp>1) abox->kp=1;
//		 a2.mutations.push_back("kp jiggled\n");
	}
}

MLPBrain MLPBrain::crossover(const MLPBrain& other)
{
	MLPBrain newbrain(*this);
	
	for (int i=0;i<newbrain.boxes.size(); i++) {
		if(randf(0,1)<0.5){
			newbrain.boxes[i].bias= this->boxes[i].bias;
			newbrain.boxes[i].gw= this->boxes[i].gw;
			newbrain.boxes[i].kp= this->boxes[i].kp;
			for (int j=0;j<newbrain.boxes[i].id.size();j++) {
				newbrain.boxes[i].id[j] = this->boxes[i].id[j];
				newbrain.boxes[i].w[j] = this->boxes[i].w[j];
				newbrain.boxes[i].type[j] = this->boxes[i].type[j];
			}
		
		} else {
			newbrain.boxes[i].bias= other.boxes[i].bias;
			newbrain.boxes[i].gw= other.boxes[i].gw;
			newbrain.boxes[i].kp= other.boxes[i].kp;
			for (int j=0;j<newbrain.boxes[i].id.size();j++) {
				newbrain.boxes[i].id[j] = other.boxes[i].id[j];
				newbrain.boxes[i].w[j] = other.boxes[i].w[j];
				newbrain.boxes[i].type[j] = other.boxes[i].type[j];
			}
		}
	}
	return newbrain;
}

