#include "test_framework.h"
#include "upe/upe_science_catalog.h"
#include "upe/upe_science_topics.h"

#include <vector>
#include <string.h>

TEST(TestScienceCatalogExpandedTopicCount) {
    uint32_t total = UpeScience_GetTopicCount();
    REQUIRE(total >= 282u);
    REQUIRE(UpeScience_GetTotalDeclaredTopicCount() == total);
}

TEST(TestScienceCatalogDomainVarietyFloorCounts) {
    REQUIRE(UpeScience_GetDomainTopicCount(UPE_SCIENCE_DOMAIN_MATH) >= 55u);
    REQUIRE(UpeScience_GetDomainTopicCount(UPE_SCIENCE_DOMAIN_ASTRONOMY) >= 37u);
    REQUIRE(UpeScience_GetDomainTopicCount(UPE_SCIENCE_DOMAIN_PHYSICS) >= 39u);
    REQUIRE(UpeScience_GetDomainTopicCount(UPE_SCIENCE_DOMAIN_STRING) >= 17u);
    REQUIRE(UpeScience_GetDomainTopicCount(UPE_SCIENCE_DOMAIN_SPACE) >= 25u);
    REQUIRE(UpeScience_GetDomainTopicCount(UPE_SCIENCE_DOMAIN_OPT) >= 56u);
}

TEST(TestScienceCatalogFindAdvancedTopics) {
    uint32_t kerr_count = UpeScience_FindTopicsContaining("Kerr spacetime", nullptr, 0);
    uint32_t brst_count = UpeScience_FindTopicsContaining("BRST cohomology", nullptr, 0);
    uint32_t dsn_count = UpeScience_FindTopicsContaining("DSN two-way ranging", nullptr, 0);

    REQUIRE(kerr_count >= 1u);
    REQUIRE(brst_count >= 1u);
    REQUIRE(dsn_count >= 1u);
}

TEST(TestScienceCatalogTopicByIdTailEntry) {
    UpeScienceTopicStatus topic = {};
    REQUIRE(UpeScience_GetTopicById(282u, &topic) == 1);
    REQUIRE(topic.domain == UPE_SCIENCE_DOMAIN_STRING);
    REQUIRE(topic.level == UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED);
    REQUIRE(strstr(topic.topic, "BRST cohomology") != nullptr);
}

TEST(TestScienceCatalogLevelConsistencyAfterExpansion) {
    uint32_t total = UpeScience_GetTopicCount();
    uint32_t not_impl = UpeScience_GetLevelTopicCount(UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED);
    uint32_t started = UpeScience_GetLevelTopicCount(UPE_SCIENCE_LEVEL_JUST_STARTED);
    uint32_t minimal = UpeScience_GetLevelTopicCount(UPE_SCIENCE_LEVEL_MINIMAL);
    uint32_t extreme = UpeScience_GetLevelTopicCount(UPE_SCIENCE_LEVEL_EXTREME);

    REQUIRE(total == (not_impl + started + minimal + extreme));
    REQUIRE(not_impl > 0u);
    REQUIRE(started > 0u);
    REQUIRE(minimal > 0u);
    REQUIRE(extreme > 0u);
}
