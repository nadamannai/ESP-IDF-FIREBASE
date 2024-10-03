
#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_camera.h"
#include "driver/i2c_master.h"
#include <esp_event.h>
#include <esp_system.h>
#include <esp_https_server.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "base64.h"
#include <cJSON.h>







//#define MOUNT_POINT "/sdcard"
#define EXAMPLE_ESP_WIFI_SSID "Aizen"
#define EXAMPLE_ESP_WIFI_PASS "nadouna3ne3i"

//static const char *TAG2 = "sdcard";
static const char *TAG = "HTTPS";

static const char *TAG2 = "Camera";

#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_SCL_IO           27                // replace with appropriate GPIO pin
#define I2C_MASTER_SDA_IO           26                // replace with appropriate GPIO pin
#define I2C_MASTER_FREQ_HZ          400000            // I2C clock frequency

// ESP32Cam (AiThinker) PIN Map

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
//#define CAM_PIN_SIOD 26
//#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22


static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    //.pin_sscb_sda = CAM_PIN_SIOD,
    //.pin_sscb_scl = CAM_PIN_SIOC,
    .pin_sscb_sda = I2C_MASTER_SDA_IO,
    .pin_sscb_scl = I2C_MASTER_SCL_IO,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 19, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};

static void i2c_master_init() {
    i2c_master_bus_config_t conf ={
        //conf.i2c_mode_t = I2C_MODE_MASTER;
         conf.sda_io_num = I2C_MASTER_SDA_IO,
        //conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
         conf.flags.enable_internal_pullup = 1,
         conf.scl_io_num = I2C_MASTER_SCL_IO,
         // conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
         conf.clk_source = I2C_MASTER_FREQ_HZ};

    //i2c_param_config(I2C_MASTER_NUM, &conf);
   // i2c_driver_install(I2C_MASTER_NUM, conf.i2c_mode_t, 0, 0, 0);
}
static esp_err_t init_camera()
{
    //initialize the camera
    i2c_master_init();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG2, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}



//SDCARD

/*static void init_sdcard()
{
  esp_err_t ret = ESP_FAIL;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };
  sdmmc_card_t *card;

  const char mount_point[] = MOUNT_POINT;
  ESP_LOGI(TAG2, "Initializing SD card");

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  ESP_LOGI(TAG2, "Mounting SD card...");
  gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
  gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
  gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
  gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
  gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG2, "SD card mount successfully!");
  }
  else
  {
    ESP_LOGE(TAG2, "Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
  }


}*/

// WiFi


static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static const char *TAG1 = "wifi station";

//static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
       // if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
		//{
            esp_wifi_connect();
           // s_retry_num++;
            ESP_LOGI(TAG1, "retry to connect to the AP");
        //} else {
        //    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGI(TAG1,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG1, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        //s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    WIFI_EVENT_STA_DISCONNECTED,
                    &event_handler,
                    NULL,
                    NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG1, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG1, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

// Server
static esp_err_t server_get_handler(httpd_req_t *req)
{
    const char resp[] = "Server GET Response .................";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t server_post_handler(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);

    // If no data is send the error will be:
    // W (88470) httpd_uri: httpd_uri: uri handler execution failed
    printf("\nServer POST content: %s\n", content);

    if (ret <= 0)
    { /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    /* Send a simple response */
    const char resp[] = "Server POST Response .................";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static const httpd_uri_t server_uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = server_get_handler
};

static const httpd_uri_t server_uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = server_post_handler
};

static httpd_handle_t start_webserver(void)
{
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");
    
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    httpd_handle_t server = NULL;
    
    extern const unsigned char servercert_pem_start[] asm("_binary_servercert_pem_start");
    extern const unsigned char servercert_pem_end[]   asm("_binary_servercert_pem_end");
    config.servercert = servercert_pem_start;
    config.servercert_len = servercert_pem_end - servercert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end"); 
    config.prvtkey_pem = prvtkey_pem_start;
    config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;
    
    esp_err_t ret = httpd_ssl_start(&server, &config);
    if (ESP_OK != ret)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &server_uri_get);
    httpd_register_uri_handler(server, &server_uri_post);
    return server;
}

