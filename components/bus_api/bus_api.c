#include "bus_api.h"

#include <stdio.h>
#include <time.h>

#include "bus_api_config.h"
#include "cJSON.h"
#include "jsmn.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"

static const char* TAG = "bus_api";
static char response_buffer[1024];  // Buffer to store the response

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                strncat(response_buffer, (char*)evt->data, evt->data_len);

                // Print the data

                // Parse the response
                // cJSON *root = cJSON_Parse(response_buffer);
                // if (root == NULL) {
                    // ESP_LOGE(TAG, "Error parsing JSON");
                    // break;
                // }else {
                    
                // }

                // cJSON *plannedDepartures = cJSON_GetObjectItem(root, 
                // "PlannedDepartures");
                // if (plannedDepartures != NULL) {
                    // cJSON *plannedDepartureTime = cJSON_GetObjectItem(plannedDepartures,
                    // "PlannedDepartureTime");
                // }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGI(TAG, "Default");
            break;
    }
    return ESP_OK;
}

// get_current_time
// Returns the current time in the format "YYYY-MM-DDTHH:MM:SS.ZZZZZZZZZ+HH:MM"
static const char* get_current_time(void) {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    static char datetime[64];
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);

    // print the current time
    ESP_LOGI(TAG, "Current time: %s", datetime);
    return datetime;
}


char* get_bus_departures(void) {

    esp_http_client_config_t config = {
    .url = API_URL,
    .cert_pem = NULL,
    .event_handler = _http_event_handle,
    };

    char post_data[256];
    snprintf(post_data, sizeof(post_data),
             "{\"stopID\":%d,"
             "\"platformNumber\":%d,"
             "\"fromDateTime\":\"%s\","
             "\"maxMinutes\":%d,"
             "\"maxDepartures\":%d}",
             STOP_ID,
             PLATFORM_NUMBER,
             get_current_time(),
             MAX_MINUTES,
             MAX_DEPARTURES);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-API-Key", API_KEY);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    memset(response_buffer, 0, sizeof(response_buffer));  // Clear the buffer

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        printf("Response: %s\n", response_buffer);
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return strdup(response_buffer);  // Return a copy of the response
}