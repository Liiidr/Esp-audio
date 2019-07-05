/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "periph_button.h"
#include "board.h"

#include "sdkconfig.h"
#include "audio_mem.h"
#include "dueros_app.h"
#include "recorder_engine.h"
#include "esp_audio.h"
#include "esp_log.h"

#include "duer_audio_wrapper.h"
#include "dueros_service.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "filter_resample.h"
#include "fatfs_stream.h"
#include "http_stream.h"
#include "amr_decoder.h"
#include "amrwb_encoder.h"
#include "mp3_decoder.h"
#include "wav_decoder.h"
#include "aac_decoder.h"
#include "ringbuf.h"

#include "display_service.h"
#include "wifi_service.h"
#include "airkiss_config.h"
#include "smart_config.h"
#include "periph_adc_button.h"

#include "periph_spiffs.h"


static const char *TAG              = "DUEROS";
extern esp_audio_handle_t           player;

static audio_service_handle_t duer_serv_handle = NULL;
static display_service_handle_t disp_serv = NULL;
static periph_service_handle_t wifi_serv = NULL;
static bool wifi_setting_flag;
static char flag_test = 0;
static QueueHandle_t Queue_vad_play = NULL;
#define QUEUE_VAD_PLAY_LEN	2
#define QUEUE_VAD_PLAY_SIZE	1
#define QUEUE_VAD 1
#define QUEUE_PLAY 2


//static audio_element_handle_t i2s_stream_writer,fatfs_stream_reader,http_stream_reader,resample_for_play;
//static audio_element_handle_t fatfs_vad_writer,fatfs_vad_reader;
//static audio_pipeline_handle_t pipeline_play,pipeline_encode;

#define PIPELINE_PLAY 0

//#define	AMR_STREAM_URI "http://118.190.93.145:8027/upload/b/a/4/0/ba4029ef57944772e9508040a1394bf3.aac"
#define AMR_STREAM_URI "http://118.190.93.145:8027/upload/d/f/8/c/df8cb45e18b0e3bf933dcaef371739f9.amr"
//#define AMR_STREAM_URI "http://118.190.93.145:8027/upload/2/e/d/3/2ed3bca13328da669bd1e9d74e92c382.mp3" 
//#define AMR_STREAM_URI "file://sdcard/4800.wav"
//#define AMR_STREAM_URI "file://spiffs/test1.raw"

static int esp32_playback_voice(const char *url);


void rec_engine_cb(rec_event_type_t type, void *user_data)
{
	BaseType_t xReturn = pdFAIL;
	uint32_t senddata1 = QUEUE_VAD; 
    if (REC_EVENT_WAKEUP_START == type) {
        ESP_LOGI(TAG, "rec_engine_cb - REC_EVENT_WAKEUP_START");

		static char buf[1024];
		vTaskGetRunTimeStats(buf);
		printf("Run Time Stats:\nTask Name	  Time	  Percent\n%s\n", buf);

		vTaskList(buf);
		printf("Task List:\nTask Name	 Status   Prio	  HWM	 Task Number\n%s\n", buf);


    } else if (REC_EVENT_VAD_START == type) {
        ESP_LOGI(TAG, "rec_engine_cb - REC_EVENT_VAD_START");

		xReturn = xQueueSend(Queue_vad_play, &senddata1, 0);
		if(pdPASS == xReturn)printf("QUEUE_VAD sended successful\r\n");


    } else if (REC_EVENT_VAD_STOP == type) {
        ESP_LOGI(TAG, "rec_engine_cb - REC_EVENT_VAD_STOP");
		
    } else if (REC_EVENT_WAKEUP_END == type) {
        ESP_LOGI(TAG, "rec_engine_cb - REC_EVENT_WAKEUP_END");
		
		esp32_playback_voice(AMR_STREAM_URI);

		
    } else {
		ESP_LOGI(TAG, "ELSE...");
    }
}