// Client
esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("Client HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}

static void client_post_rest_function(char* post_data)
{
    extern const unsigned char certificate_pem_start[] asm("_binary_certificate_pem_start");

    esp_http_client_config_t config_post = {
        .url = "https://firestore.googleapis.com/v1/projects/esp-idf-fire-ae4d1/databases/(default)/documents/my_collection",
        .method = HTTP_METHOD_POST,
        .cert_pem = (const char *)certificate_pem_start,
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    
    //char* post_data = "{\"fields\":{\"Name\":{\"stringValue\":\"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\"}}}";
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}
void app_main(void)
{
    
    int i;
    init_camera();
    camera_fb_t *pic;
    ESP_LOGI(TAG2, "Taking picture...");
    for(i=0;i<6;i++)
    {
     pic = esp_camera_fb_get();
     esp_camera_fb_return(pic);}
    
    if(!pic) {
       printf("Camera capture failed");
    }
    
   
    
   
    const char *input= (char *)pic->buf;
    int lenim=pic->len;

    char *imb64 = b64_encode(input, lenim);
   
   //char* encoded_image="/9j/4AAQSkZJRgABAQEAYABgAAD/2wBDAAoHBwkHBgoJCAkLCwoMDxkQDw4ODx4WFxIZJCAmJSMgIyIoLTkwKCo2KyIjMkQyNjs9QEBAJjBGS0U+Sjk/QD3/2wBDAQsLCw8NDx0QEB09KSMpPT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT3/wAARCAC0ALQDASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwDlwcU7NMzRmrLuPzRuptJmgQ/dSbqbmkzQBe07Tb7V52h062kuHQbn2kAKO2SSAKde6RqWnZ+22F1CB/E0ZK/99DI/WvRPANh9g8LxTMMSXjmc5HO08L/46AfxrphJjoaRVjwhXDDKkH6Gl3GvaL3QdI1Ik3mnWzuf4wm1v++hg1g3nw10ybJsrq5tW/usRKv68/rTFY813GjJrq734c6xb5Ns9tdr22vsb8m4/WuevtK1DTT/AKdY3MAH8Txnb/30OP1oCxWyaTJpoYEZBBHsaM0CHZNGTSZpM80AONJmjNFABmgUUUAFBpM0maAHCimg0UAMzg0bqb3ooAduo3U2igB2als7R9Sv7eyjPz3MixA+gJ5P4DJ/CoAa634caf8AademvWHyWcWFP+2/A/JQ350AeklY4YkiiG2ONQigdgBgU6FGmTI4qGZj0Hfir9uvlwqKRRAUkTtn6UnmlfvAir2aCqt1AoAqLNUomBGPWnNaxt04qJrNxyjZoAoXvhrRNTJa6063Ln+NF2N+a4NYF78MLCXJsb64tz2WQCRf6H9a6spNH1U/hQs5Bw3FMLHmd78Odbtcm3FveL/0yfa35Nj+dc/e6bfaacX1ncW/vJGQPz6frXuKzj1qQSBlIPI7g9DSFY8ABDDIII9RS17TfeFdD1IlrjTYN5/jjHlt+a4rmtX+GdvHZzT6XdTiSNS4hmIYNjnAbAIP1zRcVjzukJxS9aYc0wELU3PvS4oxQAA0UoWigBtLinhaXZQBHijFSkU3FAEeK9U8B6f9g8KwyuMS3jG4bPXB4Uf98gfnXmlpZPqN7b2Uf3rmRYgfQE8n8Bk/hXtZVIYljiULGihVA7AcCkxobGPMuFHYc1oiqVmv3n9TVwUDHilFMBpwoAfRSdaWgBwNIURvvKDSUooAia0jbplagYLC+1myKuk4GT2rKlbzLgntQBcSXPCj8apeIr8ab4fvLgn5vLKr9TwP51bgHtXG/EvUNttbWKnBdjIw9h0/U/pSA84xik2mp1TNPEVUIq7DS7Ks+Vijy6BFbZRVoRCigCqOtLTtmKXFAxuKTGak20mKQHT/AA9077Trst4w+Szi+X/ffgf+Ohvzr0KY9u9YvgfT/sPhiGVhiS8Y3DfQ8L/46B+dbar5k4HYc0DRahTZGBUgpop3akAopwpnrSNngdqYEw6UtRR5Gc1IDyKAHAUtNB4FOoAiuZNkJPc1nQjJz61PfyZIQUyFelJgW4htUk9q8g8Y6j/aHia6KnKQnyl/Dr+pNeqavfLpmjXN03SOMt9eK8PXdK7O5y7ksx9SetNCZMh6VZTmoESp0piQEelJUoGRSbKBjBRUgSigRSC04JUmynBTSKI9mamtNPbULyC0TrcSCPPoD1P4DJpVTNdP4IsPN1eW7YfLbR4X/fbj+QP50AdwVSKJUjGERQqgdgOBRbL1b1psxJ49anjXaoHoKQDxTqZTxQAtL1popwpgLgYxjil6fjSUv40AOHagtgZo61FcvtjNAFCVvMnNWIV5FVohk5PerkQA59KQHI/EvUPJ0qGxQ/NcPyP9leT+uK8/t4cgVueNLw6l4qlRTmO2URD69T/MflVAtHZRLNNGzwoymVVODsyN2D/u5pksRbc0k/l20DTTMEjXGT1yT0AHcn0rpPEcWi2EUT6bIpWMETCPfJ9DuPU5449RXmuoX91ql8uyNhJvEVvB0IdjgD6560JiudTGFkQMhDA+n8vrUnlmty08Jx2OiApfCaWKPJTCqWfq+cnI6k/zrO8sEcUFFUJRVsR0UAZfl09U9ql2+1O2igYxUxXf+FLP7HoMbsMSXJMzZ9D939AK4MgFSGzg9cdcd69Oimint0a0ZGh2jaVOeO1AEdzNs5zyKjj1YDhwR9RTpICSc1Xe29qQGlFfRSdCKsLIrdCK51rYqcrkfShZ7iE8NuHoaAOlFKKw4tXK8SAir8OpRyD7wpgX6WolmRujU8GgB4NU7188VZLYGaoStvm+lADoR0qS7uVsrCa4kOFjQsfoBmiNelc/48vPI0NbVThrlwmPbqf5Y/GkB5/ah7qeS4k5eVy7H3JzVu7tRcWUsJJAdCpx6EVLawhEHFWtlMkp+HrldY0GW3fe97ZwmK4ToSQCEYH3wOfWk8FWV5q/iSDUNSieaHTYSsU7MWzMyqTknrjJwO2RWXrWlbb6G8iYrE58m6XsY3+UtjvjdmtfQ/Es3h/T5IJkWaCO5MMKxISZDtDM2fqwA9yBUvRj5VJXtr/w503iS9toYvIgWI3Ur7ZGC/Mq9SM+/FYQFGoXv2y/luHAEbH90o6YGc49ee/cg+1Ni37AXGCe3pTjK4Ncr5SQAUUooqgKG3ml20/FLipGRFc9qpXM15aN5llczW8g/ijbH59jWnimtEr/AHgCKYFWx+I+uaeQl/bwahGP4h+6k/McH8q6bTviRoF+VS5lksJT/DcrtXPswyv6iuYm0mGXJA2ms250JsHChxQI9fiMN1EJbeRJI26OjAg/iKY1t14rzn4e+G3fxOZFeaG1tozJLGjsqSseFUgHGOp/4DXeeJNRHhvRbrVI90kNsATC7Z3kkDAJ6dffpSC497UHtVd7Mg5HH0rF0n4p+HdTCrcvLYyNxidfl/76GR+eK6y3kt76ETWs0U0TdHjYMPzFA9zOSS4hPDZHoasxas6cSKfwqw9sPSoWtc9qYi2uqRyJ159BRH8zbvWs8W3lvuArRtxlRSGWoxXA+Lrn7b4iWEHKWqY/4E3J/TFd48iwQPI/CqpJPtXmEUjXdxNdv96dzJ+B6fpihAWEXAp+KVRS1YitfWwubKeLHLxsB9cVSRheWsKW7JK7qJy4J2JvQAnHuVOPqegzm7fyCHTbqU9Ehdv/AB01neG4zHa4IwVt7dPx8vcf/Qqlq41Jq6RoQ2vlAFmZ27k1OBinGkotYkBRSrk0UAUx1p1IKWkUFKKKUUwAClxSinAUAIuvyeGA95DFHIJMI6PkZABPBHIPBrD8R+NLrxtZW+nWumy2tuJBLOXcNuI+6AcDjv8AlSeLXI0/YO0TsfxKr/U1L4dsFSxSVuWb9KLEsojw+TCFeJSMdKqpo1zp03naZdXFnL6wuVz9R0NdqFGKa0CP1FAWMSy8f+JtHwt9DDqUI6kjy5PzHH6V1Gl/FLQb4iO+87TpjwRcL8uf94cfnisqTT1bPH4Vl3egQTqd8Q574oDU9Vt57W/hEtpPFPEejxuGH5ip402HHavC/wDhHbrTpvP0m7ntZB3icqf0rWsfH3izRyEvYYtSiHd12v8A99L/AFFILnpPi+8NtoTxKSHuCIhj0PX9M1yECbVA7VAde1LxLKk9/brbxR/6uFMnB9ST1q4gwKaGKKXNFFUMy/EbN/YN1Gn35wsC/V2C/wBan05V/wBKdPuNcuE/3Vwg/wDQap65cLHc6ejfdid72T/diUkfmxAq/psDW2m28T/fWMF/948n9SaTEWePSmmlpKAAUUCigCpnml3VHnmlzUjJQaUVFmnA9u1MCUGnA1GM4p2aAMHxYc20w5P+jxjP1m/+tWzpaCPT4Fx0QVkeJgXt7kDqbIsP+ASqT+jVr6a4ewgYdCgoF1LoGaWmg04GmAuKQgGlBooAieBG7c1CbUjoat0mfagCJI9vWn9KWk7+9IAoA7DkmjNUdUupILdYrVlF3ckxwknheMs59lXJP4VQGRMRqesvjmOaUW6e8MR3Sn6M+1fwrpSc81h+H7eMq13EpFvsEFqG6+Sp+8fdmyx/CtompBATSUZppNA7DxRTAeOtFMLFLdS1Hup26pAeKcD71HuoL0wJg1OBz0qvv96dvoAraqqFrR5TiEu1tK3osq7c/g22meHJmbS0il4lhJjkB7FTg/yqa8gS+sZraQ/LKhUn09D9QaxtMvnt7zfcfK0zCG5H9y4A4P0dRkH13UAdaDS55qBZARTt/wCVMZNu5pc9qhD0vmDFArEhNGeai8wZ5pQ/SgLEmaQmmb6juLmK2iEk77FLBAAMlmPRVA5Yn0FMQ64uYrW3knuJBHDGMs57f4nsB3Nc1M0+sapLakNG8ihbkZ/49oM5EP8AvseW9OnY02/1S5vtTW3tFBu4zmNMhks+3mORw0voOie7cja0vTotLtFhiyzH5nduWdj1JNS2IuoqxxqiAKqjAAHQClLUwtTS/NIoeW96buqMv0pN+aYEof3oqMGigCmDQTxUQegvSAl30hfiot4NLuFMCXd70que9QbuKUNQBPvxWVqto4ka7gjWbcnlz25OBOnXGezDqD2IrQDUoOaAKemawhgXfKZLfO1Lhxgg/wByUfwP+h7GtfzCDg9R2rCutKbzmutPl+z3LDaxxlJR/ddTww+tUxqk2m/LdW8lkB3iUzWx+ikhk/Bse1AXsdQZSKQzj1rDj1u3nTieykB7x3RjP5SKP5mnHV4lH/LuPd72LH6ZP6Ux3NrzyfxpwlIBbOFUZYk4A9ye1cvN4kgRtq3EBbstvG87H6Fti/oajY6rqpXybNkXtNfsGI91jwFH/fP40riubt5r0Fvb+ZG0ZQ9J5SRF/wABx80h9l49xWNHcajrkxazMsUbDa97KMSFT1WJRxGp9uT3Y9Ks2vhqITC51GaS9uT1aU5A+grbXCjGOnpSuK1yHS9NttKtxFbrt7lj1Y+pq9u4qDOSKN/agdiYk96jL8U1pKaW5oAcWyO9NLUh47VEWpgWA/vRVcPkdaKAK3PrQSRRRQAD731p2eKKKADpinjk0UUAANOU8UUUAOXp1pxUMvPNFFAys+jafcPmW0hYnqdgz+dOh8N6SCT9hiOMdRmiigTRp2+n2tqgW3t44x/sKB/KiVApGO9FFA0QOMAkVGDmiikAp4I5prdSM0UUAGTzTu4oopgDDHc1A1FFAhlFFFAH/9k=";
   //char *post_im = (char *)malloc(strlen("{\"Name\":{\"stringValue\":\"")+strlen(encoded_image));
   //snprintf(post_im, strlen("{\"fields\":{\"Name\":{\"stringValue\":\"") + strlen(encoded_image),encoded_image);
   
   
   
   
   const char *json_prefix = "{\"fields\":{\"image\":{\"stringValue\":\"";
   const char *json_suffix = "\"}}}";
   
   size_t json_payload_len = strlen(json_prefix) + strlen(imb64)+ strlen(json_suffix) + 1;
   char *json_payload = (char *)malloc(json_payload_len);
   snprintf(json_payload, json_payload_len, "%s%s%s", json_prefix, imb64, json_suffix);
    //printf("json:%s",json_payload);
    // Print the JSON payload
   // printf("{\"image\":\"%s\",\"description\":\"Example Image\"}", encoded_image);
   //init_sdcard();
    // Read image data from a file 

    //FILE *imageFile = fopen("/sdcard/pic_1.jpg", "rb");
    //printf("image ouverte en mode binaire.....");
    //size_t image_size=ftell(imageFile);
    //fclose(imageFile);

  
   
    //sprintf(post_data, "{\"image\":\"%s\"}", base64_data);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");

    printf("Start server:\n\n");
    start_webserver();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Start client:\n\n");
    client_post_rest_function(json_payload);
    free(json_payload);
}
