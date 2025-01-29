#include "bus_api.h"

#include <stdio.h>
#include <time.h>

#include "bus_api_config.h"
#include "departure_info.h"
// #include "cJSON.h"
// #include "./managed_components/include/jsmn.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "jsmn.h"

#define MAX_RESPONSE_SIZE 8192  // Increased from 2048
#define MAX_TOKENS 512          // Increased from 256

static const char *TAG = "bus_api";
static char response_buffer[MAX_RESPONSE_SIZE];  // Increased buffer size
static size_t response_idx = 0;                  // Track response buffer position

static departure_info_t current_departure;

esp_err_t _http_event_handle(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
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
            printf("%.*s", evt->data_len, (char *)evt->data);
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
static const char *get_current_time(void) {
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

static void print_token(const char *json, jsmntok_t *token) {
    int len = token->end - token->start;
    if (len > 0) {
        ESP_LOGI(TAG, "%.*s", len, json + token->start);
    }
}

static int is_primitive(const char *json, jsmntok_t *token) {
    if (token->type != JSMN_PRIMITIVE) return 0;
    char c = json[token->start];
    return (c == 'n' || c == 't' || 'f' || c == '-' || (c >= '0' && c <= '9'));
}

static void print_value(const char *json, jsmntok_t *token) {
    if (!json || !token || token->start < 0 || token->end < token->start) {
        return;
    }
    int len = token->end - token->start;
    if (len > 0 && len < MAX_RESPONSE_SIZE) {
        ESP_LOGI(TAG, "%.*s", len, json + token->start);
    }
}

// Add this helper function before parse_json_response
static char *get_string_value(const char *json_string, jsmntok_t *token) {
    int len = token->end - token->start;
    char *value = calloc(len + 1, sizeof(char));  // Use calloc instead of malloc
    if (value) {
        strncpy(value, json_string + token->start, len);
        value[len] = '\0';  // Ensure null termination
    }
    return value;
}

static int parse_json_response(const char *json_string) {
    static jsmn_parser parser;
    static jsmntok_t tokens[MAX_TOKENS];

    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, json_string, strlen(json_string), tokens, MAX_TOKENS);

    if (token_count < 0) {
        ESP_LOGE(TAG, "JSON parsing failed with error: %d", token_count);
        return token_count;
    }

    ESP_LOGI(TAG, "Successfully parsed JSON with %d tokens", token_count);

    // Initialize departure info with default values
    memset(&current_departure, 0, sizeof(departure_info_t));
    current_departure.delay_minutes = 0;
    current_departure.platform_number = -1;

    // Find PlannedDepartures array
    for (int i = 0; i < token_count; i++) {
        if (jsoneq(json_string, &tokens[i], "PlannedDepartures") == 0 &&
            tokens[i + 1].type == JSMN_ARRAY) {
            // Process first departure (tokens[i + 2] is the first departure object)
            int departure_obj = i + 2;

            // Iterate through departure object
            for (int j = departure_obj; j < token_count - 1; j++) {
                if (tokens[j].type == JSMN_STRING) {
                    // Check for our target fields
                    if (jsoneq(json_string, &tokens[j], "PlannedDepartureTime") == 0) {
                        char *time = get_string_value(json_string, &tokens[j + 1]);
                        if (time) {
                            strncpy(current_departure.planned_departure_time, time, sizeof(current_departure.planned_departure_time) - 1);
                            free(time);
                        }
                    } else if (jsoneq(json_string, &tokens[j], "DelayMinutes") == 0) {
                        char *delay = get_string_value(json_string, &tokens[j + 1]);
                        if (delay) {
                            current_departure.delay_minutes = atoi(delay);
                            free(delay);
                        }
                    } else if (jsoneq(json_string, &tokens[j], "PlatformNumber") == 0) {
                        char *platform = get_string_value(json_string, &tokens[j + 1]);
                        if (platform) {
                            current_departure.platform_number = atoi(platform);
                            free(platform);
                        }
                    } else if (jsoneq(json_string, &tokens[j], "Destination") == 0) {
                        char *dest = get_string_value(json_string, &tokens[j + 1]);
                        if (dest) {
                            strncpy(current_departure.destination, dest, sizeof(current_departure.destination) - 1);
                            free(dest);
                        }
                    }
                }
            }
            break;  // We only process the first departure
        }
    }

    return token_count;
}

departure_info_t* get_bus_departures(void) {
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
            esp_http_client_cleanup(client);
            return NULL;
        }

        esp_http_client_cleanup(client);
        return &current_departure;
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return NULL;
    }
}