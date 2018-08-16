#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <stdbool.h>
#include <asoundlib.h>


#define  LOG_TAG    "ReadPCM"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

#define BUFSIZE (16384*2)
struct pcm *pcm = NULL;
struct pcm *pcm_play = NULL;
//char *temp_buffer;
char buffer[BUFSIZE] = {0};
//char buffer_play[2048] = {0};
unsigned int size = 16384, size_play = 2048;
static bool bsearch_startch = false;
bool bneed_rec = false;
static int startch = -1, flag = 0;
char *wav_name = NULL;
FILE * fp_wav = NULL;
unsigned int rec_totallen = 0;
struct wav_header header;
int isI2s = 0;

/*static int mem2file(const char * name, char* buf, int size)
{
	int len;
	FILE * fp = fopen(name, "a");
	if(fp <= 0) {
		LOGE("open %s error\n", name);
		return -1;
	}
	len = fwrite(buf, 1, size, fp);
	if(len < size) {
		LOGE("write %s error, len write=%d, size=%d\n", name, len, size);
	}
	fclose(fp);
	return len;
}*/

int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                 char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        LOGE("%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        LOGE("%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                        unsigned int rate, unsigned int bits, unsigned int period_size,
                        unsigned int period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        LOGE("Unable to open PCM device %u.\n", device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", "Hz");
    can_play &= check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", "Hz");

    pcm_params_free(params);

    return can_play;
}

/*
 * 打开录音声卡设备
 * isNeedRec:是否需要录音
 * recName：录音文件名
 * nDevice：声卡设备：0：i2s
 * 					 1：USB
 * 					 2：SPI
 * rate:采样频率
 *		   @channels:通道数
 * 返回值:   1 		成功
 *	 	       负数		失败
 *
 */
JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_pcmOpen(JNIEnv * env, jobject this, jint isNeedRec, jstring recName,jint nDevice,jint rate,jint channels)
{	
  struct pcm_config config;
  int need_rec = isNeedRec;
  unsigned int card = 0, device = 0;

  LOGE("need_rec:%d\n",need_rec);
  if(need_rec)
  {
    header.riff_id = ID_RIFF;
    header.riff_sz = 0;
    header.riff_fmt = ID_WAVE;
    header.fmt_id = ID_FMT;
    header.fmt_sz = 16;
    header.audio_format = FORMAT_PCM;
    header.num_channels = 8;
    header.sample_rate = 16000;

    header.bits_per_sample = pcm_format_to_bits(PCM_FORMAT_S16_LE);
	header.byte_rate = (header.bits_per_sample / 8) * (header.num_channels) * (header.sample_rate);
	header.block_align = (header.num_channels) * (header.bits_per_sample / 8);
	header.data_id = ID_DATA;

	bneed_rec = true;
	wav_name = (char *)(*env)->GetStringUTFChars(env, recName, 0);
	fp_wav = fopen(wav_name, "w+b");
	if(fp_wav <= 0) {
		LOGE("open %s error\n", wav_name);
		return -1;
	}
	/* leave enough room for header */
	fseek(fp_wav, sizeof(struct wav_header), SEEK_SET);
  }
  //config.channels = 2;

  config.period_size = 1024;
  config.period_count = 4;
  config.format = PCM_FORMAT_S16_LE;
  config.start_threshold = 0;
  config.stop_threshold = 0;
  config.silence_threshold = 0;
  if(nDevice == 0)
  {
	  //i2s
	  card = 0;
	  //device = 0;
	  config.rate = 16000;
	  config.channels = 2;
	  isI2s = 1;
  }
  else if(nDevice == 1)
  {
	  //USB
	  card = 3;
	  //device = 0;
	  //config.rate = 64000;
	  config.rate = rate;
	  config.channels = channels;
	  isI2s = 0;
  }
  	//LOGE( "*******PCM rate (%d)\n",config.rate);
	pcm = pcm_open(card, device, PCM_IN, &config);
	if (!pcm || !pcm_is_ready(pcm)) {
		LOGE( "Unable to open PCM device (%s)\n",
				pcm_get_error(pcm));
		return -1;
	}

  size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
  /*temp_buffer = malloc(size*2);
  if (!temp_buffer) {
      LOGE("Unable to allocate %d bytes\n", size*2);
      free(temp_buffer);
      pcm_close(pcm);
      return -1;
  }*/

  bsearch_startch = true;
  memset(buffer,0,sizeof(buffer));
  return 1;
}


JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_readPcmStart(JNIEnv * env, jobject this, jbyteArray mic1_data, jbyteArray mic2_data, jbyteArray mic3_data, jbyteArray mic4_data, jbyteArray mic5_data, jbyteArray mic6_data, jbyteArray mic7_data, jbyteArray mic8_data)
{
	int ret = 0;
	int i = 0, j = 0;
	jbyte *pmic_data[8] = {0};//8 ch mic
	int thisch = -1, len = 0, cnt = 0, rec_len = 0;
	//jbyte *pmic_data1;
	char *pdata = NULL;
	unsigned int read_bytes = 0;

	pmic_data[0] = (*env)->GetByteArrayElements(env,mic1_data,0);
	pmic_data[1] = (*env)->GetByteArrayElements(env,mic2_data,0);
	pmic_data[2] = (*env)->GetByteArrayElements(env,mic3_data,0);
	pmic_data[3] = (*env)->GetByteArrayElements(env,mic4_data,0);
	pmic_data[4] = (*env)->GetByteArrayElements(env,mic5_data,0);
	pmic_data[5] = (*env)->GetByteArrayElements(env,mic6_data,0);
	pmic_data[6] = (*env)->GetByteArrayElements(env,mic7_data,0);
	pmic_data[7] = (*env)->GetByteArrayElements(env,mic8_data,0);

	if(isI2s == 1)
	{
		if(bsearch_startch)
			{
				ret = pcm_read(pcm, buffer, size);
				if(ret != 0)
				{
					LOGE("pcm_read fail return : %d\n", ret);

					return ret;
				}
				//memcpy((void *)buffer,temp_buffer,size);
				//pdata = temp_buffer;

				for(i=0; i<16; i++)
				{
					if(//((buffer[i]==0xFF)&&(buffer[i+1]==0xA5))||
						((buffer[i]==0xA5)&&(buffer[i+1]==0xFF)))
					{
						LOGE("buffer[i]:%x,buffer[i+1]:%x!\n",buffer[i],buffer[i+1]);
						flag = i;
						cnt++;
					}
				}

				if(cnt == 1)
				{

				}
				else if(cnt == 0)
				{
					LOGE("data error!\n");
					return -1;
				}
				else
				{
					//LOGE("cnt == %d!\n",cnt);
					for(i=0; i<16; i++)
					{
						if(((buffer[i]==0xA5)&&(buffer[i+1]==0xFF))
								&&((buffer[i+16]==0xA5)&&(buffer[i+17]==0xFF))
								&&((buffer[i+32]==0xA5)&&(buffer[i+33]==0xFF))
							)
						{
							flag = i;
							break;
						}
					}


				}

				//搬运
				memcpy((void *)buffer,buffer+flag+10,size-flag-10);
				bsearch_startch = false;
			}//if(bsearch_startch)

				//pdata = buffer+size-flag-10;
			ret = pcm_read(pcm, buffer+size-flag-10, size);
			if(ret != 0)
			{
				LOGE("pcm_read fail return : %d\n", ret);
				return ret;
			}	
	}//if(isI2s == 1)
	else if(isI2s == 0)
	{
		ret = pcm_read(pcm, buffer, size);
		if(ret != 0)
		{
			LOGE("pcm_read fail return : %d\n", ret);
			return ret;
		}
	}

	//memcpy((void *)buffer+size-flag-10,temp_buffer,size);
	pdata = buffer;

	thisch = 1;
	j = 0;
	while(read_bytes <= size)
	{
		memcpy((void *)(pmic_data[thisch]+j*2), pdata, 2);
		//pmic_data[thisch] += 2;
		pdata += 2;
		thisch += 2;
		read_bytes += 2;
		if(thisch == 9)
		{
			thisch = 0;
		}
		else if(thisch == 8)
		{
			thisch = 1;
		}
		if(read_bytes%16 == 0)
		{
			j++;
		}
	}//while(read_bytes <= size)

	if(bneed_rec)
	{
		//mem2file(wav_name,buffer,size);
		rec_len = fwrite(buffer, 1, size, fp_wav);
		if(rec_len < (size)) {
			LOGE("write error, len write=%d, size-flag-10=%d\n", rec_len, size);
		}
		rec_totallen += rec_len;
	}

	memcpy((void *)buffer,buffer+size,size-flag-10);

	(*env)->ReleaseByteArrayElements(env, mic1_data, pmic_data[0], 0);
	(*env)->ReleaseByteArrayElements(env, mic2_data, pmic_data[1], 0);
	(*env)->ReleaseByteArrayElements(env, mic3_data, pmic_data[2], 0);
	(*env)->ReleaseByteArrayElements(env, mic4_data, pmic_data[3], 0);
	(*env)->ReleaseByteArrayElements(env, mic5_data, pmic_data[4], 0);
	(*env)->ReleaseByteArrayElements(env, mic6_data, pmic_data[5], 0);
	(*env)->ReleaseByteArrayElements(env, mic7_data, pmic_data[6], 0);
	(*env)->ReleaseByteArrayElements(env, mic8_data, pmic_data[7], 0);		 
	
	 return ret;
}

JNIEXPORT void JNICALL Java_net_mnano_ReadPCM_readPcmStop(JNIEnv * env, jobject this)
{
	bsearch_startch = false;
	int len = 0;

	if(bneed_rec)
	{
		if(fp_wav)
		{
			//写wav头
			/* write header now all information is known */
			header.data_sz = rec_totallen ;//frames * header.block_align;
			header.riff_sz = header.data_sz + sizeof(header) - 8;
			fseek(fp_wav, 0, SEEK_SET);
			len = fwrite(&header, 1,sizeof(struct wav_header), fp_wav);
			if(len < sizeof(struct wav_header))
			{
				LOGE("write wav_header error, len write=%d, size=%d\n", len, size);
			}

			fclose(fp_wav);
			fp_wav = NULL;
		}
		rec_totallen = 0;
		bneed_rec = false;
	}

	/*if(buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
	}*/
	if(NULL != pcm)
	{
		//LOGE("*******pcm_close!!\n");
		pcm_close(pcm);
	}
}

/*
 * 打开播放声卡设备
 *
 * nDevice：声卡设备：0：i2s
 * 					 1：USB
 * 					 2：SPI
 * rate:采样频率
 *
 * 返回值:   1 		成功
 *	 	       负数		失败
 */
JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_playPcmOpen(JNIEnv *env, jobject this, jint nDevice,jint rate,jint period_size,jint period_count)
{
	struct pcm_config config;
	unsigned int card = 0, device = 0;

	config.channels = 2;
	config.rate = 44100;
	config.period_size = 256;//1024
	config.period_count = 2;//4
	config.format = PCM_FORMAT_S16_LE;
	config.start_threshold = 0;
	config.stop_threshold = 0;
	config.silence_threshold = 0;

	if(nDevice == 0)
	{
	  //i2s
	  card = 0;
	  config.rate = 16000;
	}
	else if(nDevice == 1)
	{
		//USB
		card = 3;
		//config.rate = 64000;
		config.rate = rate;
		config.period_size = period_size; //256*2;//1024
		config.period_count = period_count; //2*3;//4
	}
	if (!sample_is_playable(card, device, config.channels, config.rate, 16, config.period_size, config.period_count)) {
		LOGE("sample is not playable!!!\n");
		return -1;
	}

	pcm_play = pcm_open(card, device, PCM_OUT, &config);
	if (!pcm_play || !pcm_is_ready(pcm_play)) {
		LOGE("Unable to open PCM device (%s)\n",pcm_get_error(pcm_play));
		return -1;
	}

	size_play = pcm_frames_to_bytes(pcm_play, pcm_get_buffer_size(pcm_play));
	LOGE("size_play:%d!!!\n",size_play);
	/*buffer = malloc(size_play);
	if (!buffer) {
		fprintf(stderr, "Unable to allocate %d bytes\n", size);
		free(buffer);
		pcm_close(pcm);
		return;
	}*/
	//memset(buffer_play,0,sizeof(buffer_play));
	return 1;
}

/*
 * 播放PCM数据
 * playData:播放的数据
 *
 * 返回值：0 成功
 * 		负数  失败
 */
JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_playPcmStart(JNIEnv *env, jobject this, jbyteArray playData)
{
	int ret = 0;
	jbyte *pplay_data = {0};

	pplay_data = (*env)->GetByteArrayElements(env,playData,0);

	ret = pcm_write(pcm_play, (void *)pplay_data, size_play);
	if(ret != 0)
	{
		LOGE("pcm_write fail return : %d\n", ret);
		return ret;
	}

	(*env)->ReleaseByteArrayElements(env, playData, pplay_data, 0);

	return ret;
}

/*
 * 关闭播放设备
 */
JNIEXPORT void JNICALL Java_net_mnano_ReadPCM_playPcmStop(JNIEnv *env, jobject this)
{
	if(NULL != pcm)
	{
		//LOGE("*******pcm_close!!\n");
		pcm_close(pcm_play);
	}
}

