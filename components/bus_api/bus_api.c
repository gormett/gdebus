#include "bus_api.h"
#include <stdio.h>
#include <time.h>
#include "bus_api_config.h"
#include "esp_tls.h" 
#include "esp_http_client.h"
#include "esp_log.h"

static const char* TAG = "bus_api";


const char* get_bus_departures(void) {
    esp_http_client_config_t config = {
        .url = API_URL,
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_SSL, // Enable HTTPS
        .cert_pem = NULL, // Use default certificate bundle
        .skip_cert_common_name_check = false
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Get the current date and time
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char datetime[64];
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);

    char post_data[256];
    snprintf(post_data, sizeof(post_data),
             "{\"stopID\":%d,"
             "\"platformNumber\":%d,"
             "\"fromDateTime\":\"%s\","
             "\"maxMinutes\":%d,"  
             "\"maxDepartures\":%d}",
             STOP_ID,
             PLATFORM_NUMBER, 
             datetime,
             MAX_MINUTES,
             MAX_DEPARTURES);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-API-Key", API_KEY);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    static char response_buffer[1024];

    // Print data that is sent to the server
    ESP_LOGI(TAG, "POST data: %s", post_data);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
                 
        esp_http_client_read(client, response_buffer, sizeof(response_buffer));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        snprintf(response_buffer, sizeof(response_buffer), "{\"error\":\"%s\"}", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return response_buffer;
}