int esp32_playback_voice(const char *url)
{
	int ret;
	BaseType_t xReturn = pdFAIL;
	uint32_t senddata2 = QUEUE_PLAY; 
	ret = duer_dcs_speak_handler(url);
	printf("ret =  %d\r\n",ret);
	if(ret == ESP_ERR_AUDIO_NO_ERROR){
		xReturn = xQueueSend(Queue_vad_play, &senddata2, 0);
		if(pdPASS == xReturn)printf("QUEUE_PLAY sended successful\r\n");
	}

	return ret;
}

/*
static audio_pipeline_handle_t create_vad_pipeline()
{
    audio_pipeline_handle_t pipeline;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

	fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
	fatfs_cfg.task_core = 0;
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_vad_reader = fatfs_stream_init(&fatfs_cfg);
	fatfs_cfg.type = AUDIO_STREAM_WRITER;
	fatfs_vad_writer = fatfs_stream_init(&fatfs_cfg);

	amrwb_encoder_cfg_t amrwb_cfg = DEFAULT_AMRWB_ENCODER_CONFIG();
	amrwb_cfg.task_core = 0;
   	audio_element_handle_t amrwb_encoder = amrwb_encoder_init(&amrwb_cfg);

	audio_pipeline_register(pipeline, fatfs_vad_reader, "file_r");
	audio_pipeline_register(pipeline, amrwb_encoder, "amrwb_en");
	audio_pipeline_register(pipeline, fatfs_vad_writer, "file_w");

	audio_pipeline_link(pipeline, (const char *[]) {"file_r", "amrwb_en","file_w"}, 3);
	audio_element_set_uri(fatfs_vad_reader, "/sdcard/test1.raw");
	audio_element_set_uri(fatfs_vad_writer, "/sdcard/test1.amr");

	return pipeline;
}
*/
static void vad_task(void * pram)
{
	BaseType_t xReturn = pdPASS;
	uint32_t r_queue = 0;	

	uint8_t *voiceData = audio_calloc(1, REC_ONE_BLOCK_SIZE);
	if (NULL == voiceData) {
        ESP_LOGE(TAG, "Func:%s, Line:%d, Malloc failed", __func__, __LINE__);
    }
	FILE *file = NULL;
	while(1){
		xReturn = xQueueReceive(Queue_vad_play, &r_queue, portMAX_DELAY);
		switch(r_queue){
			case QUEUE_VAD:
				printf("QUEUE_VAD\r\n");
				//file = fopen("/sdcard/test1.raw", "w+");	
				file = fopen("/spiffs/test.raw", "w+");
				if (NULL == file) {
					ESP_LOGW(TAG, "open test1.raw failed,[%d]", __LINE__);
				}		
				while(1){
					int ret = rec_engine_data_read(voiceData, REC_ONE_BLOCK_SIZE, 110 / portTICK_PERIOD_MS);
					ESP_LOGE(TAG, "index = %d", ret);
					if ((ret == 0) || (ret == -1)) {
						fclose(file);
						printf("fclose\r\n");
						
						
						break;
					}
					if (file) {
						fwrite(voiceData, 1, REC_ONE_BLOCK_SIZE, file);
					}	
				}		
				break;	
			case QUEUE_PLAY:
				
				printf("QUEUE_PLAY\r\n");
						
			
				break;	
			default :
				break;


		}
	}
	free(voiceData);
	voiceData = NULL;
}


static  audio_element_handle_t raw_read;

