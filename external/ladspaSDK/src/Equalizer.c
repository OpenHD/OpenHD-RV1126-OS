#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/inotify.h>
#include "RK_AudioInterface.h"
#include <unistd.h>


/*****************************************************************************/

#include "ladspa.h"
/*****************************************************************************/

#define VERSION "EQ/DRC Algorithm V1.1 2019-07-28"
#define PRINT_LOG 1
#define DEBUG_LOG 0
/*****************************************************************************/

/* The port numbers for the plugin: */
#define EQ_DRC_CONTROL 0
#define EQ_DRC_INPUT1  1
#define EQ_DRC_OUTPUT1 2
#define EQ_DRC_INPUT2  3
#define EQ_DRC_OUTPUT2 4
/*****************************************************************************/

#define EQ_DRC_PATH_PREFIX      "/data/cfg/eq_bin/Para_"
#define EQ_DRC_PATH_PREFIX_LEN  64

/************ EQ_DRC param debug**********/
#define EQ_DRC_PARAM_DEBUG_ 1
#define MAX_RECORD_SIZE 4 * 1024 * 1024

/*****************************************************************************/

/* The structure used to hold port connection information and state */
typedef struct {
    unsigned int m_eqfirstRun;
    unsigned int m_eqsampleRate;
    int last_frame_count ;

    /*************connect port*****************/
    LADSPA_Data * m_pfControlValue;
    LADSPA_Data * m_leftInputBuffer;
    LADSPA_Data * m_leftOutputBuffer;
    LADSPA_Data * m_rightInputBuffer;  /* (Not used for mono) */
    LADSPA_Data * m_rightOutputBuffer; /* (Not used for mono) */
    /***************param debug ***********************/
#ifdef EQ_DRC_PARAM_DEBUG_
    const char *filename;
    int fd;
    int wd;
#endif

    struct AUDIOPOST_STRUCT *AudioPostHandle;
} Equalizer;

/***************rk define*******************/
static char* rk_itoa(int val, char* dst, int radix)
{
	char *_pdst = dst;
	if (!val)
	{
		*_pdst = '0';
		*++_pdst = '\0';
		return dst;
	}
	if (val <0)
	{
		*_pdst++ = '-';
		val = -val;
	}
	char *_first = _pdst;
	char _cov;
	unsigned int _rem;
	while (val > 0)
	{
		_rem = (unsigned int)(val % radix);
		val /= radix;
		if (_rem > 9)
			*_pdst++ = (char)(_rem - 10 + 'a');
		else
			*_pdst++ = (char)(_rem + '0');
	}
	*_pdst-- = '\0';
	do {
		_cov = *_pdst;
		*_pdst = *_first;
		*_first = _cov;
		_pdst--;
		_first++;
	} while (_first < _pdst);
	return dst;
}

#if PRINT_LOG
	#define LOG(format, args...)	printf(format, ## args)
#endif

#if DEBUG_LOG
	#define LOG_DEBUG(format, args...)	printf(format, ## args)
#else
    #define LOG_DEBUG(format, args...)
#endif

#define true      1
#define false     0

static int checkDumpPCM(void) {
  char *value = NULL;

  /* env: "EQDRC_DUMP_PCM=TRUE" or "EQDRC_DUMP_PCM=true" ? */
  value = getenv("EQDRC_DUMP_PCM");
  if (value && (!strcmp("TRUE", value) || !strcmp("true", value)))
    return true;

  return false;
}

/*****************************************************************************/

/* Construct a new plugin instance. */
LADSPA_Handle instantiateEqualizer(const LADSPA_Descriptor * Descriptor,
		     unsigned long             SampleRate) {
  Equalizer * psEqualizer;

  // LOG("%s\n",VERSION);
  psEqualizer = (Equalizer *)malloc(sizeof(Equalizer));
  psEqualizer->m_eqsampleRate = (unsigned int)SampleRate;
  psEqualizer->AudioPostHandle = NULL;
  psEqualizer ->last_frame_count = 0;
  psEqualizer->fd = -1;
#ifdef EQ_DRC_PARAM_DEBUG_
  psEqualizer->filename = NULL;
#endif

  const char * Version = AudioPost_GetVersion();
  LOG("%s \n",Version);
  free(Version);

  return psEqualizer;
}


