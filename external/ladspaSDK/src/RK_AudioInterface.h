#ifndef RK_AUDIO_POST_PROINTERFACE_H
#define RK_AUDIO_POST_PROINTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#define PARALEN          (4096)  // 参数个数
struct AUDIOPOST_STRUCT;

typedef void               VOICE_VOID;
typedef int				   VOICE_INT32;
typedef float              VOICE_FLOAT;

extern struct AUDIOPOST_STRUCT* AudioPost_Init(char  *pcParaName,
		                   int swFrmLen);


extern void AudioPost_Process(struct AUDIOPOST_STRUCT *handle,
                              float *pfIn,
	                          float *pfOut,
	                          short int shwChannelNum,
	                          int swFrmLen);



extern void AudioPost_Destroy(struct AUDIOPOST_STRUCT *handle);

extern VOICE_VOID AudioPost_SetPara(struct AUDIOPOST_STRUCT *handle,
                                    VOICE_FLOAT *pfPara,
							        VOICE_INT32 swFrmLen);
extern VOICE_VOID AudioPost_GetPara(struct AUDIOPOST_STRUCT *handle,VOICE_FLOAT *pfPara);
extern const char *AudioPost_GetVersion(void);


#ifdef __cplusplus
}
#endif

#endif



