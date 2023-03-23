#ifndef IGUANA_NETSTAT_H
#define IGUANA_NETSTAT_H

#ifdef __cplusplus
extern "C" {
#endif
//
    void c_cnetworkstat_addnotary(const char *name, const char *pubkey);
    void c_cnetworkstat_updatestat(uint8_t senderind, uint32_t myipbits, int32_t packet_size);
    char *c_cnetworkstat_output();
//
#ifdef __cplusplus
} // extern "C"
#endif

#endif // IGUANA_NETSTAT_H