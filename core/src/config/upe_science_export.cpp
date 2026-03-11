#include "upe/upe_science_export.h"

#include "upe/upe_science_catalog.h"
#include "upe/upe_science_topics.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct UpeScienceDomainRuntime {
    UpeScienceDomainStatus status;
    uint32_t declared_topic_count;
    uint32_t implemented_topic_count;
    UpeScienceImplementationLevel max_level;
    uint32_t first_topic_index;
    uint32_t topic_count;
} UpeScienceDomainRuntime;

static void UpeScience_JsonAppend(char *buffer,
                                  uint32_t buffer_size,
                                  uint32_t *offset,
                                  const char *fmt,
                                  ...) {
    if (!offset || !fmt) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    uint32_t write_pos = *offset;
    int written = 0;
    if (buffer && buffer_size > 0 && write_pos < buffer_size) {
        written = vsnprintf(buffer + write_pos, (size_t)(buffer_size - write_pos), fmt, args);
    } else {
        written = vsnprintf(nullptr, 0, fmt, args);
    }

    va_end(args);

    if (written <= 0) {
        return;
    }

    *offset += (uint32_t)written;
}

static void UpeScience_JsonAppendRaw(char *buffer,
                                     uint32_t buffer_size,
                                     uint32_t *offset,
                                     const char *text,
                                     uint32_t text_len) {
    if (!offset || !text || text_len == 0u) {
        return;
    }

    uint32_t write_pos = *offset;
    if (buffer && buffer_size > 0u && write_pos < buffer_size) {
        uint32_t available = buffer_size - write_pos;
        uint32_t to_copy = (text_len < available) ? text_len : available;
        memcpy(buffer + write_pos, text, to_copy);
    }

    *offset += text_len;
}

static void UpeScience_JsonAppendChar(char *buffer,
                                      uint32_t buffer_size,
                                      uint32_t *offset,
                                      char value) {
    UpeScience_JsonAppendRaw(buffer, buffer_size, offset, &value, 1u);
}

static void UpeScience_JsonAppendEscaped(char *buffer,
                                         uint32_t buffer_size,
                                         uint32_t *offset,
                                         const char *text) {
    if (!offset) {
        return;
    }

    UpeScience_JsonAppendChar(buffer, buffer_size, offset, '"');
    if (text) {
        const char *segment_start = text;
        const char *p = text;
        while (*p != '\0') {
            unsigned char c = (unsigned char)(*p);
            switch (c) {
                case '\\':
                case '"':
                case '\n':
                case '\r':
                case '\t': {
                    uint32_t segment_len = (uint32_t)(p - segment_start);
                    UpeScience_JsonAppendRaw(buffer, buffer_size, offset, segment_start, segment_len);
                    switch (c) {
                        case '\\': UpeScience_JsonAppendRaw(buffer, buffer_size, offset, "\\\\", 2u); break;
                        case '"': UpeScience_JsonAppendRaw(buffer, buffer_size, offset, "\\\"", 2u); break;
                        case '\n': UpeScience_JsonAppendRaw(buffer, buffer_size, offset, "\\n", 2u); break;
                        case '\r': UpeScience_JsonAppendRaw(buffer, buffer_size, offset, "\\r", 2u); break;
                        case '\t': UpeScience_JsonAppendRaw(buffer, buffer_size, offset, "\\t", 2u); break;
                        default: break;
                    }
                    ++p;
                    segment_start = p;
                    break;
                }
                default:
                    if (c < 0x20u) {
                        uint32_t segment_len = (uint32_t)(p - segment_start);
                        UpeScience_JsonAppendRaw(buffer, buffer_size, offset, segment_start, segment_len);
                        const char hex[] = "0123456789abcdef";
                        char escaped[6] = {
                            '\\',
                            'u',
                            '0',
                            '0',
                            hex[(c >> 4u) & 0x0Fu],
                            hex[c & 0x0Fu],
                        };
                        UpeScience_JsonAppendRaw(buffer, buffer_size, offset, escaped, 6u);
                        ++p;
                        segment_start = p;
                        break;
                    }
                    ++p;
                    break;
            }
        }
        uint32_t tail_len = (uint32_t)(p - segment_start);
        UpeScience_JsonAppendRaw(buffer, buffer_size, offset, segment_start, tail_len);
    }
    UpeScience_JsonAppendChar(buffer, buffer_size, offset, '"');
}