/*****************************************************************************/
/* Initialise and activate a plugin instance. */
void
activateEqualizer(LADSPA_Handle Instance) {

  Equalizer * psEqualizer;
  psEqualizer = (Equalizer *)Instance;

  psEqualizer->m_eqfirstRun = 0;
}


/*****************************************************************************/


/* Connect a port to a data location. */
void
connectPortToEqualizer(LADSPA_Handle Instance,
		       unsigned long Port,
		       LADSPA_Data * DataLocation) {

  Equalizer * psEqualizer;
  psEqualizer = (Equalizer *)Instance;

  switch (Port) {
  case EQ_DRC_CONTROL:
    psEqualizer->m_pfControlValue = DataLocation;
    LOG("psEqualizer->m_pfControlValue = %f\n",*psEqualizer->m_pfControlValue);
    break;
  case EQ_DRC_INPUT1:
    psEqualizer->m_leftInputBuffer = DataLocation;
    break;
  case EQ_DRC_OUTPUT1:
    psEqualizer->m_leftOutputBuffer = DataLocation;
    break;
  case EQ_DRC_INPUT2:
    /* (This should only happen for stereo.) */
    psEqualizer->m_rightInputBuffer = DataLocation;
    break;
  case EQ_DRC_OUTPUT2:
    /* (This should only happen for stereo.) */
    psEqualizer->m_rightOutputBuffer = DataLocation;
    break;
  }
}


/*****************************************************************************/

