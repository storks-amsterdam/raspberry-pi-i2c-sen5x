/*
 * I2C-Generator: 0.3.0
 * Yaml Version: 2.1.3
 * Template Version: 0.7.0-109-gb259776
 */
/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <curl/curl.h>
#include "/opt/homebrew/opt/json-c/include/json-c/json.h" 

#include <math.h>   // NAN
#include <stdio.h>  // printf

#include "sen5x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

/*
 * TO USE CONSOLE OUTPUT (PRINTF) YOU MAY NEED TO ADAPT THE INCLUDE ABOVE OR
 * DEFINE IT ACCORDING TO YOUR PLATFORM:
 * #define printf(...)
 */


void send_data (int sensorID, time_t measurement_timestamp,
                float mass_concentration_pm1p0, float mass_concentration_pm2p5, 
                float mass_concentration_pm4p0, float mass_concentration_pm10p0, 
                float ambient_humidity, float ambient_temperature, 
                float voc_index, float nox_index)
{
    // Create a JSON object
    struct json_object *message_obj = json_object_new_array();
    json_object_array_add(message_obj, json_object_new_int(sensorID));
    json_object_array_add(message_obj, json_object_new_int64((int64_t)measurement_timestamp));
    json_object_array_add(message_obj, json_object_new_double(mass_concentration_pm1p0));
    json_object_array_add(message_obj, json_object_new_double(mass_concentration_pm2p5));
    json_object_array_add(message_obj, json_object_new_double(mass_concentration_pm4p0));
    json_object_array_add(message_obj, json_object_new_double(mass_concentration_pm10p0));
    json_object_array_add(message_obj, json_object_new_double(ambient_humidity));
    json_object_array_add(message_obj, json_object_new_double(ambient_temperature));
    json_object_array_add(message_obj, json_object_new_double(voc_index));
    json_object_array_add(message_obj, json_object_new_double(nox_index));

    // Convert the JSON object to a JSON string
    const char *json_str = json_object_to_json_string(message_obj);

    // Initialize libcurl
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Error initializing libcurl\n");
        return;
    }

    // Define the URL
    const char *myurl = "https://europe-west1-storks-app-dev.cloudfunctions.net/storks-app-dev-insert-to-pg";

    // Set the libcurl options
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, myurl);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);

    // Send the HTTP POST request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } 
    else
    {
        printf("Telemetry sent\n");
    }

    // Cleanup libcurl and JSON object
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    json_object_put(message_obj);
}