#ifdef CONFIG_ESP_LYRATD_MINI_V1_1_BOARD
static esp_err_t recorder_pipeline_open_for_mini(void **handle)
{
    audio_element_handle_t i2s_stream_reader;
    audio_pipeline_handle_t recorder;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    recorder = audio_pipeline_init(&pipeline_cfg);
    if (NULL == recorder) {
        return ESP_FAIL;
    }
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_port = 1;
    i2s_cfg.i2s_config.use_apll = 0;
    i2s_cfg.i2s_config.sample_rate = 16000;
    i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_cfg.type = AUDIO_STREAM_READER;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    raw_read = raw_stream_init(&raw_cfg);

    audio_pipeline_register(recorder, i2s_stream_reader, "i2s");
    audio_pipeline_register(recorder, raw_read, "raw");

    audio_pipeline_link(recorder, (const char *[]) {"i2s", "raw"}, 2);
    audio_pipeline_run(recorder);
    ESP_LOGI(TAG, "Recorder has been created");
    *handle = recorder;
    return ESP_OK;
}
#else
static esp_err_t recorder_pipeline_open(void **handle)
{
    audio_element_handle_t i2s_stream_reader;
    audio_pipeline_handle_t recorder;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    recorder = audio_pipeline_init(&pipeline_cfg);
    if (NULL == recorder) {
        return ESP_FAIL;
    }
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);
    audio_element_info_t i2s_info = {0};
    audio_element_getinfo(i2s_stream_reader, &i2s_info);
    i2s_info.bits = 16;
    i2s_info.channels = 2;
    i2s_info.sample_rates = 48000;
    audio_element_setinfo(i2s_stream_reader, &i2s_info);

    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 48000;
    rsp_cfg.src_ch = 2;
    rsp_cfg.dest_rate = 16000;
    rsp_cfg.dest_ch = 1;
    rsp_cfg.type = AUDIO_CODEC_TYPE_ENCODER;
    audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);

    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    raw_read = raw_stream_init(&raw_cfg);

    audio_pipeline_register(recorder, i2s_stream_reader, "i2s");
    audio_pipeline_register(recorder, filter, "filter");
    audio_pipeline_register(recorder, raw_read, "raw");
    audio_pipeline_link(recorder, (const char *[]) {"i2s", "filter", "raw"}, 3);
    audio_pipeline_run(recorder);
    ESP_LOGI(TAG, "Recorder has been created");
    *handle = recorder;
    return ESP_OK;
}
#endif

static esp_err_t recorder_pipeline_read(void *handle, char *data, int data_size)
{
    raw_stream_read(raw_read, data, data_size);
    return ESP_OK;
}

static esp_err_t recorder_pipeline_close(void *handle)
{
    audio_pipeline_deinit(handle);
    return ESP_OK;
}

static esp_err_t wifi_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    ESP_LOGD(TAG, "event type:%d,source:%p, data:%p,len:%d,ctx:%p",
             evt->type, evt->source, evt->data, evt->len, ctx);
    if (evt->type == WIFI_SERV_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "PERIPH_WIFI_CONNECTED [%d]", __LINE__);


        wifi_setting_flag = false;
    } else if (evt->type == WIFI_SERV_EVENT_DISCONNECTED) {
        ESP_LOGI(TAG, "PERIPH_WIFI_DISCONNECTED [%d]", __LINE__);
        

    } else if (evt->type == WIFI_SERV_EVENT_SETTING_TIMEOUT) {
        wifi_setting_flag = false;
    }

    return ESP_OK;
}

