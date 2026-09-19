#pragma once

#include <windows.h>
#include <cstdint>

typedef enum
{
    WINDIVERT_LAYER_NETWORK = 0,
    WINDIVERT_LAYER_NETWORK_FORWARD = 1,
    WINDIVERT_LAYER_FLOW = 2,
    WINDIVERT_LAYER_SOCKET = 3,
    WINDIVERT_LAYER_REFLECT = 4
} WINDIVERT_LAYER, *PWINDIVERT_LAYER;

typedef struct
{
    uint32_t IfIdx;
    uint32_t SubIfIdx;
} WINDIVERT_DATA_NETWORK, *PWINDIVERT_DATA_NETWORK;

typedef struct
{
    uint64_t EndpointId;
    uint64_t ParentEndpointId;
    uint32_t ProcessId;
    uint32_t LocalAddr[4];
    uint32_t RemoteAddr[4];
    uint16_t LocalPort;
    uint16_t RemotePort;
    uint8_t  Protocol;
} WINDIVERT_DATA_FLOW, *PWINDIVERT_DATA_FLOW;

typedef struct
{
    uint64_t EndpointId;
    uint64_t ParentEndpointId;
    uint32_t ProcessId;
    uint32_t LocalAddr[4];
    uint32_t RemoteAddr[4];
    uint16_t LocalPort;
    uint16_t RemotePort;
    uint8_t  Protocol;
} WINDIVERT_DATA_SOCKET, *PWINDIVERT_DATA_SOCKET;

typedef struct
{
    int64_t  Timestamp;
    uint32_t ProcessId;
    WINDIVERT_LAYER Layer;
    uint64_t Flags;
    int16_t  Priority;
} WINDIVERT_DATA_REFLECT, *PWINDIVERT_DATA_REFLECT;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)
#endif
typedef struct
{
    int64_t  Timestamp;
    uint32_t Layer:8;
    uint32_t Event:8;
    uint32_t Sniffed:1;
    uint32_t Outbound:1;
    uint32_t Loopback:1;
    uint32_t Impostor:1;
    uint32_t IPv6:1;
    uint32_t IPChecksum:1;
    uint32_t TCPChecksum:1;
    uint32_t UDPChecksum:1;
    uint32_t Reserved1:8;
    uint32_t Reserved2;
    union
    {
        WINDIVERT_DATA_NETWORK Network;
        WINDIVERT_DATA_FLOW Flow;
        WINDIVERT_DATA_SOCKET Socket;
        WINDIVERT_DATA_REFLECT Reflect;
        uint8_t Reserved3[64];
    };
} WINDIVERT_ADDRESS, *PWINDIVERT_ADDRESS;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define WINDIVERT_DIRECTION_OUTBOUND 0
#define WINDIVERT_DIRECTION_INBOUND 1

#define WINDIVERT_FLAG_SNIFF 1
#define WINDIVERT_FLAG_DROP 2
#define WINDIVERT_FLAG_DEBUG 4

typedef enum
{
    WINDIVERT_PARAM_QUEUE_LEN = 0,
    WINDIVERT_PARAM_QUEUE_TIME = 1,
    WINDIVERT_PARAM_QUEUE_SIZE = 2
} WINDIVERT_PARAM;

typedef struct
{
    uint8_t  HdrLength:4;
    uint8_t  Version:4;
    uint8_t  TOS;
    uint16_t Length;
    uint16_t Id;
    uint16_t FragOff0;
    uint8_t  TTL;
    uint8_t  Protocol;
    uint16_t Checksum;
    uint32_t SrcAddr;
    uint32_t DstAddr;
} WINDIVERT_IPHDR, *PWINDIVERT_IPHDR;

#define WINDIVERT_IPHDR_GET_FRAGOFF(hdr) (((hdr)->FragOff0) & 0xFF1F)
#define WINDIVERT_IPHDR_GET_MF(hdr) ((((hdr)->FragOff0) & 0x0020) != 0)
#define WINDIVERT_IPHDR_GET_DF(hdr) ((((hdr)->FragOff0) & 0x0040) != 0)

