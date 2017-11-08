#include <fitsio.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <sched.h>
#include <ncurses.h>
#include <semaphore.h>


#ifdef __MACH__
#include <mach/mach_time.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
int clock_gettime(int clk_id, struct mach_timespec *t){
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t time;
    time = mach_absolute_time();
    double nseconds = ((double)time * (double)timebase.numer)/((double)timebase.denom);
    double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
    t->tv_sec = seconds;
    t->tv_nsec = nseconds;
    return 0;
}
#else
#include <time.h>
#endif





#include "CommandLineInterface/CLIcore.h"
#include "00CORE/00CORE.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_iofits/COREMOD_iofits.h"


#include "SCExAO_DM/SCExAO_DM.h"

extern DATA data;

int wcol, wrow; // window size


struct timespec semwaitts;


#define DMSTROKE100 0.7 // um displacement for 100V
#define NBact 2500 // default


SCEXAO_DISPCOMB_CONF *dispcombconf; // configuration
int dmdispcomb_loaded = 0;
int SMfd;


SCEXAO_DMTURBCONF *dmturbconf; // DM turbulence configuration
int dmturb_loaded = 0;
int SMturbfd;
long IDturb;




// CLI commands
//
// function CLI_checkarg used to check arguments
// 1: float
// 2: long
// 3: string
// 4: existing image
//



int_fast8_t SCExAO_DM_dmturb_wspeed_cli()
{
  if(CLI_checkarg(1,1)==0)
    SCExAO_DM_dmturb_wspeed(data.cmdargtoken[1].val.numf);
  else
    return 1;
}


int_fast8_t SCExAO_DM_dmturb_ampl_cli()
{
  if(CLI_checkarg(1,1)==0)
    SCExAO_DM_dmturb_ampl(data.cmdargtoken[1].val.numf);
  else
    return 1;
}


int_fast8_t SCExAO_DM_dmturb_LOcoeff_cli()
{
  if(CLI_checkarg(1,1)==0)
    SCExAO_DM_dmturb_LOcoeff(data.cmdargtoken[1].val.numf);
  else
    return 1;
}


int_fast8_t SCExAO_DM_dmturb_tint_cli()
{
  if(CLI_checkarg(1,2)==0)
    SCExAO_DM_dmturb_tint(data.cmdargtoken[1].val.numl);
  else
    return 1;
}




void __attribute__ ((constructor)) libinit_SCExAO_DM()
{
	init_SCExAO_DM();
	printf(" ...... Loading module %s\n", __FILE__);
}



int init_SCExAO_DM()
{
  strcpy(data.module[data.NBmodule].name, __FILE__);
  strcpy(data.module[data.NBmodule].info, "SCExAO DM operation");
  data.NBmodule++;





  strcpy(data.cmd[data.NBcmd].key,"scexaoDMturb");
  strcpy(data.cmd[data.NBcmd].module,__FILE__);
  data.cmd[data.NBcmd].fp = SCExAO_DM_turb;
  strcpy(data.cmd[data.NBcmd].info,"DM turbulence");
  strcpy(data.cmd[data.NBcmd].syntax,"no arg");
  strcpy(data.cmd[data.NBcmd].example,"scexaoDMturb");
  strcpy(data.cmd[data.NBcmd].Ccall,"int SCExAO_DM_turb()");
  data.NBcmd++;

  strcpy(data.cmd[data.NBcmd].key,"scexaodmturboff");
  strcpy(data.cmd[data.NBcmd].module,__FILE__);
  data.cmd[data.NBcmd].fp =  SCExAO_DM_dmturboff;
  strcpy(data.cmd[data.NBcmd].info,"turn off DM turbulence");
  strcpy(data.cmd[data.NBcmd].syntax,"no arg");
  strcpy(data.cmd[data.NBcmd].example,"scexaodmturboff");
  strcpy(data.cmd[data.NBcmd].Ccall,"int SCExAO_DM_dmturboff()");
  data.NBcmd++;
  
  strcpy(data.cmd[data.NBcmd].key,"scexaodmturws");
  strcpy(data.cmd[data.NBcmd].module,__FILE__);
  data.cmd[data.NBcmd].fp = SCExAO_DM_dmturb_wspeed_cli;
  strcpy(data.cmd[data.NBcmd].info,"set turbulence wind speed");
  strcpy(data.cmd[data.NBcmd].syntax,"<wind speed [m/s]>");
  strcpy(data.cmd[data.NBcmd].example,"scexaodmturws 5.2");
  strcpy(data.cmd[data.NBcmd].Ccall,"int SCExAO_DM_dmturb_wspeed(double wspeed);");
  data.NBcmd++;

  strcpy(data.cmd[data.NBcmd].key,"scexaodmturampl");
  strcpy(data.cmd[data.NBcmd].module,__FILE__);
  data.cmd[data.NBcmd].fp = SCExAO_DM_dmturb_ampl_cli;
  strcpy(data.cmd[data.NBcmd].info,"set turbulence amplitude");
  strcpy(data.cmd[data.NBcmd].syntax,"<amplitude [um]>");
  strcpy(data.cmd[data.NBcmd].example,"scexaodmturampl 0.1");
  strcpy(data.cmd[data.NBcmd].Ccall,"int SCExAO_DM_dmturb_ampl(double ampl);");
  data.NBcmd++;

  strcpy(data.cmd[data.NBcmd].key,"scexaodmturlo");
  strcpy(data.cmd[data.NBcmd].module,__FILE__);
  data.cmd[data.NBcmd].fp = SCExAO_DM_dmturb_LOcoeff_cli;
  strcpy(data.cmd[data.NBcmd].info,"set turbulence low order coefficient");
  strcpy(data.cmd[data.NBcmd].syntax,"<coeff>");
  strcpy(data.cmd[data.NBcmd].example,"scexaodmturlo 0.2");
  strcpy(data.cmd[data.NBcmd].Ccall,"int SCExAO_DM_dmturb_LOcoeff(double LOcoeff);");
  data.NBcmd++;

  strcpy(data.cmd[data.NBcmd].key,"scexaodmturtint");
  strcpy(data.cmd[data.NBcmd].module,__FILE__);
  data.cmd[data.NBcmd].fp = SCExAO_DM_dmturb_tint_cli;
  strcpy(data.cmd[data.NBcmd].info,"set turbulence interval time");
  strcpy(data.cmd[data.NBcmd].syntax,"<interval time [us] long>");
  strcpy(data.cmd[data.NBcmd].example,"scexaodmturtint 200");
  strcpy(data.cmd[data.NBcmd].Ccall,"int SCExAO_DM_turb_tint(long tint);");
  data.NBcmd++;

  



  // add atexit functions here

  
  return 0;
}