static void UpeScience_FinalizeBuffer(char *buffer, uint32_t buffer_size, uint32_t full_len) {
    if (!buffer || buffer_size == 0) {
        return;
    }
    if (full_len < buffer_size) {
        buffer[full_len] = '\0';
    } else {
        buffer[buffer_size - 1u] = '\0';
    }
}

static int UpeScience_ToBool(int value) {
    return value ? 1 : 0;
}

static int UpeScience_StrContains(const char *haystack, const char *needle, int case_sensitive) {
    if (!haystack || !needle) {
        return 0;
    }
    if (needle[0] == '\0') {
        return 1;
    }

    if (case_sensitive) {
        return strstr(haystack, needle) != nullptr;
    }

    for (const char *h = haystack; *h != '\0'; ++h) {
        const char *h_it = h;
        const char *n_it = needle;
        while (*h_it != '\0' && *n_it != '\0') {
            char hc = (char)tolower((unsigned char)(*h_it));
            char nc = (char)tolower((unsigned char)(*n_it));
            if (hc != nc) {
                break;
            }
            ++h_it;
            ++n_it;
        }
        if (*n_it == '\0') {
            return 1;
        }
    }
    return 0;
}

static int UpeScience_TopicMatchesFilters(const UpeScienceTopicStatus *topic,
                                          const UpeScienceExportOptions *options) {
    if (!topic || !options) {
        return 0;
    }

    if (options->filter_domain >= 0 && (int)topic->domain != options->filter_domain) {
        return 0;
    }
    if (options->filter_level >= 0 && (int)topic->level != options->filter_level) {
        return 0;
    }
    if (options->include_only_implemented && topic->level == UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED) {
        return 0;
    }
    if (options->topic_substring && options->topic_substring[0] != '\0') {
        if (!UpeScience_StrContains(topic->topic,
                                    options->topic_substring,
                                    options->topic_substring_case_sensitive)) {
            return 0;
        }
    }

    return 1;
}

static int UpeScience_ValidDomainFilter(int filter_domain) {
    return filter_domain >= -1 && filter_domain < (int)UPE_SCIENCE_DOMAIN_COUNT;
}

static int UpeScience_ValidLevelFilter(int filter_level) {
    return filter_level >= -1 && filter_level <= (int)UPE_SCIENCE_LEVEL_EXTREME;
}

static int UpeScience_ValidDomainOrder(int value) {
    return value >= (int)UPE_SCIENCE_EXPORT_DOMAIN_ENUM_ORDER &&
           value <= (int)UPE_SCIENCE_EXPORT_DOMAIN_KEY_ASC;
}

static int UpeScience_ValidTopicOrder(int value) {
    return value >= (int)UPE_SCIENCE_EXPORT_TOPIC_ID_ASC &&
           value <= (int)UPE_SCIENCE_EXPORT_TOPIC_LEVEL_DESC_ID_ASC;
}

static int UpeScience_CompareTopic(const UpeScienceTopicStatus *a,
                                   const UpeScienceTopicStatus *b,
                                   int order) {
    if (!a || !b) {
        return 0;
    }

    switch (order) {
        case UPE_SCIENCE_EXPORT_TOPIC_NAME_ASC: {
            const char *at = a->topic ? a->topic : "";
            const char *bt = b->topic ? b->topic : "";
            int cmp = strcmp(at, bt);
            if (cmp != 0) {
                return cmp;
            }
            return (a->id < b->id) ? -1 : ((a->id > b->id) ? 1 : 0);
        }
        case UPE_SCIENCE_EXPORT_TOPIC_LEVEL_DESC_ID_ASC: {
            if (a->level != b->level) {
                return (a->level > b->level) ? -1 : 1;
            }
            return (a->id < b->id) ? -1 : ((a->id > b->id) ? 1 : 0);
        }
        case UPE_SCIENCE_EXPORT_TOPIC_ID_ASC:
        default:
            return (a->id < b->id) ? -1 : ((a->id > b->id) ? 1 : 0);
    }
}