typedef struct
{
    uint8_t  TrafficClass0:4;
    uint8_t  Version:4;
    uint8_t  FlowLabel0:4;
    uint8_t  TrafficClass1:4;
    uint16_t FlowLabel1;
    uint16_t Length;
    uint8_t  NextHdr;
    uint8_t  HopLimit;
    uint32_t SrcAddr[4];
    uint32_t DstAddr[4];
} WINDIVERT_IPV6HDR, *PWINDIVERT_IPV6HDR;

typedef struct
{
    uint8_t  Type;
    uint8_t  Code;
    uint16_t Checksum;
    uint32_t Body;
} WINDIVERT_ICMPHDR, *PWINDIVERT_ICMPHDR;

typedef struct
{
    uint8_t  Type;
    uint8_t  Code;
    uint16_t Checksum;
    uint32_t Body;
} WINDIVERT_ICMPV6HDR, *PWINDIVERT_ICMPV6HDR;

typedef struct
{
    uint16_t SrcPort;
    uint16_t DstPort;
    uint32_t SeqNum;
    uint32_t AckNum;
    uint16_t Reserved1:4;
    uint16_t HdrLength:4;
    uint16_t Fin:1;
    uint16_t Syn:1;
    uint16_t Rst:1;
    uint16_t Psh:1;
    uint16_t Ack:1;
    uint16_t Urg:1;
    uint16_t Reserved2:2;
    uint16_t Window;
    uint16_t Checksum;
    uint16_t UrgPtr;
} WINDIVERT_TCPHDR, *PWINDIVERT_TCPHDR;

typedef struct
{
    uint16_t SrcPort;
    uint16_t DstPort;
    uint16_t Length;
    uint16_t Checksum;
} WINDIVERT_UDPHDR, *PWINDIVERT_UDPHDR;

#define WINDIVERT_HELPER_NO_IP_CHECKSUM 1
#define WINDIVERT_HELPER_NO_ICMP_CHECKSUM 2
#define WINDIVERT_HELPER_NO_ICMPV6_CHECKSUM 4
#define WINDIVERT_HELPER_NO_TCP_CHECKSUM 8
#define WINDIVERT_HELPER_NO_UDP_CHECKSUM 16

typedef HANDLE (WINAPI *pfnWinDivertOpen)(
    const char *filter,
    WINDIVERT_LAYER layer,
    int16_t priority,
    uint64_t flags
);

typedef BOOL (WINAPI *pfnWinDivertRecv)(
    HANDLE handle,
    PVOID pPacket,
    UINT packetLen,
    UINT *pRecvLen,
    PWINDIVERT_ADDRESS pAddr
);

typedef BOOL (WINAPI *pfnWinDivertSend)(
    HANDLE handle,
    const VOID *pPacket,
    UINT packetLen,
    UINT *pSendLen,
    const WINDIVERT_ADDRESS *pAddr
);

typedef BOOL (WINAPI *pfnWinDivertClose)(
    HANDLE handle
);

typedef BOOL (WINAPI *pfnWinDivertSetParam)(
    HANDLE handle,
    WINDIVERT_PARAM param,
    uint64_t value
);

typedef BOOL (WINAPI *pfnWinDivertHelperParsePacket)(
    const VOID *pPacket,
    UINT packetLen,
    PWINDIVERT_IPHDR *ppIpHdr,
    PWINDIVERT_IPV6HDR *ppIpv6Hdr,
    UINT8 *pProtocol,
    PWINDIVERT_ICMPHDR *ppIcmpHdr,
    PWINDIVERT_ICMPV6HDR *ppIcmpv6Hdr,
    PWINDIVERT_TCPHDR *ppTcpHdr,
    PWINDIVERT_UDPHDR *ppUdpHdr,
    PVOID *ppData,
    UINT *pDataLen,
    PVOID *ppNext,
    UINT *pNextLen
);

typedef BOOL (WINAPI *pfnWinDivertHelperCalcChecksums)(
    VOID *pPacket,
    UINT packetLen,
    WINDIVERT_ADDRESS *pAddr,
    uint64_t flags
);
