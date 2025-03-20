#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

/*****************************************************************************/

#include "ladspa.h"


/*****************************************************************************/

#define SF_CHANNELS     0
#define SF_INPUT_CH0    1
#define SF_OUTPUT_CH0   2
#define SF_INPUT_CH1    3
#define SF_OUTPUT_CH1   4
#define SF_INPUT_CH2    5
#define SF_OUTPUT_CH2   6
#define SF_INPUT_CH3    7
#define SF_OUTPUT_CH3   8
#define SF_INPUT_CH4    9
#define SF_OUTPUT_CH4   10
#define SF_INPUT_CH5    11
#define SF_OUTPUT_CH5   12
#define SF_INPUT_CH6    13
#define SF_OUTPUT_CH6   14
#define SF_INPUT_CH7    15
#define SF_OUTPUT_CH7   16

#define DC_DEBUG_ 0

/*****************************************************************************/
/* Instance data for the DCBDoing filter. We can get away with using
   this structure for high-pass filters because the data
   stored is the same. Note that the actual run() calls differ
   however. */
typedef struct {

  unsigned int m_fSampleRate;
  
  /* Ports:------ */
  LADSPA_Data * m_pfChannels;
  
  LADSPA_Data * m_pfInput_ch0;
  LADSPA_Data * m_pfOutput_ch0;
  
  LADSPA_Data * m_pfInput_ch1;
  LADSPA_Data * m_pfOutput_ch1;

  LADSPA_Data * m_pfInput_ch2;
  LADSPA_Data * m_pfOutput_ch2;

  LADSPA_Data * m_pfInput_ch3;
  LADSPA_Data * m_pfOutput_ch3;

  LADSPA_Data * m_pfInput_ch4;
  LADSPA_Data * m_pfOutput_ch4;

  LADSPA_Data * m_pfInput_ch5;
  LADSPA_Data * m_pfOutput_ch5;

  LADSPA_Data * m_pfInput_ch6;
  LADSPA_Data * m_pfOutput_ch6;

  LADSPA_Data * m_pfInput_ch7;
  LADSPA_Data * m_pfOutput_ch7;

/********history info********/
    float xHist[8];
    float yHist[8];

/***********debug***********/
#ifdef DC_DEBUG_
    FILE *fp_in;
    FILE *fp_out;
  
#endif

} DCBFilter;

FILE *fp_txt = NULL;


/*****************************************************************************/
/*DCBlocking() only process for capture*/
#define IIR_COEFF 0.99951171875f //iir 1-order

void DCBlocking(float* pDataIn,float* pDataOut, unsigned long length, float *xHist, float *yHist) 
{
    float aCoeff = IIR_COEFF;
    float sum = 0.0;
    int ii = 0;
    float xHist1 = *xHist;
    float yHist1 = *yHist;
    
	for (ii = 0; ii < length; ii++) 
	{
	   // #ifdef DC_DEBUG_
        //fprintf(fp_txt,"len = %d,xHist = %f,yHist = %f\n",ii,xHist,yHist);
       // #endif
        
		sum = pDataIn[ii] - xHist1;
		sum = sum + aCoeff * yHist1;
		
        xHist1 = pDataIn[ii];
        yHist1 = sum;
        
		pDataOut[ii] = sum;
	}
	
	*xHist = xHist1;
	*yHist = yHist1;
}


/*****************************************************************************/

/* Construct a new plugin instance. In this case, as the DCBDoing Filter
   structure can be used for high-pass filters we can get away
   with only only writing one of these functions. Normally one would
   be required for each plugin type. */
LADSPA_Handle instantiateDCBFilter(const LADSPA_Descriptor * Descriptor,
			unsigned long             SampleRate) 
{

    DCBFilter * psFilter;

    psFilter = (DCBFilter *)malloc(sizeof(DCBFilter));

    if (psFilter) 
    {
        psFilter->m_fSampleRate = (unsigned int)SampleRate;  
    }

    printf("psFilter->m_fSampleRate = %d\n",psFilter->m_fSampleRate);
    return psFilter;
}
/*****************************************************************************/
            
/* Initialise and activate a plugin instance. Normally separate
    functions would have to be written for the different plugin types,
    however we can get away with a single function in this case. */
    void activateDCBFilter(LADSPA_Handle Instance)
    {
        DCBFilter * psDCBFilter;
        int ch = 0;
        psDCBFilter = (DCBFilter *)Instance;

        printf("init xHist and yHist.\n");
        for(ch = 0;ch < 8;ch ++)
        {
            psDCBFilter->xHist[ch] = 0;
            psDCBFilter->yHist[ch] = 0;
        }
        #ifdef DC_DEBUG_
        psDCBFilter->fp_in = fopen("/tmp/dc_in.pcm","wb");
        psDCBFilter->fp_out = fopen("/tmp/dc_out.pcm","wb");
        fp_txt = fopen("/tmp/hist.txt","wb");
        #endif
        
    }