void
runMonoEqualizer(LADSPA_Handle Instance,
		 unsigned long SampleCount) {
  LADSPA_Data * pfInput;
  LADSPA_Data * pfOutput;
  Equalizer * psEqualizer;
  unsigned long lSampleIndex;
  char samp_name[10];
  unsigned int samplerate;
  int pcm_channel = 1;

  float reset_para[PARALEN] = {0};
  FILE *fp = NULL;
  FILE *binFile = NULL;

  int paramIndex ;


  psEqualizer = (Equalizer *)Instance;

  paramIndex = (int) *psEqualizer->m_pfControlValue;
  pfInput = psEqualizer->m_leftInputBuffer;
  pfOutput = psEqualizer->m_leftOutputBuffer;

/******************************init****************************/
/* eq_drc init because the samplecount only be passed in this*/
if(psEqualizer->m_eqfirstRun == 0)
{
    char *param_name;

    psEqualizer->m_eqfirstRun = 1;
    samplerate = psEqualizer->m_eqsampleRate;
    if((samplerate != 48000)&&(samplerate != 44100) &&(samplerate != 32000)
        &&(samplerate != 16000) && (samplerate != 8000))
    {
        LOG("Unsupport samplerate!\n");
        exit(1);
    }

    param_name = (char *)malloc(EQ_DRC_PATH_PREFIX_LEN);
    if (!param_name) {
        LOG("%s - malloc param_name failed!\n", __func__);
        exit(1);
    }

    strcpy(param_name,EQ_DRC_PATH_PREFIX);
    rk_itoa(samplerate,samp_name,10);
    strcat(param_name,samp_name);
	strcat(param_name,"Hz_1ch.bin");
    LOG("para_name  = %s\n",param_name);


    if((paramIndex > 0) && (paramIndex <=5)) {
            char *binNameStr = strrchr(param_name,'.');
            char addNum[1];
            char binName[256] = {0};
            LOG_DEBUG("%s\n",binName);
            strncpy(binName,param_name,(int)(strlen(param_name) - strlen(binNameStr)));
            printf("binName = %s\n",binName);
            strcat(binName,"_");
            rk_itoa(paramIndex, addNum, 10);
            strcat(binName,addNum);
            strcat(binName,".bin");
            LOG_DEBUG("binName = %s \n",binName);

            binFile = fopen(binName,"rb");
            if((binFile == NULL))
            {
                char bytesName[256] = {0};
                snprintf(bytesName,256,"cp %s %s",param_name,binName);
                LOG("%s\n",bytesName);
                system(bytesName);
                system("sync");
                LOG("copy eq_bin...\n");
            }
            else {
                fclose(binFile);
            }
    }

    LOG("param_name = %s\n",param_name);
    binFile = fopen(param_name,"rb");
    if(binFile == NULL)
    {
        system("cp /usr/lib/eq_bin/ /data/cfg/ -rf");
        LOG("copy eq_bin success.\n");
    }
    else
    {
        fclose(binFile);
    }


	psEqualizer->AudioPostHandle = AudioPost_Init(param_name, SampleCount);
    if(psEqualizer->AudioPostHandle == NULL)
    {
        printf("Create Mono AudioPost Handle Fail.\n");
        exit(1);
    }

#ifdef EQ_DRC_PARAM_DEBUG_
    if (psEqualizer->filename) {
        free(psEqualizer->filename);
        psEqualizer->filename = NULL;
    }
    psEqualizer->filename = param_name;
    LOG_DEBUG("filename = %s\n",psEqualizer->filename);

	psEqualizer->fd = inotify_init();
	LOG_DEBUG("psEqualizer->fd = %d\n",psEqualizer->fd);
#endif
}
/**************************process*********************************/
  if (checkDumpPCM() == true) {
    char filename[64] = {0};
    FILE *fp;

    sprintf(filename, "/tmp/eqdrc-%dch-in-%d.pcm",
      pcm_channel, psEqualizer->m_eqsampleRate);
    fp = fopen(filename, "ab+");
    fwrite(pfInput, 1, SampleCount * sizeof(LADSPA_Data) * pcm_channel, fp);
    fclose(fp);
  }

  for (lSampleIndex = 0; lSampleIndex <2 * SampleCount; lSampleIndex = lSampleIndex + 2)
  {
      pfInput[lSampleIndex] = pfInput[lSampleIndex] * 32767.0;
  }

  AudioPost_Process(psEqualizer->AudioPostHandle,pfInput, pfOutput , pcm_channel, SampleCount);

  for (lSampleIndex = 0; lSampleIndex <2 * SampleCount; lSampleIndex = lSampleIndex + 2)
  {
      pfOutput[lSampleIndex] = pfOutput[lSampleIndex] / 32767.0;
  }

  if (checkDumpPCM() == true) {
    char filename[64] = {0};
    FILE *fp;

    sprintf(filename, "/tmp/eqdrc-%dch-out-%d.pcm",
      pcm_channel, psEqualizer->m_eqsampleRate);
    fp = fopen(filename, "ab+");
    fwrite(pfOutput, 1, SampleCount * sizeof(LADSPA_Data) * pcm_channel, fp);
    fclose(fp);
  }

  /*momitor bin-file was change only debug*/
#ifdef EQ_DRC_PARAM_DEBUG_
   /*adding the "/data/rpc-d/" directory into watch list.
    *Here, the suggestion is to validate the existence of
    *the directory before adding into monitoring list.
    */
    psEqualizer->wd = inotify_add_watch( psEqualizer->fd, psEqualizer->filename,IN_MODIFY |IN_ACCESS | IN_CREATE | IN_DELETE | IN_OPEN | IN_CLOSE_NOWRITE);
    LOG_DEBUG("psEqualizer->wd = %d,fd = %d\n",psEqualizer->wd,psEqualizer->fd);
    LOG_DEBUG("filename = %s\n",psEqualizer->filename);

    if(psEqualizer->wd == IN_MODIFY)//IN_MODIFY
    {
        LOG_DEBUG("file was changed :wd = %d ",psEqualizer->wd);
        fp = fopen(psEqualizer->filename,"rb");
        if(fp != NULL)
        {
            fread(reset_para,sizeof(float),PARALEN,fp);
            AudioPost_SetPara(psEqualizer->AudioPostHandle,reset_para, SampleCount);//EQ_DRC param reset
            LOG("modified the param succedd!!!\n");
        }
        else{
            LOG("ERROR:OPNE NEW BIN FILE FAILED!!!\n");
            exit(1);
        }
        fclose(fp);
        if(psEqualizer->fd >= 0) {
            inotify_rm_watch (psEqualizer->fd, psEqualizer->wd);
            close(psEqualizer->fd);
            psEqualizer->fd = -1;
        }
        psEqualizer->fd = inotify_init();
    }

#endif

/***********************this part must be changed*****/
}

/*****************************************************************************/

/*****************************************************************************/


