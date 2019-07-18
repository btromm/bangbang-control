// _  _ ____ _    ____ ___ _    
//  \/  |  | |    |  |  |  |    
// _/\_ |__| |___ |__|  |  |___ 
//
// Bang Bang controller, controls mRNA
// This controller can control either a synapse
// or a conductance 

#ifndef BANGBANGCONTROLLER_MRNA
#define BANGBANGCONTROLLER_MRNA
#include "mechanism.hpp"
#include <limits>

//inherit controller class spec
class BangBangController_mRNA: public mechanism {

protected:
    // flag used to switch between
    // controlling channels and synapses
    // meaning:
    // 0 --- unset, will throw an error
    // 1 --- channels
    // 2 --- synapses
    int control_type = 0;
public:
    // timescales
    double tau_m = std::numeric_limits<double>::infinity();
    double tau_g = 5e3; 

    // mRNA concentration 
    double m = 0;

    // area of the container this is in
    double container_A;

    // specify parameters + initial conditions for 
    // mechanism that controls a conductance 
    BangBangController_mRNA(double tau_m_, double tau_g_, double m_)
    {

        tau_m = tau_m_;
        tau_g = tau_g_;
        m = m_;


        // if (tau_m<=0) {mexErrMsgTxt("[IntegralController] tau_m must be > 0. Perhaps you meant to set it to Inf?\n");}
        if (tau_g<=0) {mexErrMsgTxt("[BangBangController_mRNA] tau_g must be > 0. Perhaps you meant to set it to Inf?\n");}
    }

    
    void integrate(void);

    void checkSolvers(int);

    void connect(conductance *);
    void connect(synapse*);
    void connect(compartment*);

    int getFullStateSize(void);
    int getFullState(double * cont_state, int idx);
    double getState(int);

};


double BangBangController_mRNA::getState(int idx)
{
    if (idx == 1) {return m;}
    else if (idx == 2) {return channel->gbar;}
    else {return std::numeric_limits<double>::quiet_NaN();}

}


int BangBangController_mRNA::getFullStateSize(){return 2; }


int BangBangController_mRNA::getFullState(double *cont_state, int idx)
{
    // give it the current mRNA level
    cont_state[idx] = m;

    idx++;

    // and also output the current gbar of the thing
    // being controller
    if (channel)
    {
      cont_state[idx] = channel->gbar;  
    }
    else if (syn)
    {
        cont_state[idx] = syn->gmax;  
    }
    idx++;
    return idx;
}


void BangBangController_mRNA::connect(conductance * channel_)
{

    // connect to a channel
    channel = channel_;


    // make sure the compartment that we are in knows about us
    (channel->container)->addMechanism(this);



    controlling_class = (channel_->getClass()).c_str();

    // attempt to read the area of the container that this
    // controller should be in. 
    container_A  = (channel->container)->A;

    control_type = 1;


}

void BangBangController_mRNA::connect(compartment* comp_)
{
    mexErrMsgTxt("[BangBangController_mRNA] This mechanism cannot connect to a compartment object");
}

void BangBangController_mRNA::connect(synapse* syn_)
{

    // connect to a synpase
    syn = syn_;


    // make sure the compartment that we are in knows about us
    (syn->post_syn)->addMechanism(this);


    // attempt to read the area of the container that this
    // controller should be in. 
    container_A  = (syn->post_syn)->A;

    control_type = 2;

}


void BangBangController_mRNA::integrate(void)
{


    switch (control_type)
    {
        case 0:
            mexErrMsgTxt("[BangBangController_mRNA] misconfigured controller. Make sure this object is contained by a conductance or synapse object");
            break;


        case 1:

            {
            // if the target is NaN, we will interpret this
            // as the controller being disabled 
            // and do nothing 
            if (isnan((channel->container)->Ca_target)) {return;}

            double Ca_error = (channel->container)->Ca_target - (channel->container)->Ca_prev;

            // integrate mRNA
            if(Ca_error > 0) {m += dt/tau_m;}
            else if(Ca_error < 0) {m -= dt/tau_m;}
            
            // mRNA levels below zero don't make any sense
            if (m < 0) {m = 0;}

            // copy the protein levels from this channel
            double gdot = ((dt/tau_g)*(m - channel->gbar*container_A));

            // make sure it doesn't go below zero
            if (channel->gbar_next + gdot < 0) {
                channel->gbar_next = 0;
            } else {
                channel->gbar_next += gdot/container_A;
            }


            break;

            }
        case 2:
            {
            // if the target is NaN, we will interpret this
            // as the controller being disabled 
            // and do nothing 

            if (isnan((syn->post_syn)->Ca_target)) {return;}

            double Ca_error = (syn->post_syn)->Ca_target - (syn->post_syn)->Ca_prev;

            // integrate mRNA
            m += (dt/tau_m)*(Ca_error);

            // mRNA levels below zero don't make any sense
            if (m < 0) {m = 0;}

            // copy the protein levels from this syn
            double gdot = ((dt/tau_g)*(m - syn->gmax*1e-3));

            // make sure it doesn't go below zero
            if (syn->gmax + gdot*1e3 < 0) {
                syn->gmax = 0;
            } else {
                syn->gmax += gdot*1e3;
            }


            break;

            }

        default:
            mexErrMsgTxt("[BangBangController_mRNA] misconfigured controller");
            break;

    }


}



void BangBangController_mRNA::checkSolvers(int k) {
    if (k == 0){
        return;
    } else {
        mexErrMsgTxt("[BangBangController_mRNA] unsupported solver order\n");
    }
}




#endif