/*****************************************************************************/
    
/* Connect a port to a data location. Normally separate functions
   would have to be written for the different plugin types, however we
   can get away with a single function in this case. */
void  connectPortToDCBFilter(LADSPA_Handle Instance,unsigned long Port,LADSPA_Data * DataLocation) 
{
      
    DCBFilter * psFilter; 
    psFilter = (DCBFilter *)Instance;

    switch (Port) {
        case SF_CHANNELS:
            psFilter->m_pfChannels = DataLocation;
            printf("channels = %f\n",*psFilter->m_pfChannels);
            if((*psFilter->m_pfChannels <= 0.0) ||(*psFilter->m_pfChannels > 8.0))
            {
                printf("LADSPA ERROR: DCBlocking unsupport %f channels.\n",*psFilter->m_pfChannels);
                exit(1);
            }
            break;
        case SF_INPUT_CH0:
           // printf("ch0.\n");
            psFilter->m_pfInput_ch0 = DataLocation;
            break;
        case SF_OUTPUT_CH0:
            psFilter->m_pfOutput_ch0 = DataLocation;
            break;
       case SF_INPUT_CH1:
            //printf("ch1.\n");
            psFilter->m_pfInput_ch1 = DataLocation;
            break;
        case SF_OUTPUT_CH1:
            psFilter->m_pfOutput_ch1 = DataLocation;
            break;
       case SF_INPUT_CH2:
            psFilter->m_pfInput_ch2 = DataLocation;
            break;
        case SF_OUTPUT_CH2:
            //if(*psFilter->m_pfChannels == 4.0)
                psFilter->m_pfOutput_ch2 = DataLocation;
            break;
        case SF_INPUT_CH3:
            //if(*psFilter->m_pfChannels == 4.0)
                psFilter->m_pfInput_ch3 = DataLocation;
            break;
        case SF_OUTPUT_CH3:
            //if(*psFilter->m_pfChannels == 4.0)
                psFilter->m_pfOutput_ch3 = DataLocation;
            break;
        case SF_INPUT_CH4:
            //if(*psFilter->m_pfChannels == 6.0)
                psFilter->m_pfInput_ch4 = DataLocation;
            break;
        case SF_OUTPUT_CH4:
           // if(*psFilter->m_pfChannels == 6.0)
                psFilter->m_pfOutput_ch4 = DataLocation;
            break;
        case SF_INPUT_CH5:
            //if(*psFilter->m_pfChannels == 6.0)
                psFilter->m_pfInput_ch5 = DataLocation;
            break;
        case SF_OUTPUT_CH5:
           // if(*psFilter->m_pfChannels == 6.0)
                psFilter->m_pfOutput_ch5 = DataLocation;
            break;
        case SF_INPUT_CH6:
           // if(*psFilter->m_pfChannels == 8.0)
                psFilter->m_pfInput_ch6 = DataLocation;
            break;
        case SF_OUTPUT_CH6:
            //if(*psFilter->m_pfChannels == 8.0)
                psFilter->m_pfOutput_ch6 = DataLocation;
            break;
        case SF_INPUT_CH7:
           // if(*psFilter->m_pfChannels == 8.0)
                psFilter->m_pfInput_ch7 = DataLocation;
            break;
        case SF_OUTPUT_CH7:
           // if(*psFilter->m_pfChannels == 8.0)
                psFilter->m_pfOutput_ch7 = DataLocation;
            break;
    }
}

/*****************************************************************************/