void runStereoEqualizer(LADSPA_Handle Instance,
		   unsigned long SampleCount)
{
    LADSPA_Data * pfInput;
    LADSPA_Data * pfOutput;
    Equalizer * psEqualizer;
    unsigned long lSampleIndex;
    char samp_name[10];
    unsigned int samplerate;
    int pcm_channel = 2;
    float reset_para[PARALEN] = {0};
    FILE *fp = NULL;
    FILE *binFile = NULL;
    int paramIndex;

    psEqualizer = (Equalizer *)Instance;

    paramIndex = (int)*psEqualizer->m_pfControlValue;
    
/******************************init****************************/
/* eq_drc init because the samplecount only be passed in this*/

    if(psEqualizer->m_eqfirstRun == 0)
    {
        char *param_name;

        psEqualizer->m_eqfirstRun = 1;
        samplerate = psEqualizer->m_eqsampleRate;
        if((samplerate != 48000)&&(samplerate != 44100) &&(samplerate != 32000)
            &&(samplerate != 16000) && (samplerate != 8000))
        {
            LOG("Unsupport samplerate!\n");
            exit(1);
        }

        param_name = (char *)malloc(EQ_DRC_PATH_PREFIX_LEN);
        if (!param_name) {
            LOG("%s - malloc param_name failed!\n", __func__);
            exit(1);
        }

        strcpy(param_name,EQ_DRC_PATH_PREFIX);
        rk_itoa(samplerate,samp_name,10);
        strcat(param_name,samp_name);
	    strcat(param_name,"Hz_2ch.bin");

        LOG_DEBUG("paramIndex = %d\n",paramIndex);
        if((paramIndex > 0) && (paramIndex <=5)) {
            char *binNameStr = strrchr(param_name,'.');
            char addNum[1];
            char binName[256] = {0};
            LOG_DEBUG("%s\n",binName);
            strncpy(binName,param_name,(int)(strlen(param_name) - strlen(binNameStr)));
            printf("binName = %s\n",binName);
            strcat(binName,"_");
            rk_itoa(paramIndex, addNum, 10);
            strcat(binName,addNum);
            strcat(binName,".bin");
            LOG_DEBUG("binName = %s \n",binName);

            binFile = fopen(binName,"rb");
            if((binFile == NULL))
            {
                char bytesName[256] = {0};
                snprintf(bytesName,256,"cp %s %s",param_name,binName);
                LOG("%s\n",bytesName);
                system(bytesName);
                system("sync");
                LOG("copy eq_bin...\n");
            }
            else {
                fclose(binFile);
            }
            strncpy(param_name,binName,strlen(binName));
        }

        // detect eq_bin file
        LOG("param_name = %s\n",param_name);
        binFile = fopen(param_name,"rb");
        if((binFile == NULL))
        {
	    system("cp /usr/lib/eq_bin/ /data/cfg/ -rf");
	    system("sync");
            LOG("copy eq_bin...\n");
        }
        else {
            fclose(binFile);
        }
        psEqualizer->AudioPostHandle = AudioPost_Init(param_name, SampleCount);
        if(psEqualizer->AudioPostHandle == NULL)
        {
            LOG("Create audiopost handle fail...\n");
            exit(1);
        }
         psEqualizer->last_frame_count = SampleCount;
         
#ifdef EQ_DRC_PARAM_DEBUG_
        if (psEqualizer->filename) {
            free(psEqualizer->filename);
            psEqualizer->filename = NULL;
        }
        psEqualizer->filename = param_name;
        LOG_DEBUG("filename = %s\n",psEqualizer->filename);
	    psEqualizer->fd = inotify_init();
	    LOG_DEBUG("psEqualizer->fd = %d\n",psEqualizer->fd);
#endif
    }
/***********************this part must be changed**********/
    pfInput = (LADSPA_Data *)malloc(2 * SampleCount * sizeof(LADSPA_Data));
    memset(pfInput,0.0,2 * SampleCount);
    pfOutput = (LADSPA_Data *)malloc(2 * SampleCount * sizeof(LADSPA_Data));
    memset(pfOutput,0.0,2 * SampleCount);
    for (lSampleIndex = 0; lSampleIndex <2 * SampleCount; lSampleIndex = lSampleIndex + 2)
    {
    #if 0
        printf("*(pfInput) = %f\n",*(pfInput));
        *(pfInput++) =  *(psEqualizer->m_leftInputBuffer);
        psEqualizer->m_leftInputBuffer ++;
        printf("*(pfInput) = %f\n",*(pfInput));
        *(pfInput++) =  *(psEqualizer->m_rightInputBuffer);
        psEqualizer->m_rightInputBuffer ++;
    #else
        pfInput[lSampleIndex] = psEqualizer->m_leftInputBuffer[lSampleIndex/2] * 32767.0;
        pfInput[lSampleIndex + 1] = psEqualizer->m_rightInputBuffer[lSampleIndex/2] * 32767.0;
       // LOG_DEBUG("*(pfInput) = %f %f\n",(pfInput[lSampleIndex]),pfInput[lSampleIndex + 1]);
    #endif
    }

    AudioPost_Process(psEqualizer->AudioPostHandle,pfInput, pfOutput, pcm_channel, SampleCount);

    for (lSampleIndex = 0; lSampleIndex <2 * SampleCount; lSampleIndex = lSampleIndex + 2)
    {
    #if 0
        //printf("*(pfOutput) = %f\n",*(pfOutput));
        *(psEqualizer->m_leftInputBuffer ++) = *(pfOutput++);
        // printf("*(pfOutput) = %f\n",*(pfOutput));
        *(psEqualizer->m_rightInputBuffer ++) = *(pfOutput++);
    #else
        psEqualizer->m_leftOutputBuffer[lSampleIndex/2] = pfOutput[lSampleIndex] / 32767.0;
        psEqualizer->m_rightOutputBuffer[lSampleIndex/2] = pfOutput[lSampleIndex +1] / 32767.0;
    #endif
    }

    if (checkDumpPCM() == true) {
      char filename[64] = {0};
      FILE *fp;

      for (lSampleIndex = 0; lSampleIndex <2 * SampleCount; lSampleIndex = lSampleIndex + 2)
      {
          pfInput[lSampleIndex] = pfInput[lSampleIndex] / 32767.0;
          pfInput[lSampleIndex + 1] = pfInput[lSampleIndex +1] / 32767.0;
          pfOutput[lSampleIndex] = pfOutput[lSampleIndex] / 32767.0;
          pfOutput[lSampleIndex + 1] = pfOutput[lSampleIndex + 1] / 32767.0;
         // LOG_DEBUG("*(pfInput) = %f %f\n",(pfInput[lSampleIndex]),pfInput[lSampleIndex + 1]);
      }

      sprintf(filename, "/tmp/eqdrc-%dch-in-%d.pcm",
        pcm_channel, psEqualizer->m_eqsampleRate);
      fp = fopen(filename, "ab+");
      fwrite(pfInput, 1, SampleCount * sizeof(LADSPA_Data) * pcm_channel, fp);
      fclose(fp);

      sprintf(filename, "/tmp/eqdrc-%dch-out-%d.pcm",
        pcm_channel, psEqualizer->m_eqsampleRate);
      fp = fopen(filename, "ab+");
      fwrite(pfOutput, 1, SampleCount * sizeof(LADSPA_Data) * pcm_channel, fp);
      fclose(fp);
    }

#ifdef EQ_DRC_PARAM_DEBUG_
   /*adding the "/data/rpc-d/" directory into watch list.
    *Here, the suggestion is to validate the existence of
    *the directory before adding into monitoring list.
    */
    psEqualizer->wd = inotify_add_watch( psEqualizer->fd, psEqualizer->filename,IN_MODIFY |IN_ACCESS | IN_CREATE | IN_DELETE | IN_OPEN | IN_CLOSE_NOWRITE);
    // LOG_DEBUG("psEqualizer->wd = %d,fd = %d\n",psEqualizer->wd,psEqualizer->fd);
    // LOG_DEBUG("filename = %s\n",psEqualizer->filename);

    if(psEqualizer->wd == IN_MODIFY)
    {
        LOG_DEBUG("file was changed :wd = %d ",psEqualizer->wd);
        system("sync");
        fp = fopen(psEqualizer->filename,"rb");
        if(fp != NULL)
        {
            fread(reset_para,sizeof(float),PARALEN,fp);
            AudioPost_SetPara(psEqualizer->AudioPostHandle,reset_para, SampleCount);//EQ_DRC param reset
            LOG("modified the param succedd!!!\n");
        }
        else {
            LOG("ERROR:OPNE NEW BIN FILE FAILED!!!\n");
            return;
        }
        fclose(fp);
        #if 1
        LOG_DEBUG("PRE,psEqualizer->wd = %d,fd = %d\n",psEqualizer->wd,psEqualizer->fd);
        if(psEqualizer->fd >= 0) {
            inotify_rm_watch (psEqualizer->fd, psEqualizer->wd);//删除监视
            close(psEqualizer->fd);
            psEqualizer->fd = -1;
        }
        psEqualizer->fd = inotify_init();//重新初始化监视
        LOG_DEBUG("psEqualizer->wd = %d,fd = %d\n",psEqualizer->wd,psEqualizer->fd);

        #endif

    }

#endif


#if 0
    pfInput = psEqualizer->m_leftInputBuffer;
    pfOutput = psEqualizer->m_leftOutputBuffer;
    for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++)
        *(pfOutput++) = *(pfInput++);//demo原样输出
    pfInput = psEqualizer->m_rightInputBuffer;
    pfOutput = psEqualizer->m_rightOutputBuffer;
    for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++)
        *(pfOutput++) = *(pfInput++);
