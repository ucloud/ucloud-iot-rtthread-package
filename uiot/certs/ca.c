/*
* Copyright (C) 2012-2019 UCloud. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "ca.h"

#include <stdlib.h>

#ifdef SUPPORT_TLS
static const char *iot_ca_crt = \
{
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDsjCCApoCCQCudOie27G3QDANBgkqhkiG9w0BAQsFADCBmjELMAkGA1UEBhMC\r\n"
    "Q04xETAPBgNVBAgMCFNoYW5naGFpMREwDwYDVQQHDAhTaGFuZ2hhaTEPMA0GA1UE\r\n"
    "CgwGVUNsb3VkMRgwFgYDVQQLDA9VQ2xvdWQgSW9ULUNvcmUxFjAUBgNVBAMMDXd3\r\n"
    "dy51Y2xvdWQuY24xIjAgBgkqhkiG9w0BCQEWE3Vpb3QtY29yZUB1Y2xvdWQuY24w\r\n"
    "HhcNMTkwNzI5MTIyMDQxWhcNMzkwNzI0MTIyMDQxWjCBmjELMAkGA1UEBhMCQ04x\r\n"
    "ETAPBgNVBAgMCFNoYW5naGFpMREwDwYDVQQHDAhTaGFuZ2hhaTEPMA0GA1UECgwG\r\n"
    "VUNsb3VkMRgwFgYDVQQLDA9VQ2xvdWQgSW9ULUNvcmUxFjAUBgNVBAMMDXd3dy51\r\n"
    "Y2xvdWQuY24xIjAgBgkqhkiG9w0BCQEWE3Vpb3QtY29yZUB1Y2xvdWQuY24wggEi\r\n"
    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC0g9bzkQBipln/nRsUKeQcwAkU\r\n"
    "ZUk/VMWrg5AzBislYW9hujCGZHWXUknVHPSQM2Pk30tPeIdmupSkOJllYtFplI6U\r\n"
    "6kqHpVsyPn925H5zz4uP5Ty0hkkNK+rIR/YjbEjGn8loTqWk++/o6qST5QOrZLUR\r\n"
    "vxaWvshpce0QUcYU9xMfUPzLa6/ZfmaNHgn1aWExYMJAWDyBCyw4OxeSMUyH+ydh\r\n"
    "egW7VHNQuVHOYdnXiC+VYImNJ8+QAyCIZ88lP0nqVPSKTt1XXmGW6vXrWSl+/XhO\r\n"
    "GaHNMzlwb1kqlFx/ZagTQoQ0hpmqSUKtqPgKSqGPxY9go1Rda1m2rYc8k3gJAgMB\r\n"
    "AAEwDQYJKoZIhvcNAQELBQADggEBAHU0KKqEbR7uoY1tlE+MDsfx/2zXNk9gYw44\r\n"
    "O+xGVdbvTC298Ko4uUEwc1C+l9FaMmN/2qUPSXWsSrAYDGMS63rzaxqNuADTgmo9\r\n"
    "QY0ITtTf0lZTkUahVSqAxzMFaaPzSfFeP9/EaUu14T5RPQbUZMVOAEPKDNmfK4rD\r\n"
    "06R6dnO12be4Qlha14o67+ojaNtyZ7/ESePXA/RjO9YMkeQAoa4BdnsJCZgCFmXf\r\n"
    "iKvGM+50+L/qSbH5F//byLGTO1t3TWCCdBE5/Mc/QLYEXDmZM6LMHyEAw4VuinIa\r\n"
    "I8m1P/ceVO0RjNNBG0pDH9PH4OA7ikY81c63PBCQaYMKaiksCzs=\r\n"
    "-----END CERTIFICATE-----\r\n"
};


static const char *iot_https_ca_crt = \
{
    "-----BEGIN CERTIFICATE-----\r\n"
    "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\r\n"
    "-----END CERTIFICATE-----\r\n"
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIErjCCA5agAwIBAgIQBYAmfwbylVM0jhwYWl7uLjANBgkqhkiG9w0BAQsFADBh\r\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
    "QTAeFw0xNzEyMDgxMjI4MjZaFw0yNzEyMDgxMjI4MjZaMHIxCzAJBgNVBAYTAkNO\r\n"
    "MSUwIwYDVQQKExxUcnVzdEFzaWEgVGVjaG5vbG9naWVzLCBJbmMuMR0wGwYDVQQL\r\n"
    "ExREb21haW4gVmFsaWRhdGVkIFNTTDEdMBsGA1UEAxMUVHJ1c3RBc2lhIFRMUyBS\r\n"
    "U0EgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCgWa9X+ph+wAm8\r\n"
    "Yh1Fk1MjKbQ5QwBOOKVaZR/OfCh+F6f93u7vZHGcUU/lvVGgUQnbzJhR1UV2epJa\r\n"
    "e+m7cxnXIKdD0/VS9btAgwJszGFvwoqXeaCqFoP71wPmXjjUwLT70+qvX4hdyYfO\r\n"
    "JcjeTz5QKtg8zQwxaK9x4JT9CoOmoVdVhEBAiD3DwR5fFgOHDwwGxdJWVBvktnoA\r\n"
    "zjdTLXDdbSVC5jZ0u8oq9BiTDv7jAlsB5F8aZgvSZDOQeFrwaOTbKWSEInEhnchK\r\n"
    "ZTD1dz6aBlk1xGEI5PZWAnVAba/ofH33ktymaTDsE6xRDnW97pDkimCRak6CEbfe\r\n"
    "3dXw6OV5AgMBAAGjggFPMIIBSzAdBgNVHQ4EFgQUf9OZ86BHDjEAVlYijrfMnt3K\r\n"
    "AYowHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUwDgYDVR0PAQH/BAQD\r\n"
    "AgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAG\r\n"
    "AQH/AgEAMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYYaHR0cDovL29jc3Au\r\n"
    "ZGlnaWNlcnQuY29tMEIGA1UdHwQ7MDkwN6A1oDOGMWh0dHA6Ly9jcmwzLmRpZ2lj\r\n"
    "ZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwTAYDVR0gBEUwQzA3Bglg\r\n"
    "hkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQuY29t\r\n"
    "L0NQUzAIBgZngQwBAgEwDQYJKoZIhvcNAQELBQADggEBAK3dVOj5dlv4MzK2i233\r\n"
    "lDYvyJ3slFY2X2HKTYGte8nbK6i5/fsDImMYihAkp6VaNY/en8WZ5qcrQPVLuJrJ\r\n"
    "DSXT04NnMeZOQDUoj/NHAmdfCBB/h1bZ5OGK6Sf1h5Yx/5wR4f3TUoPgGlnU7EuP\r\n"
    "ISLNdMRiDrXntcImDAiRvkh5GJuH4YCVE6XEntqaNIgGkRwxKSgnU3Id3iuFbW9F\r\n"
    "UQ9Qqtb1GX91AJ7i4153TikGgYCdwYkBURD8gSVe8OAco6IfZOYt/TEwii1Ivi1C\r\n"
    "qnuUlWpsF1LdQNIdfbW3TSe0BhQa7ifbVIfvPWHYOu3rkg1ZeMo6XRU9B4n5VyJY\r\n"
    "RmE=\r\n"
    "-----END CERTIFICATE-----"
};
#endif

const char *iot_ca_get() {
#ifdef SUPPORT_TLS
    return iot_ca_crt;
#else
    return NULL;
#endif
}

const char *iot_https_ca_get() {
#ifdef SUPPORT_TLS
    return iot_https_ca_crt;
#else
    return NULL;
#endif
}

#ifdef __cplusplus
}
#endif