/* Run the HPF algorithm for a block of SampleCount samples. */
void runDCBHighPassFilter(LADSPA_Handle Instance,unsigned long SampleCount) 
{

     DCBFilter * psFilter;
     psFilter = (DCBFilter *)Instance;
     int SampleIndex = 0;
     int ChannelsIndex = 0;
     int ChannelsCount = (unsigned int)(*psFilter->m_pfChannels);

     if((psFilter->m_fSampleRate != 16000) && (psFilter->m_fSampleRate != 48000))
     {
        printf("LADSPA ERROR: DCBlocking unsupport %d samplerate.\n",psFilter->m_fSampleRate);
        //return ;
        exit(1);
     }

     if((ChannelsCount <= 0) || (ChannelsCount > 8))
     {
        printf("LADSPA ERROR: DCBlocking unsupport %d channels.\n",ChannelsCount);
        //return ;
        exit(1);
     }

    if(ChannelsCount == 2)
    {
        DCBlocking((float *)psFilter->m_pfInput_ch0,(float *)psFilter->m_pfOutput_ch0,SampleCount,&(psFilter->xHist[0]),&(psFilter->yHist[0]));
        DCBlocking((float *)psFilter->m_pfInput_ch1,(float *)psFilter->m_pfOutput_ch1,SampleCount,&(psFilter->xHist[1]),&(psFilter->yHist[1]));
    }
    if(ChannelsCount == 4)
    {
       // printf(".......\n");
        DCBlocking((float *)psFilter->m_pfInput_ch2,(float *)psFilter->m_pfOutput_ch2,SampleCount,&(psFilter->xHist[2]),&(psFilter->yHist[2]));
        DCBlocking((float *)psFilter->m_pfInput_ch3,(float *)psFilter->m_pfOutput_ch3,SampleCount,&(psFilter->xHist[3]),&(psFilter->yHist[3]));
    }
    if(ChannelsCount == 6)
    {
        DCBlocking((float *)psFilter->m_pfInput_ch4,(float *)psFilter->m_pfOutput_ch4,SampleCount,&(psFilter->xHist[4]),&(psFilter->yHist[4]));
        DCBlocking((float *)psFilter->m_pfInput_ch5,(float *)psFilter->m_pfOutput_ch5,SampleCount,&(psFilter->xHist[5]),&(psFilter->yHist[5]));
    }
    if(ChannelsCount == 8)
    {
        DCBlocking((float *)psFilter->m_pfInput_ch6,(float *)psFilter->m_pfOutput_ch6,SampleCount,&(psFilter->xHist[6]),&(psFilter->yHist[6]));
        DCBlocking((float *)psFilter->m_pfInput_ch7,(float *)psFilter->m_pfOutput_ch7,SampleCount,&(psFilter->xHist[7]),&(psFilter->yHist[7]));
    }

    
    #ifdef DC_DEBUG_
    fwrite(psFilter->m_pfInput_ch3,sizeof(LADSPA_Data),SampleCount,psFilter->fp_in);
    fwrite(psFilter->m_pfOutput_ch3,sizeof(LADSPA_Data),SampleCount,psFilter->fp_out);
    #endif
    return ;
}

/*****************************************************************************/

/* Throw away a filter instance. Normally separate functions
   would have to be written for the different plugin types, however we
   can get away with a single function in this case. */
void 
cleanupDCBFilter(LADSPA_Handle Instance)
{ 
    DCBFilter * psFilter;
    psFilter = (DCBFilter *)Instance;
    #ifdef DC_DEBUG_
    fclose(psFilter->fp_in);
    fclose(psFilter->fp_out);
    fclose(fp_txt);
    #endif
    free(Instance);
}

/*****************************************************************************/

LADSPA_Descriptor * g_psHPFDescriptor = NULL;

/*****************************************************************************/