#endif
    free(pfInput);
    free(pfOutput);
}



/*****************************************************************************/

/* Throw away a simple delay line. */
void cleanupEqualizer(LADSPA_Handle Instance) {

/****************Equalizer deinit*************************/
    Equalizer * psEqualizer;
    psEqualizer = (Equalizer *)Instance;

#ifdef EQ_DRC_PARAM_DEBUG_
    if(psEqualizer->fd >= 0) {
        inotify_rm_watch (psEqualizer->fd, psEqualizer->wd);
        close(psEqualizer->fd);
        psEqualizer->fd = -1;
    }
    if (psEqualizer->filename) {
            free(psEqualizer->filename);
            psEqualizer->filename = NULL;
    }
#endif

    if(psEqualizer->m_eqfirstRun == 1)
    {
        AudioPost_Destroy(psEqualizer->AudioPostHandle);
        psEqualizer->m_eqfirstRun = 0;
    }
    free(Instance);
}

/*****************************************************************************/

LADSPA_Descriptor * g_eqMonoDescriptor = NULL;
LADSPA_Descriptor * g_eqStereoDescriptor = NULL;

/*****************************************************************************/
/* _init() is called automatically when the plugin library is first loaded. */
void
_init() {
    char ** pcPortNames;
    LADSPA_PortDescriptor * piPortDescriptors;
    LADSPA_PortRangeHint * psPortRangeHints;
    g_eqMonoDescriptor = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
    g_eqStereoDescriptor = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
    if (g_eqMonoDescriptor) {
        g_eqMonoDescriptor->UniqueID = 1060;
        g_eqMonoDescriptor->Label = strdup("eq_drc_mono");
        g_eqMonoDescriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
        g_eqMonoDescriptor->Name = strdup("Mono Equalizer");
        g_eqMonoDescriptor->Maker = strdup("Richard Furse (LADSPA example plugins)");
        g_eqMonoDescriptor->Copyright = strdup("None");
        g_eqMonoDescriptor->PortCount = 3;
        piPortDescriptors = (LADSPA_PortDescriptor *)calloc(3, sizeof(LADSPA_PortDescriptor));
        g_eqMonoDescriptor->PortDescriptors = (const LADSPA_PortDescriptor *)piPortDescriptors;
        piPortDescriptors[EQ_DRC_CONTROL] = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
        piPortDescriptors[EQ_DRC_INPUT1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[EQ_DRC_OUTPUT1]= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        pcPortNames = (char **)calloc(3, sizeof(char *));
        g_eqMonoDescriptor->PortNames = (const char **)pcPortNames;
        pcPortNames[EQ_DRC_CONTROL] = strdup("Gain");
        pcPortNames[EQ_DRC_INPUT1] = strdup("Input");
        pcPortNames[EQ_DRC_OUTPUT1] = strdup("Output");
        psPortRangeHints = ((LADSPA_PortRangeHint *)calloc(3, sizeof(LADSPA_PortRangeHint)));
        g_eqMonoDescriptor->PortRangeHints = (const LADSPA_PortRangeHint *)psPortRangeHints;
        psPortRangeHints[EQ_DRC_CONTROL].HintDescriptor
            = (LADSPA_HINT_BOUNDED_BELOW
	        | LADSPA_HINT_LOGARITHMIC
	        | LADSPA_HINT_DEFAULT_1);
        psPortRangeHints[EQ_DRC_CONTROL].LowerBound = 0;
        psPortRangeHints[EQ_DRC_INPUT1].HintDescriptor = 0;
        psPortRangeHints[EQ_DRC_OUTPUT1].HintDescriptor = 0;
        g_eqMonoDescriptor->instantiate = instantiateEqualizer;
        g_eqMonoDescriptor->connect_port = connectPortToEqualizer;
        g_eqMonoDescriptor->activate = activateEqualizer;
        g_eqMonoDescriptor->run = runMonoEqualizer;
        g_eqMonoDescriptor->run_adding = NULL;
        g_eqMonoDescriptor->set_run_adding_gain = NULL;
        g_eqMonoDescriptor->deactivate = NULL;
        g_eqMonoDescriptor->cleanup = cleanupEqualizer;
  }

    if (g_eqStereoDescriptor) {
        g_eqStereoDescriptor->UniqueID = 1061;
        g_eqStereoDescriptor->Label = strdup("eq_drc_stereo");
        g_eqStereoDescriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
        g_eqStereoDescriptor->Name = strdup("Stereo Equalizer");
        g_eqStereoDescriptor->Maker = strdup("Richard Furse (LADSPA example plugins)");
        g_eqStereoDescriptor->Copyright = strdup("None");
        g_eqStereoDescriptor->PortCount = 5;
        piPortDescriptors = (LADSPA_PortDescriptor *)calloc(5, sizeof(LADSPA_PortDescriptor));
        g_eqStereoDescriptor->PortDescriptors = (const LADSPA_PortDescriptor *)piPortDescriptors;
        piPortDescriptors[EQ_DRC_CONTROL] = LADSPA_PORT_INPUT |LADSPA_PORT_CONTROL;
        piPortDescriptors[EQ_DRC_INPUT1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[EQ_DRC_OUTPUT1] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[EQ_DRC_INPUT2] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        piPortDescriptors[EQ_DRC_OUTPUT2] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        pcPortNames = (char **)calloc(5, sizeof(char *));
        g_eqStereoDescriptor->PortNames = (const char **)pcPortNames;
        pcPortNames[EQ_DRC_CONTROL] = strdup("Gain");
        pcPortNames[EQ_DRC_INPUT1] = strdup("Input (Left)");
        pcPortNames[EQ_DRC_OUTPUT1]= strdup("Output (Left)");
        pcPortNames[EQ_DRC_INPUT2] = strdup("Input (Right)");
        pcPortNames[EQ_DRC_OUTPUT2] = strdup("Output (Right)");
        psPortRangeHints = ((LADSPA_PortRangeHint *)calloc(5, sizeof(LADSPA_PortRangeHint)));
        g_eqStereoDescriptor->PortRangeHints = (const LADSPA_PortRangeHint *)psPortRangeHints;
        psPortRangeHints[EQ_DRC_CONTROL].HintDescriptor = (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_DEFAULT_1);
        psPortRangeHints[EQ_DRC_CONTROL].LowerBound = 0;
        psPortRangeHints[EQ_DRC_INPUT1].HintDescriptor= 0;
        psPortRangeHints[EQ_DRC_OUTPUT1].HintDescriptor = 0;
        psPortRangeHints[EQ_DRC_INPUT2].HintDescriptor = 0;
        psPortRangeHints[EQ_DRC_OUTPUT2].HintDescriptor = 0;
        g_eqStereoDescriptor->instantiate = instantiateEqualizer;
        g_eqStereoDescriptor->connect_port = connectPortToEqualizer;
        g_eqStereoDescriptor->activate = activateEqualizer;
        g_eqStereoDescriptor->run = runStereoEqualizer;
        g_eqStereoDescriptor->run_adding = NULL;
        g_eqStereoDescriptor->set_run_adding_gain = NULL;
        g_eqStereoDescriptor->deactivate = NULL;
        g_eqStereoDescriptor->cleanup = cleanupEqualizer;
    }
}


/*****************************************************************************/

void deleteDescriptor(LADSPA_Descriptor * psDescriptor)
{
    unsigned long lIndex;
    if (psDescriptor) {
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
    deleteDescriptor(g_eqMonoDescriptor);
    deleteDescriptor(g_eqStereoDescriptor);
}

/*****************************************************************************/
/* Return a descriptor of the requested plugin type. There are two
   plugin types available in this library (mono and stereo). */
const LADSPA_Descriptor * ladspa_descriptor(unsigned long Index)
{
    /* Return the requested descriptor or null if the index is out of
    range. */
    switch (Index) {
        case 0:
            return g_eqMonoDescriptor;
        case 1:
            return g_eqStereoDescriptor;
        default:
            return NULL;
  }
}

/*****************************************************************************/
