/**
 * @file netstat.cpp
 * @author Decker (decker@komodoplatform.com)
 * @brief Network statistics and utilities module for iguana, provide interface from C++ to C.
 * @version 0.1
 * @date 2023-03-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <iostream>
#include <cstdint>
#include <vector>
#include <set>
#include <mutex>
#include <chrono>
#include <ctime>
#include <cstring>
#include "netstat.h"
#include "tinyformat.h"
// #include "../includes/iguana_funcs.h"
// #include "../crypto777/OS_portable.h"
#include "../includes/cJSON.h"

/*
    - https://stackoverflow.com/questions/199418/using-c-library-in-c-code
    - https://isocpp.org/wiki/faq/mixing-c-and-cpp
    - https://stackoverflow.com/questions/6045809/link-error-undefined-reference-to-gxx-personality-v0-and-g

*/

int64_t GetTime()
{
    time_t now = time(nullptr);
    assert(now > 0);
    return now;
}

int64_t GetTimeMicros()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t GetSystemTimeInSeconds()
{
    return GetTimeMicros()/1000000;
}

std::string FormatISO8601DateTime(int64_t nTime) {
    struct tm ts;
    time_t time_val = nTime;
    // if (gmtime_s(&ts, &time_val) != 0) {
    if (gmtime_r(&time_val, &ts) == nullptr) {
        return {};
    }
    return strprintf("%04i-%02i-%02iT%02i:%02i:%02iZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}

class CNotary {
    public:
    std::string name;
    std::string pubkey_hex;
    std::set<uint32_t> ips;
    uint64_t packets;
    uint64_t bytes;
    int64_t init_time;
    int64_t last_packet_time;
    double last_pps;

    CNotary() = default; // force generation of a default constructor
    CNotary(const char *_name, const char *_pubkey) : CNotary() {
        name = _name; pubkey_hex = _pubkey;
        init_time = GetTimeMicros();
    }
};

class CNetworkStat
{
private:
    std::vector<CNotary> notaries;
    mutable std::mutex mtx;
public:
    CNetworkStat() {
        notaries.reserve(64);
    };
    ~CNetworkStat() {};
    void AddNotary(const char *name, const char *pubkey);
    bool UpdateNotary(uint8_t senderind, uint32_t myipbits, int32_t packet_size);
    const CNotary& GetNotary(size_t index) {
        std::lock_guard<std::mutex> lock(mtx);
        return notaries[index];
    }
    size_t GetNotariesCount() const;
} instance_of_cnetworkstat;

void CNetworkStat::AddNotary(const char *name, const char *pubkey)
{
    std::lock_guard<std::mutex> lock(mtx);
    CNotary notary(name, pubkey);
    notaries.push_back(notary);
    // std::cerr << notary.name << ": " << notary.pubkey_hex << " - " << FormatISO8601DateTime(notary.init_time/1000000) << std::endl;
}

bool CNetworkStat::UpdateNotary(uint8_t senderind, uint32_t myipbits, int32_t packet_size)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (senderind < notaries.size()) {
        const int64_t current_time = GetTimeMicros();
        const auto time_diff = std::max(current_time - notaries[senderind].last_packet_time, (int64_t) 0);
        notaries[senderind].last_pps = 1.0 * 1000000 / time_diff;
        notaries[senderind].last_packet_time = GetTimeMicros();
        notaries[senderind].packets++;
        if (notaries[senderind].ips.count(myipbits) == 0) {
            notaries[senderind].ips.insert(myipbits);
        }
        notaries[senderind].bytes += packet_size;

        return true;

    } else {
        return false;
    }
}

size_t CNetworkStat::GetNotariesCount() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return notaries.size();
}

std::string IPv4ToString(uint32_t ip)
{
    //return strprintf("%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >>  8) & 0xFF, (ip      ) & 0xFF);
    return strprintf("%u.%u.%u.%u", (ip      ) & 0xFF, (ip >>  8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
}

// ---

void c_cnetworkstat_addnotary(const char *name, const char *pubkey) {
    instance_of_cnetworkstat.AddNotary(name, pubkey);
}

void c_cnetworkstat_updatestat(uint8_t senderind, uint32_t myipbits, int32_t packet_size) {
    bool fAdded = instance_of_cnetworkstat.UpdateNotary(senderind, myipbits, packet_size);
}

char *c_cnetworkstat_output() {

    cJSON *json = cJSON_CreateObject();
    char *json_str = nullptr;

    if (json) {

        cJSON *value = nullptr;
        cJSON *notaries_array = nullptr;
        cJSON *notary = nullptr;

        cJSON *name = nullptr; cJSON *ips = nullptr; cJSON *ip = nullptr;
        cJSON *packets = nullptr; cJSON *bytes = nullptr;
        cJSON *init_time = nullptr; cJSON *last_packet_time = nullptr;
        cJSON *last_pps = nullptr;

        value = cJSON_CreateString("success");
        if (value) {
            cJSON_AddItemToObject(json, "result", value);
            notaries_array = cJSON_CreateArray();
            if (notaries_array) {
                cJSON_AddItemToObject(json, "notaries", notaries_array);
                for (int i = 0; i < instance_of_cnetworkstat.GetNotariesCount(); ++i) {
                    CNotary nn = instance_of_cnetworkstat.GetNotary(i);
                    notary = cJSON_CreateObject();
                    if (notary) {
                        cJSON_AddItemToArray(notaries_array, notary);

                        name = cJSON_CreateString(nn.name.c_str());
                        ips = cJSON_CreateArray();
                        packets = cJSON_CreateNumber(nn.packets);
                        bytes = cJSON_CreateNumber(nn.bytes);
                        init_time = cJSON_CreateString(FormatISO8601DateTime(nn.init_time/1000000).c_str());
                        last_packet_time = cJSON_CreateString(FormatISO8601DateTime(nn.last_packet_time/1000000).c_str());
                        last_pps = cJSON_CreateNumber(nn.last_pps);

                        if (name && ips && packets && bytes && init_time && last_packet_time && last_pps) {
                            cJSON_AddItemToObject(notary, "name", name);
                            cJSON_AddItemToObject(notary, "ips", ips);

                            for (const uint32_t elem : nn.ips) {
                                std::string ip_str = IPv4ToString(elem);
                                ip = cJSON_CreateString(ip_str.c_str());
                                if (ip) {
                                    cJSON_AddItemToArray(ips, ip);
                                }
                            }

                            cJSON_AddItemToObject(notary, "packets", packets);
                            cJSON_AddItemToObject(notary, "bytes", bytes);
                            cJSON_AddItemToObject(notary, "init_time", init_time);
                            cJSON_AddItemToObject(notary, "last_packet_time", last_packet_time);
                            cJSON_AddItemToObject(notary, "last_pps", last_pps);

                        }
                    }
                }
            }

            json_str = cJSON_Print(json);
            cJSON_Delete(json);
        }
    }

    if (!json_str)
    {
        std::string res = "{\"result\":\"failed\"}";
        size_t ret_size = res.size() + 1;
        json_str = (char*)malloc(ret_size);
        strncpy(json_str, res.c_str(), ret_size);
    }

    return json_str;
}