void _init()
{
    char ** pcPortNames;
    LADSPA_PortDescriptor * piPortDescriptors;
    LADSPA_PortRangeHint * psPortRangeHints;
    g_psHPFDescriptor = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

    if (g_psHPFDescriptor != NULL) {

        g_psHPFDescriptor->UniqueID = 1024;
        g_psHPFDescriptor->Label = strdup("DCBDoing");
        g_psHPFDescriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
        g_psHPFDescriptor->Name = strdup("DCB High Pass Filter");
        g_psHPFDescriptor->Maker = strdup("ROCKCHIPS LADSPA PLUG FOR DCBDoing");
        g_psHPFDescriptor->Copyright = strdup("none");
        g_psHPFDescriptor->PortCount = 17;
        piPortDescriptors = (LADSPA_PortDescriptor *)calloc(17, sizeof(LADSPA_PortDescriptor));
        g_psHPFDescriptor->PortDescriptors = (const LADSPA_PortDescriptor *)piPortDescriptors;
        piPortDescriptors[SF_CHANNELS] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
        piPortDescriptors[SF_INPUT_CH0] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH0] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_INPUT_CH1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH1] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_INPUT_CH2] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH2] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_INPUT_CH3] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH3] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_INPUT_CH4] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH4] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_INPUT_CH5] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH5] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_INPUT_CH6] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH6] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_INPUT_CH7] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[SF_OUTPUT_CH7] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    
        pcPortNames = (char **)calloc(17, sizeof(char *));
        g_psHPFDescriptor->PortNames = (const char **)pcPortNames;
        pcPortNames[SF_CHANNELS] = strdup("The Current channels count");
        pcPortNames[SF_INPUT_CH0] = strdup("input ch0");
        pcPortNames[SF_OUTPUT_CH0] = strdup("Output ch0");
        pcPortNames[SF_INPUT_CH1] = strdup("input ch1");
        pcPortNames[SF_OUTPUT_CH1] = strdup("Output ch1");
        pcPortNames[SF_INPUT_CH2] = strdup("input ch2");
        pcPortNames[SF_OUTPUT_CH2] = strdup("Output ch2");
        pcPortNames[SF_INPUT_CH3] = strdup("input ch3");
        pcPortNames[SF_OUTPUT_CH3] = strdup("Output ch3");
        pcPortNames[SF_INPUT_CH4] = strdup("input ch4");
        pcPortNames[SF_OUTPUT_CH4] = strdup("Output ch4");
        pcPortNames[SF_INPUT_CH5] = strdup("input ch5");
        pcPortNames[SF_OUTPUT_CH5] = strdup("Output ch5");
        pcPortNames[SF_INPUT_CH6] = strdup("input ch6");
        pcPortNames[SF_OUTPUT_CH6] = strdup("Output ch6");
        pcPortNames[SF_INPUT_CH7] = strdup("input ch7");
        pcPortNames[SF_OUTPUT_CH7] = strdup("Output ch7");
        psPortRangeHints = ((LADSPA_PortRangeHint *)calloc(17, sizeof(LADSPA_PortRangeHint)));
        g_psHPFDescriptor->PortRangeHints = (const LADSPA_PortRangeHint *)psPortRangeHints;
        psPortRangeHints[SF_CHANNELS].HintDescriptor = (LADSPA_HINT_BOUNDED_BELOW 
	                                                    | LADSPA_HINT_BOUNDED_ABOVE
	                                                    | LADSPA_HINT_SAMPLE_RATE
	                                                    | LADSPA_HINT_LOGARITHMIC 
	                                                    | LADSPA_HINT_INTEGER
	                                                    | LADSPA_HINT_DEFAULT_440);
        psPortRangeHints[SF_CHANNELS].LowerBound = 1;
        psPortRangeHints[SF_CHANNELS].UpperBound = 8; 

        psPortRangeHints[SF_INPUT_CH0].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH0].HintDescriptor = 0;
        psPortRangeHints[SF_INPUT_CH1].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH1].HintDescriptor = 0;
        psPortRangeHints[SF_INPUT_CH2].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH2].HintDescriptor = 0;
        psPortRangeHints[SF_INPUT_CH3].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH3].HintDescriptor = 0;
        psPortRangeHints[SF_INPUT_CH4].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH4].HintDescriptor = 0;
        psPortRangeHints[SF_INPUT_CH5].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH5].HintDescriptor = 0;
        psPortRangeHints[SF_INPUT_CH6].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH6].HintDescriptor = 0;
        psPortRangeHints[SF_INPUT_CH7].HintDescriptor = 0;
        psPortRangeHints[SF_OUTPUT_CH7].HintDescriptor = 0;
    
        g_psHPFDescriptor->instantiate = instantiateDCBFilter;
        g_psHPFDescriptor->connect_port = connectPortToDCBFilter;
        g_psHPFDescriptor->activate = activateDCBFilter;
        g_psHPFDescriptor->run = runDCBHighPassFilter;
        g_psHPFDescriptor->run_adding = NULL;
        g_psHPFDescriptor->set_run_adding_gain = NULL;
        g_psHPFDescriptor->deactivate = NULL;
        g_psHPFDescriptor->cleanup = cleanupDCBFilter;
    }
}

/*****************************************************************************/

void deleteDescriptor(LADSPA_Descriptor * psDescriptor) 
{
    unsigned long lIndex;
    if (psDescriptor)
    {
        free((char *)psDescriptor->Label);
        free((char *)psDescriptor->Name);
        free((char *)psDescriptor->Maker);
        free((char *)psDescriptor->Copyright);
        free((LADSPA_PortDescriptor *)psDescriptor->PortDescriptors);
        for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
            free((char *)(psDescriptor->PortNames[lIndex]));
        free((char **)psDescriptor->PortNames);
        free((LADSPA_PortRangeHint *)psDescriptor->PortRangeHints);
        free(psDescriptor);
    }
}


/*****************************************************************************/

/* _fini() is called automatically when the library is unloaded. */
void _fini() 
{
    deleteDescriptor(g_psHPFDescriptor);
}

/*****************************************************************************/

/* Return a descriptor of the requested plugin type. There are two
   plugin types available in this library. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) 
{
  /* Return the requested descriptor or null if the index is out of
     range. */
     switch (Index) 
     {
        case 0:
            return g_psHPFDescriptor;
        default:
            return NULL;
    }
}

/*****************************************************************************/

/* EOF */