int main(void) {
    int16_t error = 0;

    sensirion_i2c_hal_init();

    error = sen5x_device_reset();
    if (error) {
        printf("Error executing sen5x_device_reset(): %i\n", error);
    }

    unsigned char serial_number[32];
    uint8_t serial_number_size = 32;
    error = sen5x_get_serial_number(serial_number, serial_number_size);
    if (error) {
        printf("Error executing sen5x_get_serial_number(): %i\n", error);
    } else {
        printf("Serial number: %s\n", serial_number);
    }

    unsigned char product_name[32];
    uint8_t product_name_size = 32;
    error = sen5x_get_product_name(product_name, product_name_size);
    if (error) {
        printf("Error executing sen5x_get_product_name(): %i\n", error);
    } else {
        printf("Product name: %s\n", product_name);
    }

    uint8_t firmware_major;
    uint8_t firmware_minor;
    bool firmware_debug;
    uint8_t hardware_major;
    uint8_t hardware_minor;
    uint8_t protocol_major;
    uint8_t protocol_minor;
    error = sen5x_get_version(&firmware_major, &firmware_minor, &firmware_debug,
                              &hardware_major, &hardware_minor, &protocol_major,
                              &protocol_minor);
    if (error) {
        printf("Error executing sen5x_get_version(): %i\n", error);
    } else {
        printf("Firmware: %u.%u, Hardware: %u.%u\n", firmware_major,
               firmware_minor, hardware_major, hardware_minor);
    }

    // set a temperature offset in degrees celsius
    // Note: supported by SEN54 and SEN55 sensors
    // By default, the temperature and humidity outputs from the sensor
    // are compensated for the modules self-heating. If the module is
    // designed into a device, the temperature compensation might need
    // to be adapted to incorporate the change in thermal coupling and
    // self-heating of other device components.
    //
    // A guide to achieve optimal performance, including references
    // to mechanical design-in examples can be found in the app note
    // “SEN5x – Temperature Compensation Instruction” at www.sensirion.com.
    // Please refer to those application notes for further information
    // on the advanced compensation settings used in
    // `sen5x_set_temperature_offset_parameters`,
    // `sen5x_set_warm_start_parameter` and `sen5x_set_rht_acceleration_mode`.
    //
    // Adjust temp_offset to account for additional temperature offsets
    // exceeding the SEN module's self heating.
    float temp_offset = 0.0f;
    error = sen5x_set_temperature_offset_simple(temp_offset);
    if (error) {
        printf("Error executing sen5x_set_temperature_offset_simple(): %i\n",
               error);
    } else {
        printf("Temperature Offset set to %.2f °C (SEN54/SEN55 only)\n",
               temp_offset);
    }

    // Start Measurement
    error = sen5x_start_measurement();

    if (error) {
        printf("Error executing sen5x_start_measurement(): %i\n", error);
    }

    for (int i = 0; i < 60; i++) {
        // Read Measurement
        sensirion_i2c_hal_sleep_usec(1000000); // 1s

        int sensorID = 1;
        time_t measurement_timestamp = time(NULL);

        float mass_concentration_pm1p0;
        float mass_concentration_pm2p5;
        float mass_concentration_pm4p0;
        float mass_concentration_pm10p0;
        float ambient_humidity;
        float ambient_temperature;
        float voc_index;
        float nox_index;

        error = sen5x_read_measured_values(
            &mass_concentration_pm1p0, &mass_concentration_pm2p5,
            &mass_concentration_pm4p0, &mass_concentration_pm10p0,
            &ambient_humidity, &ambient_temperature, &voc_index, &nox_index);
        if (error) {
            printf("Error executing sen5x_read_measured_values(): %i\n", error);
        } else {

            printf("Successfully collected sample.\n");
            send_data(sensorID, 
                      measurement_timestamp, 
                      mass_concentration_pm1p0,
                      mass_concentration_pm2p5, 
                      mass_concentration_pm4p0, 
                      mass_concentration_pm10p0, 
                      ambient_humidity, 
                      ambient_temperature, 
                      voc_index, 
                      nox_index);
            // printf("Mass concentration pm1p0: %.1f µg/m³\n",
            //        mass_concentration_pm1p0);
            // printf("Mass concentration pm2p5: %.1f µg/m³\n",
            //        mass_concentration_pm2p5);
            // printf("Mass concentration pm4p0: %.1f µg/m³\n",
            //        mass_concentration_pm4p0);
            // printf("Mass concentration pm10p0: %.1f µg/m³\n",
            //        mass_concentration_pm10p0);
            // if (isnan(ambient_humidity)) {
            //     printf("Ambient humidity: n/a\n");
            // } else {
            //     printf("Ambient humidity: %.1f %%RH\n", ambient_humidity);
            // }
            // if (isnan(ambient_temperature)) {
            //     printf("Ambient temperature: n/a\n");
            // } else {
            //     printf("Ambient temperature: %.1f °C\n", ambient_temperature);
            // }
            // if (isnan(voc_index)) {
            //     printf("Voc index: n/a\n");
            // } else {
            //     printf("Voc index: %.1f\n", voc_index);
            // }
            // if (isnan(nox_index)) {
            //     printf("Nox index: n/a\n");
            // } else {
            //     printf("Nox index: %.1f\n", nox_index);
            // }
        }
    }

    error = sen5x_stop_measurement();
    if (error) {
        printf("Error executing sen5x_stop_measurement(): %i\n", error);
    }

    return 0;
}