static int UpeScience_CompareDomainRuntime(const UpeScienceDomainRuntime *a,
                                           const UpeScienceDomainRuntime *b,
                                           int order) {
    if (!a || !b) {
        return 0;
    }

    switch (order) {
        case UPE_SCIENCE_EXPORT_DOMAIN_DECLARED_DESC:
            if (a->declared_topic_count != b->declared_topic_count) {
                return (a->declared_topic_count > b->declared_topic_count) ? -1 : 1;
            }
            break;
        case UPE_SCIENCE_EXPORT_DOMAIN_IMPLEMENTED_DESC:
            if (a->implemented_topic_count != b->implemented_topic_count) {
                return (a->implemented_topic_count > b->implemented_topic_count) ? -1 : 1;
            }
            break;
        case UPE_SCIENCE_EXPORT_DOMAIN_KEY_ASC: {
            const char *ak = a->status.domain_key ? a->status.domain_key : "";
            const char *bk = b->status.domain_key ? b->status.domain_key : "";
            int cmp = strcmp(ak, bk);
            if (cmp != 0) {
                return cmp;
            }
            break;
        }
        case UPE_SCIENCE_EXPORT_DOMAIN_ENUM_ORDER:
        default:
            break;
    }

    return ((int)a->status.domain < (int)b->status.domain)
        ? -1
        : (((int)a->status.domain > (int)b->status.domain) ? 1 : 0);
}

static void UpeScience_SortTopics(UpeScienceTopicStatus *topics,
                                  uint32_t count,
                                  int order) {
    if (!topics || count < 2u) {
        return;
    }

    for (uint32_t i = 1; i < count; ++i) {
        UpeScienceTopicStatus key = topics[i];
        uint32_t j = i;
        while (j > 0u && UpeScience_CompareTopic(&topics[j - 1u], &key, order) > 0) {
            topics[j] = topics[j - 1u];
            --j;
        }
        topics[j] = key;
    }
}

static void UpeScience_SortDomainIndices(const UpeScienceDomainRuntime *domains,
                                         uint32_t *indices,
                                         uint32_t count,
                                         int order) {
    if (!domains || !indices || count < 2u) {
        return;
    }

    for (uint32_t i = 1; i < count; ++i) {
        uint32_t key = indices[i];
        uint32_t j = i;
        while (j > 0u && UpeScience_CompareDomainRuntime(&domains[indices[j - 1u]], &domains[key], order) > 0) {
            indices[j] = indices[j - 1u];
            --j;
        }
        indices[j] = key;
    }
}

UpeScienceExportOptions UpeScienceExportOptions_Default(void) {
    UpeScienceExportOptions options = {};
    options.pretty = 1;
    options.include_schema = 1;
    options.include_totals = 1;
    options.include_topics = 1;
    options.include_domain_summaries = 1;
    options.include_filter_metadata = 1;
    options.include_empty_domains = 0;
    options.include_only_implemented = 0;
    options.topic_substring_case_sensitive = 0;
    options.filter_domain = -1;
    options.filter_level = -1;
    options.domain_order = (int)UPE_SCIENCE_EXPORT_DOMAIN_ENUM_ORDER;
    options.topic_order = (int)UPE_SCIENCE_EXPORT_TOPIC_ID_ASC;
    options.max_topics_per_domain = 0;
    options.topic_substring = nullptr;
    return options;
}

