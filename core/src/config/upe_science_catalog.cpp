#include "upe/upe_science_catalog.h"
#include "upe/upe_science_topics.h"

static const UpeScienceDomainStatus kScienceDomainStatuses[] = {
    {
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "MATH",
        "Core engine math/runtime numerics implemented; broad academic curriculum mostly pending.",
        0,
        0
    },
    {
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "CHEM",
        "Chemistry stack represented in catalog but not implemented as simulation modules.",
        0,
        0
    },
    {
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "ASTROLOGY",
        "Astrology topics cataloged only; no runtime implementation.",
        0,
        0
    },
    {
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "ASTRONOMY",
        "Astronomy represented with initial celestial mechanics subset through orbital tools.",
        0,
        0
    },
    {
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "PHYSICS",
        "Rigid-body/constraints/collision pipeline strongly implemented; many advanced subdomains still pending.",
        0,
        0
    },
    {
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "STRING",
        "String-theory topics cataloged only; no simulation/runtime modules implemented.",
        0,
        0
    },
    {
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "SPACE",
        "Space-force and orbital/relativistic utility foundations implemented; operational space systems pending.",
        0,
        0
    },
    {
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "OPT",
        "Engine optimization foundation heavily implemented in hot paths, determinism, memory, and benchmarking.",
        0,
        0
    }
};

static UpeScienceImplementationLevel UpeScience_DomainDerivedLevel(UpeScienceDomain domain) {
    if (domain >= UPE_SCIENCE_DOMAIN_COUNT) {
        return UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED;
    }

    uint32_t total = UpeScience_GetDomainTopicCount(domain);
    if (total == 0) {
        return UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED;
    }

    uint32_t extreme = UpeScience_GetTopicsByDomainAndLevel(domain, UPE_SCIENCE_LEVEL_EXTREME, nullptr, 0);
    uint32_t minimal = UpeScience_GetTopicsByDomainAndLevel(domain, UPE_SCIENCE_LEVEL_MINIMAL, nullptr, 0);
    uint32_t started = UpeScience_GetTopicsByDomainAndLevel(domain, UPE_SCIENCE_LEVEL_JUST_STARTED, nullptr, 0);

    if (extreme > 0) {
        return UPE_SCIENCE_LEVEL_EXTREME;
    }
    if (minimal > 0) {
        return UPE_SCIENCE_LEVEL_MINIMAL;
    }
    if (started > 0) {
        return UPE_SCIENCE_LEVEL_JUST_STARTED;
    }

    return UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED;
}

static UpeScienceDomainStatus UpeScience_DeriveDomainStatus(const UpeScienceDomainStatus *base) {
    UpeScienceDomainStatus out = *base;
    out.declared_topic_count = UpeScience_GetDomainTopicCount(base->domain);
    out.implemented_topic_count = UpeScience_GetDomainImplementedTopicCount(base->domain);
    out.level = UpeScience_DomainDerivedLevel(base->domain);
    return out;
}

uint32_t UpeScience_GetDomainStatusCount(void) {
    return (uint32_t)(sizeof(kScienceDomainStatuses) / sizeof(kScienceDomainStatuses[0]));
}

uint32_t UpeScience_GetDomainStatuses(UpeScienceDomainStatus *out_statuses, uint32_t max_statuses) {
    if (!out_statuses || max_statuses == 0) {
        return 0;
    }

    uint32_t count = UpeScience_GetDomainStatusCount();
    if (count > max_statuses) {
        count = max_statuses;
    }

    for (uint32_t i = 0; i < count; ++i) {
        out_statuses[i] = UpeScience_DeriveDomainStatus(&kScienceDomainStatuses[i]);
    }

    return count;
}

int UpeScience_GetDomainStatus(UpeScienceDomain domain, UpeScienceDomainStatus *out_status) {
    if (!out_status) {
        return 0;
    }

    uint32_t count = UpeScience_GetDomainStatusCount();
    for (uint32_t i = 0; i < count; ++i) {
        if (kScienceDomainStatuses[i].domain == domain) {
            *out_status = UpeScience_DeriveDomainStatus(&kScienceDomainStatuses[i]);
            return 1;
        }
    }

    return 0;
}

uint32_t UpeScience_GetTotalDeclaredTopicCount(void) {
    return UpeScience_GetTopicCount();
}

uint32_t UpeScience_GetTotalImplementedTopicCount(void) {
    uint32_t total = UpeScience_GetTopicCount();
    uint32_t not_impl = UpeScience_GetLevelTopicCount(UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED);
    return (total >= not_impl) ? (total - not_impl) : 0;
}

float UpeScience_GetOverallImplementationRatio(void) {
    uint32_t total = UpeScience_GetTotalDeclaredTopicCount();
    if (total == 0) {
        return 0.0f;
    }
    uint32_t implemented = UpeScience_GetTotalImplementedTopicCount();
    return (float)implemented / (float)total;
}

const char *UpeScience_DomainToString(UpeScienceDomain domain) {
    switch (domain) {
        case UPE_SCIENCE_DOMAIN_MATH: return "MATH";
        case UPE_SCIENCE_DOMAIN_CHEM: return "CHEM";
        case UPE_SCIENCE_DOMAIN_ASTROLOGY: return "ASTROLOGY";
        case UPE_SCIENCE_DOMAIN_ASTRONOMY: return "ASTRONOMY";
        case UPE_SCIENCE_DOMAIN_PHYSICS: return "PHYSICS";
        case UPE_SCIENCE_DOMAIN_STRING: return "STRING";
        case UPE_SCIENCE_DOMAIN_SPACE: return "SPACE";
        case UPE_SCIENCE_DOMAIN_OPT: return "OPT";
        default: return "UNKNOWN";
    }
}

const char *UpeScience_LevelToString(UpeScienceImplementationLevel level) {
    switch (level) {
        case UPE_SCIENCE_LEVEL_EXTREME: return "Added in extreme detail";
        case UPE_SCIENCE_LEVEL_MINIMAL: return "Minimal detail";
        case UPE_SCIENCE_LEVEL_JUST_STARTED: return "Just started";
        case UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED: return "Not implemented at all";
        default: return "Unknown";
    }
}
