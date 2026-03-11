#include <iostream>
#include <string>
#include <vector>

#include "upe/upe_api.h"

int main() {
    UpeScienceExportOptions full_options = UpeScienceExportOptions_Default();
    full_options.pretty = 1;

    uint32_t required = UpeScience_WriteCatalogJsonEx(&full_options, nullptr, 0);
    std::vector<char> json(required + 1u, '\0');
    uint32_t produced = UpeScience_WriteCatalogJsonEx(&full_options, json.data(), (uint32_t)json.size());

    std::cout << "science catalog json bytes: " << produced << std::endl;

    UpeScienceExportOptions filtered_options = UpeScienceExportOptions_Default();
    filtered_options.pretty = 1;
    filtered_options.include_schema = 0;
    filtered_options.include_filter_metadata = 1;
    filtered_options.include_domain_summaries = 0;
    filtered_options.include_empty_domains = 0;
    filtered_options.filter_domain = (int)UPE_SCIENCE_DOMAIN_PHYSICS;
    filtered_options.include_only_implemented = 1;
    filtered_options.topic_substring = "space";
    filtered_options.topic_substring_case_sensitive = 0;
    filtered_options.domain_order = (int)UPE_SCIENCE_EXPORT_DOMAIN_IMPLEMENTED_DESC;
    filtered_options.topic_order = (int)UPE_SCIENCE_EXPORT_TOPIC_LEVEL_DESC_ID_ASC;
    filtered_options.max_topics_per_domain = 8;

    uint32_t filtered_required = UpeScience_WriteCatalogJsonEx(&filtered_options, nullptr, 0);
    std::vector<char> filtered_json(filtered_required + 1u, '\0');
    uint32_t filtered_produced = UpeScience_WriteCatalogJsonEx(
        &filtered_options,
        filtered_json.data(),
        (uint32_t)filtered_json.size());

    std::cout << "filtered physics-space json bytes: " << filtered_produced << std::endl;

    UpeCapabilities caps = UpeEngine_GetCapabilities();
    std::cout << "capability bits: " << caps.capability_bits << std::endl;

    std::string preview(json.data());
    if (preview.size() > 1200) {
        preview.resize(1200);
        preview += "\n... (truncated) ...\n";
    }

    std::cout << preview;

    std::cout << "\n=== Filtered Preview ===\n";
    std::string filtered_preview(filtered_json.data());
    if (filtered_preview.size() > 600) {
        filtered_preview.resize(600);
        filtered_preview += "\n... (truncated) ...\n";
    }
    std::cout << filtered_preview;

    return 0;
}
