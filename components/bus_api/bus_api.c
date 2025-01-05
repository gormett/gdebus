#include "bus_api.h"

#include <stdio.h>
#include <time.h>

#include "bus_api_config.h"
// #include "cJSON.h"
// #include "./managed_components/include/jsmn.h"
#include "jsmn.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"

#define MAX_RESPONSE_SIZE 8192  // Increased from 2048
#define MAX_TOKENS 512         // Increased from 256

static const char* TAG = "bus_api";
static char response_buffer[MAX_RESPONSE_SIZE];  // Increased buffer size
static size_t response_idx = 0;  // Track response buffer position

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
                // Safe concatenation with boundary check
                if (response_idx + evt->data_len < MAX_RESPONSE_SIZE - 1) {
                    memcpy(response_buffer + response_idx, evt->data, evt->data_len);
                    response_idx += evt->data_len;
                    response_buffer[response_idx] = 0;  // Null terminate
                } else {
                    ESP_LOGE(TAG, "Response buffer overflow prevented");
                    return ESP_FAIL;
                }
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

// Helper function to extract token content
static void print_token_value(const char *json, jsmntok_t *token) {
    if (token->type == JSMN_STRING || token->type == JSMN_PRIMITIVE) {
        printf("%.*s", token->end - token->start, json + token->start);
    }
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && 
        (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

static int parse_json_response(const char* json_string) {
    static jsmn_parser parser;  // Make static to move from stack to global
    static jsmntok_t tokens[MAX_TOKENS]; // Make static to move from stack
    
    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, json_string, strlen(json_string), tokens, MAX_TOKENS);
    
    if (token_count < 0) {
        switch (token_count) {
            case JSMN_ERROR_INVAL:
                ESP_LOGE(TAG, "JSON parsing error: invalid JSON string");
                break;
            case JSMN_ERROR_NOMEM:
                ESP_LOGE(TAG, "JSON parsing error: not enough tokens");
                break;
            case JSMN_ERROR_PART:
                ESP_LOGE(TAG, "JSON parsing error: incomplete JSON string");
                break;
            default:
                ESP_LOGE(TAG, "JSON parsing error: unknown error");
        }
        return token_count;
    }

    ESP_LOGI(TAG, "Successfully parsed JSON with %d tokens", token_count);

    // Verify we have a root object
    if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
        ESP_LOGE(TAG, "Expected root object");
        return -1;
    }

    // Parse through tokens
    for (int i = 1; i < token_count; i++) {
        if (tokens[i].type == JSMN_STRING) {
            // This is potentially a key
            int next = i + 1;
            if (next < token_count) {
                printf("Key: %.*s = ", 
                    tokens[i].end - tokens[i].start,
                    json_string + tokens[i].start);
                
                // Handle different value types
                switch(tokens[next].type) {
                    case JSMN_STRING:
                    case JSMN_PRIMITIVE:
                        print_token_value(json_string, &tokens[next]);
                        break;
                    case JSMN_ARRAY:
                        printf("[Array with %d elements]", tokens[next].size);
                        break;
                    case JSMN_OBJECT:
                        printf("{Object with %d members}", tokens[next].size);
                        break;
                    case JSMN_UNDEFINED:
                    default:
                        printf("Undefined");
                        break;
                }
                printf("\n");
                
                // Skip the value token and its children
                i = next + tokens[next].size;
            }
        }
    }
    
    return token_count;
}

char* get_bus_departures(void) {
    // Reset response buffer index
    response_idx = 0;
    memset(response_buffer, 0, MAX_RESPONSE_SIZE);

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
        
        // Parse the JSON response
        int parse_result = parse_json_response(response_buffer);
        if (parse_result < 0) {
            ESP_LOGE(TAG, "Failed to parse JSON response");
        }
        
        printf("Response: %s\n", response_buffer);
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return strdup(response_buffer);  // Return a copy of the response
}