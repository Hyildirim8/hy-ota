#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <string.h>

// ---- WiFi Ayarları ----
#define WIFI_SSID      "Plaza_Aurora"
#define WIFI_PASS      "34Plaza34*-"

// ---- OTA Ayarları ----
#define OTA_FIRMWARE_URL  "http://10.42.101.19:8000/firmware"
#define OTA_SIGNATURE_URL "http://10.42.101.19:8000/signature"

// ---- PUBLIC KEY ----
#define PUBKEY_PEM "-----BEGIN PUBLIC KEY-----\n" \
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqG28ekTRqFi3nSt//rA+\n" \
"BwdTdgulXMIkRxMRZfyZug2QXoKubl7xbmSPEnKjgsGUDmeG7yciv70nSGxXEEvw\n" \
"GzSm6us4wUI7/315BkLfIWDhCbXfeBA7yDRpaGmhS+0C3nSI77lwZR8RPV+rEae+\n" \
"SdIGeaeUbx1UidZuywniNTDjNojgNX16jd7JVF5n3DHgi88OdqctBsr0ZaJi5Jce\n" \
"GBn//PhU4hSb5dDsMskEXHWpuBS6mqijHXnPUO6k8JABXw3tYy8BLxFF1s3MFJkE\n" \
"wPhW1G8XHhLb3t8QH66pxIqGw7CKthOLW55/MUmS1/4TmGrXrnDp1sbydLZtNXDv\n" \
"fQIDAQAB\n" \
"-----END PUBLIC KEY-----\n"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "mbedtls/pk.h"
#include "mbedtls/md.h"

// ---- WiFi Bağlantı Mantığı ----
static const char *TAG = "secure_ota";
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    EventGroupHandle_t wifi_event_group = (EventGroupHandle_t)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*)event_data;
        ESP_LOGW(TAG, "Disconnect reason: %d", disconn->reason);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_connect(void)
{
    wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, wifi_event_group);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, wifi_event_group);

    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi bağlanıyor...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi bağlantısı BAŞARILI!");
}

// ---- OTA Yardımcı Fonksiyonlar ----

esp_err_t download_signature(uint8_t **sig_buf, int *sig_len) {
    esp_http_client_config_t config = {.url = OTA_SIGNATURE_URL,};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (esp_http_client_open(client, 0) != ESP_OK) return ESP_FAIL;
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) return ESP_FAIL;
    *sig_buf = malloc(content_length);
    *sig_len = esp_http_client_read(client, (char *)(*sig_buf), content_length);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (*sig_len != content_length) return ESP_FAIL;
    return ESP_OK;
}

esp_err_t download_firmware(uint8_t **fw_buf, int *fw_len) {
    esp_http_client_config_t config = {.url = OTA_FIRMWARE_URL,};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (esp_http_client_open(client, 0) != ESP_OK) return ESP_FAIL;
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) return ESP_FAIL;
    *fw_buf = malloc(content_length);
    *fw_len = esp_http_client_read(client, (char *)(*fw_buf), content_length);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (*fw_len != content_length) return ESP_FAIL;
    return ESP_OK;
}

esp_err_t verify_signature(const uint8_t *fw_data, int fw_len, const uint8_t *sig, int sig_len) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    int ret = mbedtls_pk_parse_public_key(&pk, (const unsigned char *)PUBKEY_PEM, strlen(PUBKEY_PEM) + 1);
    if (ret != 0) {
        ESP_LOGE(TAG, "Public key parse error: -0x%04x", -ret);
        return ESP_FAIL;
    }
    unsigned char hash[32];
    mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), fw_data, fw_len, hash);
    ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, sizeof(hash), sig, sig_len);
    mbedtls_pk_free(&pk);
    if (ret != 0) {
        ESP_LOGE(TAG, "Signature verification failed: -0x%04x", -ret);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Signature verified!");
    return ESP_OK;
}

void secure_ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting SECURE OTA task");

    uint8_t *sig_buf = NULL;
    int sig_len = 0;
    if (download_signature(&sig_buf, &sig_len) != ESP_OK) {
        ESP_LOGE(TAG, "Signature indirilemedi!");
        vTaskDelete(NULL);
    }

    uint8_t *fw_buf = NULL;
    int fw_len = 0;
    if (download_firmware(&fw_buf, &fw_len) != ESP_OK) {
        ESP_LOGE(TAG, "Firmware indirilemedi!");
        free(sig_buf);
        vTaskDelete(NULL);
    }

    if (verify_signature(fw_buf, fw_len, sig_buf, sig_len) == ESP_OK) {
        ESP_LOGI(TAG, "İmza doğru! OTA başlatılıyor.");
        esp_ota_handle_t update_handle = 0;
        const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
        ESP_ERROR_CHECK(esp_ota_begin(update_partition, fw_len, &update_handle));
        ESP_ERROR_CHECK(esp_ota_write(update_handle, fw_buf, fw_len));
        ESP_ERROR_CHECK(esp_ota_end(update_handle));
        ESP_ERROR_CHECK(esp_ota_set_boot_partition(update_partition));
        ESP_LOGI(TAG, "Güncelleme tamamlandı! Reset atılıyor...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "İmza HATALI, güncelleme iptal!");
    }

    free(fw_buf);
    free(sig_buf);
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "Güvenli OTA başlıyor...");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    wifi_connect();
    xTaskCreate(&secure_ota_task, "secure_ota_task", 16384, NULL, 5, NULL);

    // Sonsuz döngüde bekle (opsiyonel, tasklar için)
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
