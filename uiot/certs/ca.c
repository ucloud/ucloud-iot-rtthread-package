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
    "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\r\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
    "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\r\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\r\n"
    "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\r\n"
    "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\r\n"
    "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\r\n"
    "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\r\n"
    "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\r\n"
    "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\r\n"
    "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\r\n"
    "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\r\n"
    "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\r\n"
    "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\r\n"
    "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\r\n"
    "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\r\n"
    "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\r\n"
    "-----END CERTIFICATE-----\r\n"
};

const char *iot_ca_get() {
    return iot_ca_crt;
}

const char *iot_https_ca_get() {
    return iot_https_ca_crt;
}

#ifdef __cplusplus
}
#endif