struct timespec time_diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}









int_fast8_t SCEXAO_DM_createconf()
{
    if( dmdispcomb_loaded == 0 )
    {
	    int result;
		
        printf("Create/read configuration\n");

        SMfd = open(DISPCOMB_FILENAME_CONF, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
        if (SMfd == -1) {
            perror("Error opening file for writing");
            exit(EXIT_FAILURE);
        }

        result = lseek(SMfd, sizeof(SCEXAO_DISPCOMB_CONF)-1, SEEK_SET);
        if (result == -1) {
            close(SMfd);
            perror("Error calling lseek() to 'stretch' the file");
            exit(EXIT_FAILURE);
        }

        result = write(SMfd, "", 1);
        if (result != 1) {
            close(SMfd);
            perror("Error writing last byte of the file");
            exit(EXIT_FAILURE);
        }

        dispcombconf = (SCEXAO_DISPCOMB_CONF*)mmap(0, sizeof(SCEXAO_DISPCOMB_CONF), PROT_READ | PROT_WRITE, MAP_SHARED, SMfd, 0);
        if (dispcombconf == MAP_FAILED) {
            close(SMfd);
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
        }


        dispcombconf[0].ON = 1;
        dispcombconf[0].busy = 0;
        dispcombconf[0].MAXVOLT = 150.0;
        dispcombconf[0].moninterval = 30000; // 33Hz
        dispcombconf[0].status = 0;

        dmdispcomb_loaded = 1;

    }

    return 0;
}






























int_fast8_t SCEXAO_DMturb_createconf()
{

  if( dmturb_loaded == 0 ) 
    {
	long IDc1;
	  int result;
	  
	  	
      printf("Create/read configuration\n");  
      fflush(stdout);

      IDc1 = image_ID("dmdisp1");
      if(IDc1 == -1)
	read_sharedmem_image("dmdisp1");
      
      IDturb = create_2Dimage_ID("turbs", 50, 50);

 
      SMturbfd = open(DMTURBCONF_FILENAME, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
      if (SMturbfd == -1) {
	perror("Error opening file for writing");
	exit(EXIT_FAILURE);
      }
      
      result = lseek(SMturbfd, sizeof(SCEXAO_DMTURBCONF)-1, SEEK_SET);
      if (result == -1) {
	close(SMturbfd);
	perror("Error calling lseek() to 'stretch' the file");
	exit(EXIT_FAILURE);
      }
      
      result = write(SMturbfd, "", 1);
      if (result != 1) {
	close(SMturbfd);
	perror("Error writing last byte of the file");
	exit(EXIT_FAILURE);
      }
      
      dmturbconf = (SCEXAO_DMTURBCONF*)mmap(0, sizeof(SCEXAO_DMTURBCONF), PROT_READ | PROT_WRITE, MAP_SHARED, SMturbfd, 0);
      if (dmturbconf == MAP_FAILED) {
	close(SMturbfd);
	perror("Error mmapping the file");
	exit(EXIT_FAILURE);
      }
      
      dmturbconf[0].on = 1;
      dmturbconf[0].ampl = 0.01; // [um]
      dmturbconf[0].tint = 100; // [us]
      dmturbconf[0].simtime = 0.0; // sec
      dmturbconf[0].wspeed = 10.0; // [m/s]
      dmturbconf[0].LOcoeff = 0.2;
            
      dmturb_loaded = 1;
 
    }

  return 0;
}




int_fast8_t SCEXAO_DMturb_loadconf()
{
  int result;

  if( dmturb_loaded == 0 ) 
    {
      printf("Create/read configuration\n");  
      
      SMturbfd = open(DMTURBCONF_FILENAME, O_RDWR, (mode_t)0600);
      if (SMturbfd == -1) {
	perror("Error opening file for writing");
	exit(EXIT_FAILURE);
      }      

      dmturbconf = (SCEXAO_DMTURBCONF*)mmap(0, sizeof(SCEXAO_DMTURBCONF), PROT_READ | PROT_WRITE, MAP_SHARED, SMturbfd, 0);
      if (dmturbconf == MAP_FAILED) {
	close(SMturbfd);
	perror("Error mmapping the file");
	exit(EXIT_FAILURE);
      }
      dmturb_loaded = 1; 
    }

  return 0;
}




int_fast8_t SCExAO_DM_dmturboff()
{
  SCEXAO_DMturb_loadconf();
  dmturbconf[0].on = 0;
  SCExAO_DM_dmturb_printstatus();

  return 0;
}

int_fast8_t SCExAO_DM_dmturb_wspeed(double wspeed)
{
  SCEXAO_DMturb_loadconf();
  dmturbconf[0].wspeed = wspeed;
  SCExAO_DM_dmturb_printstatus();

  return 0;
}

int_fast8_t SCExAO_DM_dmturb_ampl(double ampl)
{
  SCEXAO_DMturb_loadconf();
  dmturbconf[0].ampl = ampl;
  SCExAO_DM_dmturb_printstatus();

  return 0;
}

int_fast8_t SCExAO_DM_dmturb_LOcoeff(double LOcoeff)
{
  SCEXAO_DMturb_loadconf();
  dmturbconf[0].LOcoeff = LOcoeff;
  SCExAO_DM_dmturb_printstatus();

  return 0;
}

int_fast8_t SCExAO_DM_dmturb_tint(long tint)
{
  SCEXAO_DMturb_loadconf();
  dmturbconf[0].tint = tint;
  SCExAO_DM_dmturb_printstatus();

  return 0;
}



int_fast8_t SCExAO_DM_dmturb_printstatus()
{
  SCEXAO_DMturb_loadconf();

  printf("Run time = %.3f sec\n", dmturbconf[0].simtime);
  printf("\n");
  printf("cnt              : %ld   (ave frequ = %.2f kHz)\n", dmturbconf[0].cnt, 0.001*dmturbconf[0].cnt/dmturbconf[0].simtime);
  printf("\n");

  if(dmturbconf[0].on == 1)
    printf("LOOP IS ON\n");
  else 
    printf("LOOP IS OFF\n");

  printf("ampl    =  %.2f um\n", dmturbconf[0].ampl);
  printf("wspeed  =  %.2f m/s\n", dmturbconf[0].wspeed);
  printf("tint    =  %ld us\n", dmturbconf[0].tint);
  printf("Requested uptdate frequ = %.2f kHz\n", 0.001/(1.0e-6*dmturbconf[0].tint));
  printf("\n");
  printf("\n");

  return(0);
} 





int_fast8_t SCExAO_DM_turb()
{
  long size_sx; // screen size
  long size_sy;
  long IDs1, IDs2;
  
  struct timespec tlast;  
  struct timespec tdiff;
  struct timespec tdiff1;

  float screen0_X = 0.0;
  float screen0_Y = 0.0;
  long ii, jj, ii1;
  float x, y;
  float xpix, ypix;
  float xpixf, ypixf;
  long xpix1, xpix2, ypix1, ypix2;

  double angle = 1.0;
  double coeff = 0.001;

  double r;

  float pixscale = 0.1; // [m/pix]
  // Subaru pupil ~ 80 pix diam
  // Single actuator ~7 pix 


  SCEXAO_DMturb_createconf();
 
  IDs1 = load_fits("/home/scexao/src/DMcontrol/dm_turb/turbscreen0.fits", "screen1", 1);
  IDs2 = load_fits("/home/scexao/src/DMcontrol/dm_turb/turbscreen0g.fits", "screen2", 1); 
  

  printf("ARRAY SIZE = %ld %ld\n", (long) data.image[IDs1].md[0].size[0], (long) data.image[IDs1].md[0].size[1]);
  size_sx = data.image[IDs1].md[0].size[0];
  size_sy = data.image[IDs1].md[0].size[1];

  clock_gettime(CLOCK_REALTIME, &dmturbconf[0].tstart);
  dmturbconf[0].tend = dmturbconf[0].tstart;



  while(dmturbconf[0].on == 1) // computation loop
    {      
		double tdiff1v;
		float ave;
		  double RMSval;
		  long RMSvalcnt;
		  
		  
      usleep(dmturbconf[0].tint);
 
      tlast = dmturbconf[0].tend;
      clock_gettime(CLOCK_REALTIME, &dmturbconf[0].tend);
      tdiff = time_diff(dmturbconf[0].tstart, dmturbconf[0].tend);
      tdiff1 =  time_diff(tlast, dmturbconf[0].tend);
      tdiff1v = 1.0*tdiff1.tv_sec + 1.0e-9*tdiff1.tv_nsec;
     
      screen0_X += dmturbconf[0].wspeed*tdiff1v*cos(angle); // [m]
      screen0_Y += dmturbconf[0].wspeed*tdiff1v*sin(angle); // [m]
      

      dmturbconf[0].simtime = 1.0*tdiff.tv_sec + 1.0e-9*tdiff.tv_nsec;


      for(ii=0;ii<50;ii++)
	for(jj=0;jj<50;jj++)
	  {
	    ii1 = jj*50+ii;
	    
	    x = 10.0*ii/50.0 + screen0_X; // [m]
	    y = 10.0*jj/50.0 + screen0_Y; // [m]
	    
	    xpix = 0.5*size_sx + x/pixscale;
	    ypix = 0.5*size_sy + y/pixscale;
	    
	    xpix1 = ((long) xpix)%size_sx;
	    xpix2 = (xpix1+1)%size_sx;
	    xpixf = xpix- (long) xpix;
	    ypix1 = ((long) ypix)%size_sy;
	    ypix2 = (ypix1+1)%size_sy;
	    ypixf = ypix - (long) ypix;
	    
	    if(xpix1<0)
	      xpix1 = 0;
	    if(xpix1>size_sx-1)
	      xpix1 = size_sx-1;
	    
	    if(ypix1<0)
	      ypix1 = 0;
	    if(ypix1>size_sy-1)
	      ypix1 = size_sy-1;
	    
	    
	    
	    data.image[IDturb].array.F[ii1] = (1.0-xpixf)*(1.0-ypixf)*(data.image[IDs1].array.F[ypix1*size_sx+xpix1]-(1.0-dmturbconf[0].LOcoeff)*data.image[IDs2].array.F[ypix1*size_sx+xpix1]);
	    
	    data.image[IDturb].array.F[ii1]  +=  (xpixf)*(1.0-ypixf)*(data.image[IDs1].array.F[ypix1*size_sx+xpix2]-(1.0-dmturbconf[0].LOcoeff)*data.image[IDs2].array.F[ypix1*size_sx+xpix2]);
	    
	    data.image[IDturb].array.F[ii1]  += (1.0-xpixf)*(ypixf)*(data.image[IDs1].array.F[ypix2*size_sx+xpix1]-(1.0-dmturbconf[0].LOcoeff)*data.image[IDs2].array.F[ypix2*size_sx+xpix1]);
	    
	    data.image[IDturb].array.F[ii1]  += xpixf*ypixf*(data.image[IDs1].array.F[ypix2*size_sx+xpix2]-(1.0-dmturbconf[0].LOcoeff)*data.image[IDs2].array.F[ypix2*size_sx+xpix2]);
	  }
      
      // proccess array
      ave = 0.0;
      for(ii1=0;ii1<50*50;ii1++)	    
	ave += data.image[IDturb].array.F[ii1];
      ave /= 2500;
      for(ii1=0;ii1<50*50;ii1++)	    
	  {
	    data.image[IDturb].array.F[ii1] -= ave;
	  data.image[IDturb].array.F[ii1] *= coeff;
	  }
      
      RMSval = 0.0;
      RMSvalcnt = 0;
      
      for(ii=0;ii<50;ii++)	    
	for(jj=0;jj<50;jj++)
	  {
	    ii1 = 50*jj+ii;
	    x = 24.5 - ii;
	    y = 24.5 - jj;
	    r = sqrt(x*x+y*y);
	    if(r<24.0)
	      {
		RMSval += data.image[IDturb].array.F[ii1]*data.image[IDturb].array.F[ii1];
		RMSvalcnt++;
	      }
	  }
      RMSval = sqrt(RMSval/RMSvalcnt);
      
      if(RMSval>dmturbconf[0].ampl)
	coeff *= 0.999;
      else
	coeff *= 1.001;
      
      copy_image_ID("turbs", "dmdisp1", 1);
    }
  

  return(0);
}
