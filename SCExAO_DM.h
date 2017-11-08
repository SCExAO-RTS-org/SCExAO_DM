#ifndef _AOSYSTSIM_H
#define _AOSYSTSIM_H



#define DISPCOMB_FILENAME_CONF "/tmp/dmdispcombconf.conf.shm"
#define DMTURBCONF_FILENAME "/tmp/dmturb.conf.shm"



typedef struct
{
  int ON;

  long loopcnt;
  long updatecnt;
  
  int busy; // if set to 1, hold off and wait
  float MAXVOLT; // maximum voltage on DM

  int status;

  struct timespec tstart;
  struct timespec tend;

  long moninterval; // [us]  
} SCEXAO_DISPCOMB_CONF;



typedef struct
{
  int on;
  long cnt;


  double wspeed; // wind speed [m/s]
  double ampl; // [um RMS]
  double LOcoeff; // 0 for full correction of low orders, 1 for no correction

  long tint; // interval between consecutive DM updates [us]


  double simtime;

  struct timespec tstart;
  struct timespec tend;

} SCEXAO_DMTURBCONF;



int init_SCExAO_DM();

int_fast8_t SCEXAO_DM_createconf();
int_fast8_t SCEXAO_DM_loadconf();
int_fast8_t SCEXAO_DM_unloadconf();


int_fast8_t SCEXAO_DMturb_createconf();
int_fast8_t SCEXAO_DMturb_loadconf();
int_fast8_t SCExAO_DM_dmturboff();
int_fast8_t SCExAO_DM_dmturb_wspeed(double wspeed);
int_fast8_t SCExAO_DM_dmturb_ampl(double ampl);
int_fast8_t SCExAO_DM_dmturb_LOcoeff(double LOcoeff);
int_fast8_t SCExAO_DM_dmturb_tint(long tint);
int_fast8_t SCExAO_DM_dmturb_printstatus();
int_fast8_t SCExAO_DM_turb();


#endif