uint32_t UpeScience_WriteCatalogJsonEx(const UpeScienceExportOptions *options_ptr,
                                       char *buffer,
                                       uint32_t buffer_size) {
    UpeScienceExportOptions options = UpeScienceExportOptions_Default();
    if (options_ptr) {
        options = *options_ptr;
    }

    options.pretty = UpeScience_ToBool(options.pretty);
    options.include_schema = UpeScience_ToBool(options.include_schema);
    options.include_totals = UpeScience_ToBool(options.include_totals);
    options.include_topics = UpeScience_ToBool(options.include_topics);
    options.include_domain_summaries = UpeScience_ToBool(options.include_domain_summaries);
    options.include_filter_metadata = UpeScience_ToBool(options.include_filter_metadata);
    options.include_empty_domains = UpeScience_ToBool(options.include_empty_domains);
    options.include_only_implemented = UpeScience_ToBool(options.include_only_implemented);
    options.topic_substring_case_sensitive = UpeScience_ToBool(options.topic_substring_case_sensitive);

    if (!UpeScience_ValidDomainFilter(options.filter_domain)) {
        options.filter_domain = -1;
    }
    if (!UpeScience_ValidLevelFilter(options.filter_level)) {
        options.filter_level = -1;
    }
    if (!UpeScience_ValidDomainOrder(options.domain_order)) {
        options.domain_order = (int)UPE_SCIENCE_EXPORT_DOMAIN_ENUM_ORDER;
    }
    if (!UpeScience_ValidTopicOrder(options.topic_order)) {
        options.topic_order = (int)UPE_SCIENCE_EXPORT_TOPIC_ID_ASC;
    }
    if (!options.topic_substring || options.topic_substring[0] == '\0') {
        options.topic_substring = nullptr;
    }

    uint32_t offset = 0;
    const char *nl = options.pretty ? "\n" : "";
    const char *i1 = options.pretty ? "  " : "";
    const char *i2 = options.pretty ? "    " : "";
    const char *i3 = options.pretty ? "      " : "";

    uint32_t domain_count = UpeScience_GetDomainStatusCount();
    UpeScienceDomainRuntime *domains = nullptr;
    int domain_to_runtime_idx[UPE_SCIENCE_DOMAIN_COUNT];
    for (uint32_t i = 0; i < UPE_SCIENCE_DOMAIN_COUNT; ++i) {
        domain_to_runtime_idx[i] = -1;
    }
    if (domain_count > 0) {
        domains = (UpeScienceDomainRuntime*)malloc(domain_count * sizeof(UpeScienceDomainRuntime));
        if (domains) {
            UpeScienceDomainStatus *base = (UpeScienceDomainStatus*)malloc(domain_count * sizeof(UpeScienceDomainStatus));
            if (!base) {
                free(domains);
                domains = nullptr;
                domain_count = 0;
            } else {
                UpeScience_GetDomainStatuses(base, domain_count);
                for (uint32_t i = 0; i < domain_count; ++i) {
                    domains[i] = {};
                    domains[i].status = base[i];
                    domains[i].declared_topic_count = 0;
                    domains[i].implemented_topic_count = 0;
                    domains[i].max_level = UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED;
                    domains[i].first_topic_index = 0;
                    domains[i].topic_count = 0;
                    if ((uint32_t)domains[i].status.domain < UPE_SCIENCE_DOMAIN_COUNT) {
                        domain_to_runtime_idx[(uint32_t)domains[i].status.domain] = (int)i;
                    }
                }
                free(base);
            }
        } else {
            domain_count = 0;
        }
    }

    uint32_t all_topic_count = UpeScience_GetTopicCount();
    UpeScienceTopicStatus *all_topics = nullptr;
    if (all_topic_count > 0) {
        all_topics = (UpeScienceTopicStatus*)malloc(all_topic_count * sizeof(UpeScienceTopicStatus));
        if (all_topics) {
            UpeScience_GetTopics(all_topics, all_topic_count);
        } else {
            all_topic_count = 0;
        }
    }

    uint32_t total_declared = 0;
    uint32_t total_implemented = 0;

    uint8_t *topic_match = nullptr;
    if (all_topic_count > 0) {
        topic_match = (uint8_t*)malloc(all_topic_count * sizeof(uint8_t));
        if (!topic_match) {
            all_topic_count = 0;
            free(all_topics);
            all_topics = nullptr;
        }
    }

    for (uint32_t i = 0; i < all_topic_count; ++i) {
        int matched = UpeScience_TopicMatchesFilters(&all_topics[i], &options);
        topic_match[i] = matched ? 1u : 0u;
        if (!matched) {
            continue;
        }

        total_declared += 1u;
        if (all_topics[i].level != UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED) {
            total_implemented += 1u;
        }

        int runtime_idx = -1;
        if ((uint32_t)all_topics[i].domain < UPE_SCIENCE_DOMAIN_COUNT) {
            runtime_idx = domain_to_runtime_idx[(uint32_t)all_topics[i].domain];
        }
        if (runtime_idx >= 0) {
            UpeScienceDomainRuntime *dr = &domains[runtime_idx];
            dr->declared_topic_count += 1u;
            dr->topic_count += 1u;
            if (all_topics[i].level != UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED) {
                dr->implemented_topic_count += 1u;
            }
            if (all_topics[i].level > dr->max_level) {
                dr->max_level = all_topics[i].level;
            }
        }
    }
    float ratio = (total_declared > 0) ? ((float)total_implemented / (float)total_declared) : 0.0f;

    UpeScienceTopicStatus *domain_topics = nullptr;
    uint32_t *domain_write_cursor = nullptr;
    int topics_available = 0;
    if (options.include_topics && total_declared > 0 && domain_count > 0) {
        uint32_t prefix = 0;
        for (uint32_t d = 0; d < domain_count; ++d) {
            domains[d].first_topic_index = prefix;
            prefix += domains[d].topic_count;
        }

        domain_topics = (UpeScienceTopicStatus*)malloc(total_declared * sizeof(UpeScienceTopicStatus));
        domain_write_cursor = (uint32_t*)malloc(domain_count * sizeof(uint32_t));
        if (domain_topics && domain_write_cursor) {
            for (uint32_t d = 0; d < domain_count; ++d) {
                domain_write_cursor[d] = 0;
            }
            for (uint32_t i = 0; i < all_topic_count; ++i) {
                if (!topic_match || topic_match[i] == 0) {
                    continue;
                }
                int runtime_idx = -1;
                if ((uint32_t)all_topics[i].domain < UPE_SCIENCE_DOMAIN_COUNT) {
                    runtime_idx = domain_to_runtime_idx[(uint32_t)all_topics[i].domain];
                }
                if (runtime_idx < 0) {
                    continue;
                }
                UpeScienceDomainRuntime *dr = &domains[runtime_idx];
                uint32_t w = domain_write_cursor[runtime_idx]++;
                if (w < dr->topic_count) {
                    domain_topics[dr->first_topic_index + w] = all_topics[i];
                }
            }

            for (uint32_t d = 0; d < domain_count; ++d) {
                UpeScienceDomainRuntime *dr = &domains[d];
                if (dr->topic_count > 1u) {
                    UpeScience_SortTopics(
                        &domain_topics[dr->first_topic_index],
                        dr->topic_count,
                        options.topic_order);
                }
            }
            topics_available = 1;
        }
    }

    uint32_t *domain_indices = nullptr;
    uint32_t domain_emit_count = 0;
    if (domain_count > 0) {
        domain_indices = (uint32_t*)malloc(domain_count * sizeof(uint32_t));
        if (domain_indices) {
            for (uint32_t d = 0; d < domain_count; ++d) {
                const UpeScienceDomainRuntime *dr = &domains[d];
                if (options.filter_domain >= 0 && (int)dr->status.domain != options.filter_domain) {
                    continue;
                }
                if (!options.include_empty_domains && dr->declared_topic_count == 0) {
                    continue;
                }
                domain_indices[domain_emit_count++] = d;
            }
            if (domain_emit_count > 1u) {
                UpeScience_SortDomainIndices(domains, domain_indices, domain_emit_count, options.domain_order);
            }
        }
    }

    UpeScience_JsonAppend(buffer, buffer_size, &offset, "{%s", nl);

    int emitted_header_field = 0;
    if (options.include_schema) {
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"schema\":\"upe.science.catalog.v1\"", i1);
        emitted_header_field = 1;
    }

    if (options.include_totals) {
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s%s\"totals\":{%s", emitted_header_field ? "," : "", nl, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"declaredTopics\":%u,%s", i2, total_declared, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"implementedTopics\":%u,%s", i2, total_implemented, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"implementationRatio\":%.6f%s", i2, ratio, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s}%s", i1, nl);
        emitted_header_field = 1;
    }

    if (options.include_filter_metadata) {
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s%s\"filters\":{%s", emitted_header_field ? "," : "", nl, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"filterDomain\":%d,%s", i2, options.filter_domain, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"filterLevel\":%d,%s", i2, options.filter_level, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"includeOnlyImplemented\":%s,%s", i2, options.include_only_implemented ? "true" : "false", nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"substringCaseSensitive\":%s,%s", i2, options.topic_substring_case_sensitive ? "true" : "false", nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"maxTopicsPerDomain\":%u,%s", i2, options.max_topics_per_domain, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"domainOrder\":%d,%s", i2, options.domain_order, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"topicOrder\":%d,%s", i2, options.topic_order, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"topicSubstring\":", i2);
        UpeScience_JsonAppendEscaped(buffer, buffer_size, &offset, options.topic_substring ? options.topic_substring : "");
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s", nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s}%s", i1, nl);
        emitted_header_field = 1;
    }

    if (emitted_header_field) {
        UpeScience_JsonAppend(buffer, buffer_size, &offset, ",%s", nl);
    }

    UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"domains\":[%s", i1, nl);
    uint32_t emitted_domain_count = 0;

    for (uint32_t out_idx = 0; out_idx < domain_emit_count; ++out_idx) {
        UpeScienceDomainRuntime *dr = &domains[domain_indices[out_idx]];
        uint32_t topic_count = dr->declared_topic_count;
        uint32_t implemented_topic_count = dr->implemented_topic_count;
        UpeScienceImplementationLevel max_level = dr->max_level;
        uint32_t topic_write_count = topic_count;
        if (options.max_topics_per_domain > 0 && topic_write_count > options.max_topics_per_domain) {
            topic_write_count = options.max_topics_per_domain;
        }

        if (!topics_available) {
            topic_write_count = 0;
        }

        if (emitted_domain_count > 0u) {
            UpeScience_JsonAppend(buffer, buffer_size, &offset, ",%s", nl);
        }
        emitted_domain_count += 1u;
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s{%s", i2, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"domain\":", i3);
        UpeScience_JsonAppendEscaped(buffer, buffer_size, &offset, UpeScience_DomainToString(dr->status.domain));
        UpeScience_JsonAppend(buffer, buffer_size, &offset, ",%s", nl);

        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"level\":", i3);
        UpeScience_JsonAppendEscaped(buffer, buffer_size, &offset, UpeScience_LevelToString(max_level));
        UpeScience_JsonAppend(buffer, buffer_size, &offset, ",%s", nl);

        if (options.include_domain_summaries) {
            UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"summary\":", i3);
            UpeScience_JsonAppendEscaped(buffer, buffer_size, &offset, dr->status.summary ? dr->status.summary : "");
            UpeScience_JsonAppend(buffer, buffer_size, &offset, ",%s", nl);
        }

        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"declaredTopicCount\":%u,%s", i3, topic_count, nl);
        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"implementedTopicCount\":%u", i3, implemented_topic_count);

        if (options.include_topics) {
            UpeScience_JsonAppend(buffer, buffer_size, &offset, ",%s", nl);
            UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"topics\":[%s", i3, nl);
            for (uint32_t t = 0; t < topic_write_count; ++t) {
                UpeScienceTopicStatus *ts = &domain_topics[dr->first_topic_index + t];
                UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s{%s", i3, nl);
                UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"id\":%u,%s", i3, ts->id, nl);
                UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"level\":", i3);
                UpeScience_JsonAppendEscaped(buffer, buffer_size, &offset, UpeScience_LevelToString(ts->level));
                UpeScience_JsonAppend(buffer, buffer_size, &offset, ",%s", nl);
                UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s\"topic\":", i3);
                UpeScience_JsonAppendEscaped(buffer, buffer_size, &offset, ts->topic ? ts->topic : "");
                UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s%s", nl, i3);
                UpeScience_JsonAppend(buffer, buffer_size, &offset, "}%s%s", (t + 1u < topic_write_count) ? "," : "", nl);
            }
            UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s]%s", i3, nl);
        } else {
            UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s", nl);
        }

        UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s}%s", i2, nl);
    }

    UpeScience_JsonAppend(buffer, buffer_size, &offset, "%s]%s", i1, nl);
    UpeScience_JsonAppend(buffer, buffer_size, &offset, "}%s", nl);

    free(domain_indices);
    free(domain_topics);
    free(domain_write_cursor);
    free(topic_match);
    free(all_topics);
    free(domains);

    UpeScience_FinalizeBuffer(buffer, buffer_size, offset);
    return offset;
}

uint32_t UpeScience_WriteCatalogJson(char *buffer, uint32_t buffer_size, int pretty) {
    UpeScienceExportOptions options = UpeScienceExportOptions_Default();
    options.pretty = pretty ? 1 : 0;
    return UpeScience_WriteCatalogJsonEx(&options, buffer, buffer_size);
}
