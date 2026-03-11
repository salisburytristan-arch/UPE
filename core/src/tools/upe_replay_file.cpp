#include "upe/upe_replay_file.h"
#include <stdio.h>
#include <string.h>

static const uint32_t UPE_REPLAY_MAGIC = 0x55505250u; /* UPRP */
static const uint32_t UPE_REPLAY_VERSION = 1u;

typedef struct UpeReplayHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t seed;
} UpeReplayHeader;

typedef struct UpeReplayFooter {
    uint64_t digest;
    uint32_t frame_count;
} UpeReplayFooter;

static FILE *as_fp(UpeReplayFile *file) {
    return (FILE *)file->handle;
}

static int write_u32(FILE *fp, uint32_t value) {
    return fwrite(&value, sizeof(value), 1, fp) == 1 ? 1 : 0;
}

static int write_u64(FILE *fp, uint64_t value) {
    return fwrite(&value, sizeof(value), 1, fp) == 1 ? 1 : 0;
}

static int read_u32(FILE *fp, uint32_t *value) {
    return fread(value, sizeof(*value), 1, fp) == 1 ? 1 : 0;
}

static int read_u64(FILE *fp, uint64_t *value) {
    return fread(value, sizeof(*value), 1, fp) == 1 ? 1 : 0;
}

int UpeReplayFile_BeginWrite(UpeReplayFile *file, const char *path, uint64_t seed) {
    if (!file || !path) {
        return 0;
    }

    memset(file, 0, sizeof(*file));

    FILE *fp = fopen(path, "wb");
    if (!fp) {
        return 0;
    }

    UpeReplayHeader header;
    header.magic = UPE_REPLAY_MAGIC;
    header.version = UPE_REPLAY_VERSION;
    header.seed = seed;

    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    UpeReplay_Begin(&file->stream, seed);
    file->handle = fp;
    file->mode = 1;
    file->frame_count = 0;
    file->seed = seed;
    file->expected_digest = 0;

    return 1;
}

int UpeReplayFile_WriteStep(UpeReplayFile *file, uint64_t step, const uint32_t *inputs, uint32_t input_count) {
    if (!file || file->mode != 1 || !file->handle) {
        return 0;
    }

    FILE *fp = as_fp(file);

    if (!write_u64(fp, step) || !write_u32(fp, input_count)) {
        return 0;
    }

    for (uint32_t i = 0; i < input_count; ++i) {
        if (!write_u32(fp, inputs ? inputs[i] : 0u)) {
            return 0;
        }
    }

    UpeReplay_NextStep(&file->stream);
    UpeReplay_PushU32(&file->stream, input_count);
    UpeReplay_PushU32(&file->stream, (uint32_t)step);
    UpeReplay_PushU32(&file->stream, (uint32_t)(step >> 32));

    for (uint32_t i = 0; i < input_count; ++i) {
        UpeReplay_PushU32(&file->stream, inputs ? inputs[i] : 0u);
    }

    file->frame_count += 1;
    return 1;
}

int UpeReplayFile_EndWrite(UpeReplayFile *file, uint64_t final_digest) {
    if (!file || file->mode != 1 || !file->handle) {
        return 0;
    }

    FILE *fp = as_fp(file);

    if (final_digest == 0) {
        final_digest = UpeReplay_Digest(&file->stream);
    }

    UpeReplayFooter footer;
    footer.digest = final_digest;
    footer.frame_count = file->frame_count;

    int ok = (fwrite(&footer, sizeof(footer), 1, fp) == 1) ? 1 : 0;

    fclose(fp);
    file->handle = NULL;
    file->mode = 0;
    return ok;
}

int UpeReplayFile_BeginRead(UpeReplayFile *file, const char *path, uint64_t *out_seed) {
    if (!file || !path) {
        return 0;
    }

    memset(file, 0, sizeof(*file));

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return 0;
    }

    UpeReplayHeader header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    if (header.magic != UPE_REPLAY_MAGIC || header.version != UPE_REPLAY_VERSION) {
        fclose(fp);
        return 0;
    }

    UpeReplay_Begin(&file->stream, header.seed);
    file->handle = fp;
    file->mode = 2;
    file->seed = header.seed;
    file->expected_digest = 0;
    file->frame_count = 0;

    if (out_seed) {
        *out_seed = header.seed;
    }

    return 1;
}

int UpeReplayFile_ReadStep(UpeReplayFile *file,
                           uint64_t *out_step,
                           uint32_t *out_inputs,
                           uint32_t max_inputs,
                           uint32_t *out_input_count) {
    if (!file || file->mode != 2 || !file->handle) {
        return 0;
    }

    FILE *fp = as_fp(file);

    uint64_t step = 0;
    uint32_t input_count = 0;

    long pos_before = ftell(fp);
    if (pos_before < 0) {
        return 0;
    }

    if (!read_u64(fp, &step)) {
        return 0;
    }

    if (!read_u32(fp, &input_count)) {
        return 0;
    }

    if (input_count > 1000000u) {
        return 0;
    }

    UpeReplay_NextStep(&file->stream);
    UpeReplay_PushU32(&file->stream, input_count);
    UpeReplay_PushU32(&file->stream, (uint32_t)step);
    UpeReplay_PushU32(&file->stream, (uint32_t)(step >> 32));

    for (uint32_t i = 0; i < input_count; ++i) {
        uint32_t value = 0;
        if (!read_u32(fp, &value)) {
            return 0;
        }
        if (out_inputs && i < max_inputs) {
            out_inputs[i] = value;
        }
        UpeReplay_PushU32(&file->stream, value);
    }

    file->frame_count += 1;

    if (out_step) {
        *out_step = step;
    }
    if (out_input_count) {
        *out_input_count = input_count;
    }

    return 1;
}

int UpeReplayFile_EndRead(UpeReplayFile *file, uint64_t *out_final_digest) {
    if (!file || file->mode != 2 || !file->handle) {
        return 0;
    }

    FILE *fp = as_fp(file);

    UpeReplayFooter footer;
    if (fread(&footer, sizeof(footer), 1, fp) != 1) {
        fclose(fp);
        file->handle = NULL;
        file->mode = 0;
        return 0;
    }

    file->expected_digest = footer.digest;

    if (out_final_digest) {
        *out_final_digest = footer.digest;
    }

    fclose(fp);
    file->handle = NULL;
    file->mode = 0;
    return 1;
}

void UpeReplayFile_Close(UpeReplayFile *file) {
    if (!file) {
        return;
    }

    if (file->handle) {
        FILE *fp = as_fp(file);
        fclose(fp);
    }

    memset(file, 0, sizeof(*file));
}
