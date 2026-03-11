#include "test_framework.h"
#include "upe/upe_science_export.h"
#include "upe/upe_science_catalog.h"

#include <string.h>
#include <vector>

static uint32_t CountOccurrences(const char *text, const char *needle) {
    if (!text || !needle || needle[0] == '\0') {
        return 0;
    }
    uint32_t count = 0;
    const char *cursor = text;
    const size_t nlen = strlen(needle);
    while ((cursor = strstr(cursor, needle)) != nullptr) {
        ++count;
        cursor += nlen;
    }
    return count;
}

TEST(TestScienceExportDefaultJson) {
    uint32_t required = UpeScience_WriteCatalogJson(nullptr, 0, 1);
    REQUIRE(required > 0);

    std::vector<char> json(required + 1u, '\0');
    uint32_t produced = UpeScience_WriteCatalogJson(json.data(), (uint32_t)json.size(), 1);
    REQUIRE(produced == required);

    REQUIRE(strstr(json.data(), "\"schema\":\"upe.science.catalog.v1\"") != nullptr);
    REQUIRE(strstr(json.data(), "\"domains\":[") != nullptr);
    REQUIRE(strstr(json.data(), "\"totals\":") != nullptr);
}

TEST(TestScienceExportPhysicsNoTopics) {
    UpeScienceExportOptions options = UpeScienceExportOptions_Default();
    options.pretty = 0;
    options.filter_domain = (int)UPE_SCIENCE_DOMAIN_PHYSICS;
    options.include_topics = 0;

    uint32_t required = UpeScience_WriteCatalogJsonEx(&options, nullptr, 0);
    REQUIRE(required > 0);

    std::vector<char> json(required + 1u, '\0');
    uint32_t produced = UpeScience_WriteCatalogJsonEx(&options, json.data(), (uint32_t)json.size());
    REQUIRE(produced == required);

    REQUIRE(strstr(json.data(), "\"domain\":\"PHYSICS\"") != nullptr);
    REQUIRE(strstr(json.data(), "\"topics\":[") == nullptr);
}

TEST(TestScienceExportInvalidFiltersFallback) {
    UpeScienceExportOptions options = UpeScienceExportOptions_Default();
    options.pretty = 0;
    options.filter_domain = 9999;
    options.filter_level = 9999;

    uint32_t required = UpeScience_WriteCatalogJsonEx(&options, nullptr, 0);
    REQUIRE(required > 0);

    std::vector<char> json(required + 1u, '\0');
    UpeScience_WriteCatalogJsonEx(&options, json.data(), (uint32_t)json.size());

    REQUIRE(strstr(json.data(), "\"domains\":[") != nullptr);
    REQUIRE(strstr(json.data(), "\"declaredTopics\":") != nullptr);
}

TEST(TestScienceExportImplementedOnlyExcludesNotImplemented) {
    UpeScienceExportOptions options = UpeScienceExportOptions_Default();
    options.pretty = 0;
    options.include_only_implemented = 1;
    options.filter_level = (int)UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED;

    uint32_t required = UpeScience_WriteCatalogJsonEx(&options, nullptr, 0);
    REQUIRE(required > 0);

    std::vector<char> json(required + 1u, '\0');
    UpeScience_WriteCatalogJsonEx(&options, json.data(), (uint32_t)json.size());

    REQUIRE(strstr(json.data(), "\"declaredTopics\":0") != nullptr);
    REQUIRE(strstr(json.data(), "\"implementedTopics\":0") != nullptr);
}

TEST(TestScienceExportSubstringCaseInsensitive) {
    UpeScienceExportOptions upper = UpeScienceExportOptions_Default();
    upper.pretty = 0;
    upper.include_filter_metadata = 0;
    upper.topic_substring = "SPACE";

    UpeScienceExportOptions lower = UpeScienceExportOptions_Default();
    lower.pretty = 0;
    lower.include_filter_metadata = 0;
    lower.topic_substring = "space";

    uint32_t required_upper = UpeScience_WriteCatalogJsonEx(&upper, nullptr, 0);
    uint32_t required_lower = UpeScience_WriteCatalogJsonEx(&lower, nullptr, 0);
    REQUIRE(required_upper > 0);
    REQUIRE(required_upper == required_lower);

    std::vector<char> json_upper(required_upper + 1u, '\0');
    std::vector<char> json_lower(required_lower + 1u, '\0');
    UpeScience_WriteCatalogJsonEx(&upper, json_upper.data(), (uint32_t)json_upper.size());
    UpeScience_WriteCatalogJsonEx(&lower, json_lower.data(), (uint32_t)json_lower.size());

    REQUIRE(strcmp(json_upper.data(), json_lower.data()) == 0);
}

TEST(TestScienceExportCaseSensitiveToggle) {
    UpeScienceExportOptions insensitive = UpeScienceExportOptions_Default();
    insensitive.pretty = 0;
    insensitive.topic_substring = "SPACE";
    insensitive.topic_substring_case_sensitive = 0;

    UpeScienceExportOptions sensitive = UpeScienceExportOptions_Default();
    sensitive.pretty = 0;
    sensitive.topic_substring = "SPACE";
    sensitive.topic_substring_case_sensitive = 1;

    uint32_t required_insensitive = UpeScience_WriteCatalogJsonEx(&insensitive, nullptr, 0);
    uint32_t required_sensitive = UpeScience_WriteCatalogJsonEx(&sensitive, nullptr, 0);

    REQUIRE(required_insensitive >= required_sensitive);
}

TEST(TestScienceExportHeaderToggleControls) {
    UpeScienceExportOptions options = UpeScienceExportOptions_Default();
    options.pretty = 0;
    options.include_schema = 0;
    options.include_totals = 0;
    options.include_filter_metadata = 0;

    uint32_t required = UpeScience_WriteCatalogJsonEx(&options, nullptr, 0);
    REQUIRE(required > 0);

    std::vector<char> json(required + 1u, '\0');
    UpeScience_WriteCatalogJsonEx(&options, json.data(), (uint32_t)json.size());

    REQUIRE(strstr(json.data(), "\"schema\":\"upe.science.catalog.v1\"") == nullptr);
    REQUIRE(strstr(json.data(), "\"totals\":") == nullptr);
    REQUIRE(strstr(json.data(), "\"filters\":") == nullptr);
    REQUIRE(strstr(json.data(), "\"domains\":[") != nullptr);
}

TEST(TestScienceExportMaxTopicsPerDomain) {
    UpeScienceExportOptions options = UpeScienceExportOptions_Default();
    options.pretty = 0;
    options.filter_domain = (int)UPE_SCIENCE_DOMAIN_PHYSICS;
    options.max_topics_per_domain = 1;
    options.include_topics = 1;

    uint32_t required = UpeScience_WriteCatalogJsonEx(&options, nullptr, 0);
    REQUIRE(required > 0);

    std::vector<char> json(required + 1u, '\0');
    UpeScience_WriteCatalogJsonEx(&options, json.data(), (uint32_t)json.size());

    REQUIRE(strstr(json.data(), "\"domain\":\"PHYSICS\"") != nullptr);
    REQUIRE(CountOccurrences(json.data(), "\"id\":") <= 1u);
    REQUIRE(strstr(json.data(), "\"declaredTopicCount\":") != nullptr);
}