esp_err_t periph_callback(audio_event_iface_msg_t *event, void *context)
{
    ESP_LOGD(TAG, "Periph Event received: src_type:%x, source:%p cmd:%d, data:%p, data_len:%d",
             event->source_type, event->source, event->cmd, event->data, event->data_len);
    switch (event->source_type) {
        case PERIPH_ID_BUTTON: {
                if ((int)event->data == get_input_rec_id() && event->cmd == PERIPH_BUTTON_PRESSED) {
                    ESP_LOGI(TAG, "PERIPH_NOTIFY_KEY_REC");
                    rec_engine_trigger_start();
                } else if ((int)event->data == get_input_mode_id() &&
                           ((event->cmd == PERIPH_BUTTON_RELEASE) || (event->cmd == PERIPH_BUTTON_LONG_RELEASE))) {
                    ESP_LOGI(TAG, "PERIPH_NOTIFY_KEY_REC_QUIT");
                }
                break;
            }
        case PERIPH_ID_TOUCH: {
                if ((int)event->data == TOUCH_PAD_NUM4 && event->cmd == PERIPH_BUTTON_PRESSED) {

                    int player_volume = 0;
                    esp_audio_vol_get(player, &player_volume);
                    player_volume -= 10;
                    if (player_volume < 0) {
                        player_volume = 0;
                    }
                    esp_audio_vol_set(player, player_volume);
                    ESP_LOGI(TAG, "AUDIO_USER_KEY_VOL_DOWN [%d]", player_volume);
                } else if ((int)event->data == TOUCH_PAD_NUM4 && (event->cmd == PERIPH_BUTTON_RELEASE)) {


                } else if ((int)event->data == TOUCH_PAD_NUM7 && event->cmd == PERIPH_BUTTON_PRESSED) {
                    int player_volume = 0;
                    esp_audio_vol_get(player, &player_volume);
                    player_volume += 10;
                    if (player_volume > 100) {
                        player_volume = 100;
                    }
                    esp_audio_vol_set(player, player_volume);
                    ESP_LOGI(TAG, "AUDIO_USER_KEY_VOL_UP [%d]", player_volume);
                } else if ((int)event->data == TOUCH_PAD_NUM7 && (event->cmd == PERIPH_BUTTON_RELEASE)) {


                } else if ((int)event->data == TOUCH_PAD_NUM8 && event->cmd == PERIPH_BUTTON_PRESSED) {
                    ESP_LOGI(TAG, "AUDIO_USER_KEY_PLAY [%d]", __LINE__);

                } else if ((int)event->data == TOUCH_PAD_NUM8 && (event->cmd == PERIPH_BUTTON_RELEASE)) {


                } else if ((int)event->data == TOUCH_PAD_NUM9 && event->cmd == PERIPH_BUTTON_PRESSED) {
                    if (wifi_setting_flag == false) {
                        wifi_service_setting_start(wifi_serv, 0);
                        wifi_setting_flag = true;
                        display_service_set_pattern(disp_serv, DISPLAY_PATTERN_WIFI_SETTING, 0);
                        ESP_LOGI(TAG, "AUDIO_USER_KEY_WIFI_SET, WiFi setting started.");
                    } else {
                        ESP_LOGW(TAG, "AUDIO_USER_KEY_WIFI_SET, WiFi setting will be stopped.");
                        wifi_service_setting_stop(wifi_serv, 0);
                        wifi_setting_flag = false;
                        display_service_set_pattern(disp_serv, DISPLAY_PATTERN_TURN_OFF, 0);
                    }
                } else if ((int)event->data == TOUCH_PAD_NUM9 && (event->cmd == PERIPH_BUTTON_RELEASE)) {

                }
                break;
            }
        case PERIPH_ID_ADC_BTN:
            if (((int)event->data == get_input_volup_id()) && (event->cmd == PERIPH_ADC_BUTTON_RELEASE)) {
                int player_volume = 0;
                esp_audio_vol_get(player, &player_volume);
                player_volume += 10;
                if (player_volume > 100) {
                    player_volume = 100;
                }
                esp_audio_vol_set(player, player_volume);
                ESP_LOGI(TAG, "AUDIO_USER_KEY_VOL_UP [%d]", player_volume);
            } else if (((int)event->data == get_input_voldown_id()) && (event->cmd == PERIPH_ADC_BUTTON_RELEASE)) {
                int player_volume = 0;
                esp_audio_vol_get(player, &player_volume);
                player_volume -= 10;
                if (player_volume < 0) {
                    player_volume = 0;
                }
                esp_audio_vol_set(player, player_volume);
                ESP_LOGI(TAG, "AUDIO_USER_KEY_VOL_DOWN [%d]", player_volume);
            } else if (((int)event->data == get_input_play_id()) && (event->cmd == PERIPH_ADC_BUTTON_RELEASE)) {

            } else if (((int)event->data == get_input_set_id()) && (event->cmd == PERIPH_ADC_BUTTON_RELEASE)) {
                esp_audio_vol_set(player, 0);
                ESP_LOGI(TAG, "AUDIO_USER_KEY_VOL_MUTE [0]");
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

void duer_app_init(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "ADF version is %s", ADF_VER);

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

	if (set != NULL) {
        esp_periph_set_register_callback(set, periph_callback, NULL);
    }

    //audio_board_sdcard_init(set);

// Initialize Spiffs peripheral
    periph_spiffs_cfg_t spiffs_cfg = {
        .root = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_periph_handle_t spiffs_handle = periph_spiffs_init(&spiffs_cfg);

    // Start spiffs & button peripheral
    esp_periph_start(set, spiffs_handle);

    // Wait until spiffs is mounted
    while (!periph_spiffs_is_mounted(spiffs_handle)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    periph_wifi_cfg_t wifi_cfg = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    //periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);



/*
	wifi_config_t sta_cfg = {0};
	strncpy((char *)&sta_cfg.sta.ssid, CONFIG_WIFI_SSID, strlen(CONFIG_WIFI_SSID));
	strncpy((char *)&sta_cfg.sta.password, CONFIG_WIFI_PASSWORD, strlen(CONFIG_WIFI_PASSWORD));

	wifi_service_config_t cfg = WIFI_SERVICE_DEFAULT_CONFIG();
	cfg.evt_cb = wifi_service_cb;
	cfg.cb_ctx = NULL;
	cfg.setting_timeout_s = 60;
	wifi_serv = wifi_service_create(&cfg);

	int reg_idx = 0;
	esp_wifi_setting_handle_t h = NULL;
#ifdef CONFIG_AIRKISS_ENCRYPT
	airkiss_config_info_t air_info = AIRKISS_CONFIG_INFO_DEFAULT();
	air_info.lan_pack.appid = CONFIG_AIRKISS_APPID;
	air_info.lan_pack.deviceid = CONFIG_AIRKISS_DEVICEID;
	air_info.aes_key = CONFIG_DUER_AIRKISS_KEY;
	h = airkiss_config_create(&air_info);
#elif (defined CONFIG_ESP_SMARTCONFIG)
	smart_config_info_t info = SMART_CONFIG_INFO_DEFAULT();
	h = smart_config_create(&info);
#endif
	esp_wifi_setting_regitster_notify_handle(h, (void *)wifi_serv);
	wifi_service_register_setting_handle(wifi_serv, h, &reg_idx);
	wifi_service_set_sta_info(wifi_serv, &sta_cfg);
	wifi_service_connect(wifi_serv);
*/

    rec_config_t eng = DEFAULT_REC_ENGINE_CONFIG();
    eng.vad_off_delay_ms = 800;
    eng.wakeup_time_ms = 5 * 1000;
    eng.evt_cb = rec_engine_cb;
#ifdef CONFIG_ESP_LYRATD_MINI_V1_1_BOARD
    eng.open = recorder_pipeline_open_for_mini;
#else
    eng.open = recorder_pipeline_open;
#endif
    eng.close = recorder_pipeline_close;
    eng.fetch = recorder_pipeline_read;
    eng.extension = NULL;
    eng.support_encoding = false;
    eng.user_data = NULL;
	rec_engine_create(&eng);

	Queue_vad_play = xQueueCreate(QUEUE_VAD_PLAY_LEN, QUEUE_VAD_PLAY_SIZE);
	if(NULL != Queue_vad_play){
		printf("Queue_vad_play created successful\r\n");
	}
	
	xTaskCreate(vad_task, "vad_task", 4096, NULL, 3,NULL);

	duer_audio_wrapper_init();

 
}
