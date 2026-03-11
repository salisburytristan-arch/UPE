#pragma once

#include <stdint.h>
#include "upe/upe_science_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeScienceTopicStatus {
    uint32_t id;
    UpeScienceDomain domain;
    UpeScienceImplementationLevel level;
    const char *topic;
} UpeScienceTopicStatus;

uint32_t UpeScience_GetTopicCount(void);
uint32_t UpeScience_GetTopics(UpeScienceTopicStatus *out_topics, uint32_t max_topics);
uint32_t UpeScience_GetTopicsByDomain(UpeScienceDomain domain, UpeScienceTopicStatus *out_topics, uint32_t max_topics);
uint32_t UpeScience_GetTopicsByLevel(UpeScienceImplementationLevel level, UpeScienceTopicStatus *out_topics, uint32_t max_topics);
uint32_t UpeScience_GetTopicsByDomainAndLevel(UpeScienceDomain domain,
                                              UpeScienceImplementationLevel level,
                                              UpeScienceTopicStatus *out_topics,
                                              uint32_t max_topics);
uint32_t UpeScience_FindTopicsContaining(const char *substring, UpeScienceTopicStatus *out_topics, uint32_t max_topics);
int UpeScience_GetTopicById(uint32_t id, UpeScienceTopicStatus *out_topic);

uint32_t UpeScience_GetDomainTopicCount(UpeScienceDomain domain);
uint32_t UpeScience_GetDomainImplementedTopicCount(UpeScienceDomain domain);
uint32_t UpeScience_GetLevelTopicCount(UpeScienceImplementationLevel level);

#ifdef __cplusplus
}
#endif
