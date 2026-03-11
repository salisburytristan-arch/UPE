#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UpeScienceDomain {
    UPE_SCIENCE_DOMAIN_MATH = 0,
    UPE_SCIENCE_DOMAIN_CHEM = 1,
    UPE_SCIENCE_DOMAIN_ASTROLOGY = 2,
    UPE_SCIENCE_DOMAIN_ASTRONOMY = 3,
    UPE_SCIENCE_DOMAIN_PHYSICS = 4,
    UPE_SCIENCE_DOMAIN_STRING = 5,
    UPE_SCIENCE_DOMAIN_SPACE = 6,
    UPE_SCIENCE_DOMAIN_OPT = 7,
    UPE_SCIENCE_DOMAIN_COUNT = 8
} UpeScienceDomain;

typedef enum UpeScienceImplementationLevel {
    UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED = 0,
    UPE_SCIENCE_LEVEL_JUST_STARTED = 1,
    UPE_SCIENCE_LEVEL_MINIMAL = 2,
    UPE_SCIENCE_LEVEL_EXTREME = 3
} UpeScienceImplementationLevel;

typedef struct UpeScienceDomainStatus {
    UpeScienceDomain domain;
    UpeScienceImplementationLevel level;
    const char *domain_key;
    const char *summary;
    uint32_t declared_topic_count;
    uint32_t implemented_topic_count;
} UpeScienceDomainStatus;

uint32_t UpeScience_GetDomainStatusCount(void);
uint32_t UpeScience_GetDomainStatuses(UpeScienceDomainStatus *out_statuses, uint32_t max_statuses);
int UpeScience_GetDomainStatus(UpeScienceDomain domain, UpeScienceDomainStatus *out_status);

uint32_t UpeScience_GetTotalDeclaredTopicCount(void);
uint32_t UpeScience_GetTotalImplementedTopicCount(void);
float UpeScience_GetOverallImplementationRatio(void);

const char *UpeScience_DomainToString(UpeScienceDomain domain);
const char *UpeScience_LevelToString(UpeScienceImplementationLevel level);

#ifdef __cplusplus
}
#endif
