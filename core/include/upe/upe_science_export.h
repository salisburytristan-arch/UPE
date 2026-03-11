#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UpeScienceExportDomainOrder {
	UPE_SCIENCE_EXPORT_DOMAIN_ENUM_ORDER = 0,
	UPE_SCIENCE_EXPORT_DOMAIN_DECLARED_DESC = 1,
	UPE_SCIENCE_EXPORT_DOMAIN_IMPLEMENTED_DESC = 2,
	UPE_SCIENCE_EXPORT_DOMAIN_KEY_ASC = 3
} UpeScienceExportDomainOrder;

typedef enum UpeScienceExportTopicOrder {
	UPE_SCIENCE_EXPORT_TOPIC_ID_ASC = 0,
	UPE_SCIENCE_EXPORT_TOPIC_NAME_ASC = 1,
	UPE_SCIENCE_EXPORT_TOPIC_LEVEL_DESC_ID_ASC = 2
} UpeScienceExportTopicOrder;

typedef struct UpeScienceExportOptions {
	int pretty;
	int include_schema;
	int include_totals;
	int include_topics;
	int include_domain_summaries;
	int include_filter_metadata;
	int include_empty_domains;
	int include_only_implemented;
	int topic_substring_case_sensitive;
	int filter_domain;
	int filter_level;
	int domain_order;
	int topic_order;
	uint32_t max_topics_per_domain;
	const char *topic_substring;
} UpeScienceExportOptions;

UpeScienceExportOptions UpeScienceExportOptions_Default(void);

/**
 * Advanced JSON export with filtering and projection controls.
 *
 * filter_domain/filter_level:
 * - set to -1 for no filter
 * - otherwise use UpeScienceDomain / UpeScienceImplementationLevel numeric value
 *
 * max_topics_per_domain:
 * - set to 0 for no cap
 * - otherwise limits serialized topics[] per domain while preserving declaredTopicCount
 */
uint32_t UpeScience_WriteCatalogJsonEx(const UpeScienceExportOptions *options,
									   char *buffer,
									   uint32_t buffer_size);

/**
 * Serialize the full science catalog (domains + topics + derived totals) as JSON.
 *
 * Behavior:
 * - If buffer is null or buffer_size is 0, returns required JSON length (excluding null terminator).
 * - If buffer is provided, writes as much as fits, always null-terminates when buffer_size > 0.
 * - Return value is the full JSON length that would be produced (excluding null terminator).
 */
uint32_t UpeScience_WriteCatalogJson(char *buffer, uint32_t buffer_size, int pretty);

#ifdef __cplusplus
}
#endif
