import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC_MD = ROOT / "EnhancementList.md"
OUT_H = ROOT / "core" / "include" / "upe" / "upe_science_topics.h"
OUT_CPP = ROOT / "core" / "src" / "config" / "upe_science_topics_generated.cpp"

DOMAIN_ENUM = {
    "MATH": "UPE_SCIENCE_DOMAIN_MATH",
    "CHEM": "UPE_SCIENCE_DOMAIN_CHEM",
    "ASTROLOGY": "UPE_SCIENCE_DOMAIN_ASTROLOGY",
    "ASTRONOMY": "UPE_SCIENCE_DOMAIN_ASTRONOMY",
    "PHYSICS": "UPE_SCIENCE_DOMAIN_PHYSICS",
    "STRING": "UPE_SCIENCE_DOMAIN_STRING",
    "SPACE": "UPE_SCIENCE_DOMAIN_SPACE",
    "OPT": "UPE_SCIENCE_DOMAIN_OPT",
}

DOMAIN_LEVEL = {
    "MATH": "UPE_SCIENCE_LEVEL_MINIMAL",
    "CHEM": "UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED",
    "ASTROLOGY": "UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED",
    "ASTRONOMY": "UPE_SCIENCE_LEVEL_JUST_STARTED",
    "PHYSICS": "UPE_SCIENCE_LEVEL_EXTREME",
    "STRING": "UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED",
    "SPACE": "UPE_SCIENCE_LEVEL_MINIMAL",
    "OPT": "UPE_SCIENCE_LEVEL_EXTREME",
}

def c_string(s: str) -> str:
    s = s.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{s}"'


def parse_topics(text: str):
    start = text.find("[MATH]")
    if start < 0:
        raise RuntimeError("Could not find [MATH] topic list start")
    blob = text[start:]
    parts = re.split(r",\s*\[", blob)
    topics = []
    for i, part in enumerate(parts):
        token = part if i == 0 else "[" + part
        token = token.strip()
        m = re.match(r"^\[([A-Z]+)\]\s*(.+)$", token)
        if not m:
            continue
        domain = m.group(1)
        topic = m.group(2).strip()
        if domain not in DOMAIN_ENUM or not topic:
            continue
        topics.append((domain, topic))
    return topics


def generate_header():
    return """#pragma once

#include <stdint.h>
#include "upe/upe_science_catalog.h"

#ifdef __cplusplus
extern \"C\" {
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
int UpeScience_GetTopicById(uint32_t id, UpeScienceTopicStatus *out_topic);

#ifdef __cplusplus
}
#endif
"""


def generate_cpp(topics):
    lines = []
    lines.append('#include "upe/upe_science_topics.h"')
    lines.append('')
    lines.append('static const UpeScienceTopicStatus kScienceTopics[] = {')
    for i, (domain, topic) in enumerate(topics, 1):
        lines.append('    {')
        lines.append(f'        {i}u,')
        lines.append(f'        {DOMAIN_ENUM[domain]},')
        lines.append(f'        {DOMAIN_LEVEL[domain]},')
        lines.append(f'        {c_string(topic)}')
        lines.append('    },')
    lines.append('};')
    lines.append('')
    lines.append('uint32_t UpeScience_GetTopicCount(void) {')
    lines.append('    return (uint32_t)(sizeof(kScienceTopics) / sizeof(kScienceTopics[0]));')
    lines.append('}')
    lines.append('')
    lines.append('uint32_t UpeScience_GetTopics(UpeScienceTopicStatus *out_topics, uint32_t max_topics) {')
    lines.append('    if (!out_topics || max_topics == 0) {')
    lines.append('        return 0;')
    lines.append('    }')
    lines.append('    uint32_t count = UpeScience_GetTopicCount();')
    lines.append('    if (count > max_topics) {')
    lines.append('        count = max_topics;')
    lines.append('    }')
    lines.append('    for (uint32_t i = 0; i < count; ++i) {')
    lines.append('        out_topics[i] = kScienceTopics[i];')
    lines.append('    }')
    lines.append('    return count;')
    lines.append('}')
    lines.append('')
    lines.append('uint32_t UpeScience_GetTopicsByDomain(UpeScienceDomain domain, UpeScienceTopicStatus *out_topics, uint32_t max_topics) {')
    lines.append('    if (!out_topics || max_topics == 0) {')
    lines.append('        return 0;')
    lines.append('    }')
    lines.append('    uint32_t written = 0;')
    lines.append('    uint32_t total = UpeScience_GetTopicCount();')
    lines.append('    for (uint32_t i = 0; i < total && written < max_topics; ++i) {')
    lines.append('        if (kScienceTopics[i].domain == domain) {')
    lines.append('            out_topics[written++] = kScienceTopics[i];')
    lines.append('        }')
    lines.append('    }')
    lines.append('    return written;')
    lines.append('}')
    lines.append('')
    lines.append('int UpeScience_GetTopicById(uint32_t id, UpeScienceTopicStatus *out_topic) {')
    lines.append('    if (!out_topic || id == 0) {')
    lines.append('        return 0;')
    lines.append('    }')
    lines.append('    uint32_t total = UpeScience_GetTopicCount();')
    lines.append('    for (uint32_t i = 0; i < total; ++i) {')
    lines.append('        if (kScienceTopics[i].id == id) {')
    lines.append('            *out_topic = kScienceTopics[i];')
    lines.append('            return 1;')
    lines.append('        }')
    lines.append('    }')
    lines.append('    return 0;')
    lines.append('}')
    lines.append('')
    return "\n".join(lines)


def main():
    text = SRC_MD.read_text(encoding="utf-8")
    topics = parse_topics(text)
    if not topics:
        raise RuntimeError("No topics parsed from EnhancementList.md")

    OUT_H.parent.mkdir(parents=True, exist_ok=True)
    OUT_CPP.parent.mkdir(parents=True, exist_ok=True)
    OUT_H.write_text(generate_header(), encoding="utf-8")
    OUT_CPP.write_text(generate_cpp(topics), encoding="utf-8")

    print(f"Generated {len(topics)} topics")


if __name__ == "__main__":
    main()
