#include "upe/upe_math_toolbox.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static float UpeMath_Min4(float a, float b, float c, float d) {
    float m = (a < b) ? a : b;
    m = (m < c) ? m : c;
    return (m < d) ? m : d;
}

static float UpeMath_Max4(float a, float b, float c, float d) {
    float m = (a > b) ? a : b;
    m = (m > c) ? m : c;
    return (m > d) ? m : d;
}

static int UpeMath_CompareFloatAscending(const void *lhs, const void *rhs) {
    const float a = *(const float*)lhs;
    const float b = *(const float*)rhs;
    return (a > b) - (a < b);
}

static float UpeMath_QuantileSorted(const float *sorted, uint32_t count, float q) {
    if (!sorted || count == 0u) {
        return 0.0f;
    }
    if (q <= 0.0f) {
        return sorted[0];
    }
    if (q >= 1.0f) {
        return sorted[count - 1u];
    }

    float position = q * (float)(count - 1u);
    uint32_t lower = (uint32_t)floorf(position);
    uint32_t upper = (lower + 1u < count) ? (lower + 1u) : lower;
    float fraction = position - (float)lower;
    return sorted[lower] + (sorted[upper] - sorted[lower]) * fraction;
}

UpeIntervalf UpeMath_Interval(float a, float b) {
    UpeIntervalf out = {};
    out.lo = (a < b) ? a : b;
    out.hi = (a < b) ? b : a;
    return out;
}

UpeIntervalf UpeMath_IntervalAdd(UpeIntervalf a, UpeIntervalf b) {
    return UpeMath_Interval(a.lo + b.lo, a.hi + b.hi);
}

UpeIntervalf UpeMath_IntervalSub(UpeIntervalf a, UpeIntervalf b) {
    return UpeMath_Interval(a.lo - b.hi, a.hi - b.lo);
}

UpeIntervalf UpeMath_IntervalMul(UpeIntervalf a, UpeIntervalf b) {
    float p0 = a.lo * b.lo;
    float p1 = a.lo * b.hi;
    float p2 = a.hi * b.lo;
    float p3 = a.hi * b.hi;
    UpeIntervalf out = {};
    out.lo = UpeMath_Min4(p0, p1, p2, p3);
    out.hi = UpeMath_Max4(p0, p1, p2, p3);
    return out;
}

UpeIntervalf UpeMath_IntervalDiv(UpeIntervalf a, UpeIntervalf b) {
    if (b.lo <= 0.0f && b.hi >= 0.0f) {
        return UpeMath_Interval(-INFINITY, INFINITY);
    }
    UpeIntervalf inv_b = UpeMath_Interval(1.0f / b.hi, 1.0f / b.lo);
    return UpeMath_IntervalMul(a, inv_b);
}

int UpeMath_IntervalContains(UpeIntervalf interval, float value) {
    return value >= interval.lo && value <= interval.hi;
}

int UpeMath_DeterministicReduceF32(const float *values, uint32_t count,
                                   UpeDeterministicReductionSummary *out_summary) {
    if (!values || count == 0 || !out_summary) {
        return 0;
    }

    double sum = 0.0;
    double product = 1.0;
    double sum_sq = 0.0;
    float min_value = values[0];
    float max_value = values[0];
    for (uint32_t i = 0; i < count; ++i) {
        float v = values[i];
        sum += (double)v;
        product *= (double)v;
        sum_sq += (double)v * (double)v;
        if (v < min_value) {
            min_value = v;
        }
        if (v > max_value) {
            max_value = v;
        }
    }

    out_summary->sum = sum;
    out_summary->product = product;
    out_summary->mean = sum / (double)count;
    out_summary->rms = sqrt(sum_sq / (double)count);
    out_summary->min_value = min_value;
    out_summary->max_value = max_value;
    out_summary->count = count;
    return 1;
}

int UpeMath_LeastSquaresFitLine(const float *xs, const float *ys, uint32_t count,
                                UpeLeastSquaresFit1D *out_fit) {
    if (!xs || !ys || count < 2 || !out_fit) {
        return 0;
    }

    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xx = 0.0;
    double sum_xy = 0.0;
    double sum_yy = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double x = xs[i];
        double y = ys[i];
        sum_x += x;
        sum_y += y;
        sum_xx += x * x;
        sum_xy += x * y;
        sum_yy += y * y;
    }

    double denom = ((double)count * sum_xx) - (sum_x * sum_x);
    if (fabs(denom) <= 1.0e-12) {
        return 0;
    }

    double slope = (((double)count * sum_xy) - (sum_x * sum_y)) / denom;
    double intercept = (sum_y - slope * sum_x) / (double)count;
    double rms_error = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double residual = ((slope * xs[i]) + intercept) - ys[i];
        rms_error += residual * residual;
    }
    rms_error = sqrt(rms_error / (double)count);

    double corr_denom = sqrt((((double)count * sum_xx) - (sum_x * sum_x)) *
                             (((double)count * sum_yy) - (sum_y * sum_y)));
    double correlation = (corr_denom > 1.0e-12)
        ? ((((double)count * sum_xy) - (sum_x * sum_y)) / corr_denom)
        : 0.0;

    out_fit->slope = (float)slope;
    out_fit->intercept = (float)intercept;
    out_fit->rms_error = (float)rms_error;
    out_fit->correlation = (float)correlation;
    return 1;
}

int UpeMath_BarycentricInterpolate1D(const float *xs, const float *ys, uint32_t count,
                                     float x, float *out_y) {
    if (!xs || !ys || count == 0 || !out_y) {
        return 0;
    }
    if (count == 1) {
        *out_y = ys[0];
        return 1;
    }

    for (uint32_t i = 0; i < count; ++i) {
        if (fabsf(x - xs[i]) <= 1.0e-6f) {
            *out_y = ys[i];
            return 1;
        }
    }

    double numerator = 0.0;
    double denominator = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double wi = 1.0;
        for (uint32_t j = 0; j < count; ++j) {
            if (i == j) {
                continue;
            }
            double diff = (double)xs[i] - (double)xs[j];
            if (fabs(diff) <= 1.0e-12) {
                return 0;
            }
            wi /= diff;
        }
        double term = wi / ((double)x - (double)xs[i]);
        numerator += term * (double)ys[i];
        denominator += term;
    }

    if (fabs(denominator) <= 1.0e-12) {
        return 0;
    }
    *out_y = (float)(numerator / denominator);
    return 1;
}

int UpeMath_ComputeMomentsF32(const float *values, uint32_t count,
                              UpeStatMoments *out_moments) {
    if (!values || count == 0u || !out_moments) {
        return 0;
    }

    double mean = 0.0;
    float min_value = values[0];
    float max_value = values[0];
    for (uint32_t i = 0; i < count; ++i) {
        float v = values[i];
        mean += (double)v;
        if (v < min_value) {
            min_value = v;
        }
        if (v > max_value) {
            max_value = v;
        }
    }
    mean /= (double)count;

    double variance = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double delta = (double)values[i] - mean;
        variance += delta * delta;
    }
    variance /= (double)count;

    double stddev = sqrt(variance);
    double margin = 1.96 * stddev / sqrt((double)count);

    out_moments->mean = mean;
    out_moments->variance = variance;
    out_moments->stddev = stddev;
    out_moments->min_value = min_value;
    out_moments->max_value = max_value;
    out_moments->confidence95_low = (float)(mean - margin);
    out_moments->confidence95_high = (float)(mean + margin);
    out_moments->count = count;
    return 1;
}

int UpeMath_BootstrapMeanInterval(const float *values, uint32_t count,
                                  uint32_t resamples,
                                  UpeBootstrapSummary *out_summary) {
    if (!values || count == 0u || resamples < 8u || !out_summary) {
        return 0;
    }

    float *means = (float*)malloc(resamples * sizeof(float));
    if (!means) {
        return 0;
    }

    uint32_t state = 2166136261u ^ count ^ (resamples << 1u);
    for (uint32_t r = 0; r < resamples; ++r) {
        double sum = 0.0;
        for (uint32_t i = 0; i < count; ++i) {
            state = state * 1664525u + 1013904223u;
            uint32_t idx = state % count;
            sum += (double)values[idx];
        }
        means[r] = (float)(sum / (double)count);
    }

    UpeStatMoments moments = {};
    if (!UpeMath_ComputeMomentsF32(means, resamples, &moments)) {
        free(means);
        return 0;
    }

    qsort(means, resamples, sizeof(float), UpeMath_CompareFloatAscending);
    out_summary->mean = (float)moments.mean;
    out_summary->resample_stddev = (float)moments.stddev;
    out_summary->confidence95_low = UpeMath_QuantileSorted(means, resamples, 0.025f);
    out_summary->confidence95_high = UpeMath_QuantileSorted(means, resamples, 0.975f);
    out_summary->resample_count = resamples;

    free(means);
    return 1;
}

int UpeMath_ConfidenceInterval95(const float *values, uint32_t count,
                                 float *out_low, float *out_high) {
    if (!out_low || !out_high) {
        return 0;
    }

    UpeStatMoments moments = {};
    if (!UpeMath_ComputeMomentsF32(values, count, &moments)) {
        return 0;
    }

    *out_low = moments.confidence95_low;
    *out_high = moments.confidence95_high;
    return 1;
}

int UpeMath_MovingAverageF32(const float *values, uint32_t count,
                             uint32_t window, float *out_values) {
    if (!values || !out_values || count == 0u || window == 0u) {
        return 0;
    }

    uint32_t half_window = window / 2u;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t begin = (i > half_window) ? (i - half_window) : 0u;
        uint32_t end = i + half_window + 1u;
        if (end > count) {
            end = count;
        }

        double sum = 0.0;
        uint32_t n = 0u;
        for (uint32_t j = begin; j < end; ++j) {
            sum += (double)values[j];
            ++n;
        }
        out_values[i] = (n > 0u) ? (float)(sum / (double)n) : values[i];
    }
    return 1;
}

int UpeMath_ExponentialMovingAverageF32(const float *values, uint32_t count,
                                        float alpha, float *out_values) {
    if (!values || !out_values || count == 0u || alpha <= 0.0f || alpha > 1.0f) {
        return 0;
    }

    out_values[0] = values[0];
    for (uint32_t i = 1; i < count; ++i) {
        out_values[i] = alpha * values[i] + (1.0f - alpha) * out_values[i - 1u];
    }
    return 1;
}

int UpeMath_DetectOutlierMaskZScoreF32(const float *values, uint32_t count,
                                       float z_threshold,
                                       uint8_t *out_mask) {
    if (!values || !out_mask || count == 0u || z_threshold <= 0.0f) {
        return 0;
    }

    UpeStatMoments moments = {};
    if (!UpeMath_ComputeMomentsF32(values, count, &moments)) {
        return 0;
    }

    double stddev = moments.stddev;
    for (uint32_t i = 0; i < count; ++i) {
        if (stddev <= 1.0e-12) {
            out_mask[i] = 0u;
            continue;
        }
        double z = fabs(((double)values[i] - moments.mean) / stddev);
        out_mask[i] = (z >= (double)z_threshold) ? 1u : 0u;
    }
    return 1;
}

float UpeMath_TrimmedMeanF32(const float *values, uint32_t count, float trim_fraction) {
    if (!values || count == 0u) {
        return 0.0f;
    }

    if (trim_fraction < 0.0f) {
        trim_fraction = 0.0f;
    }
    if (trim_fraction >= 0.5f) {
        trim_fraction = 0.49f;
    }

    float *sorted = (float*)malloc(count * sizeof(float));
    if (!sorted) {
        return 0.0f;
    }
    memcpy(sorted, values, count * sizeof(float));
    qsort(sorted, count, sizeof(float), UpeMath_CompareFloatAscending);

    uint32_t trim = (uint32_t)floorf(trim_fraction * (float)count);
    if (trim * 2u >= count) {
        trim = 0u;
    }

    double sum = 0.0;
    uint32_t kept = 0u;
    for (uint32_t i = trim; i < count - trim; ++i) {
        sum += (double)sorted[i];
        ++kept;
    }

    free(sorted);
    return (kept > 0u) ? (float)(sum / (double)kept) : 0.0f;
}

float UpeMath_WeightedMeanF32(const float *values, const float *weights, uint32_t count) {
    if (!values || !weights || count == 0u) {
        return 0.0f;
    }

    double weighted_sum = 0.0;
    double total_weight = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double w = (weights[i] > 0.0f) ? (double)weights[i] : 0.0;
        weighted_sum += (double)values[i] * w;
        total_weight += w;
    }
    if (total_weight <= 1.0e-12) {
        return 0.0f;
    }
    return (float)(weighted_sum / total_weight);
}

int UpeMath_WeightedLeastSquaresFitLine(const float *xs, const float *ys, const float *weights,
                                        uint32_t count, UpeLeastSquaresFit1D *out_fit) {
    if (!xs || !ys || !weights || count < 2u || !out_fit) {
        return 0;
    }

    double sw = 0.0;
    double sx = 0.0;
    double sy = 0.0;
    double sxx = 0.0;
    double sxy = 0.0;
    double syy = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double w = (weights[i] > 0.0f) ? (double)weights[i] : 0.0;
        double x = (double)xs[i];
        double y = (double)ys[i];
        sw += w;
        sx += w * x;
        sy += w * y;
        sxx += w * x * x;
        sxy += w * x * y;
        syy += w * y * y;
    }
    if (sw <= 1.0e-12) {
        return 0;
    }

    double denom = (sw * sxx) - (sx * sx);
    if (fabs(denom) <= 1.0e-12) {
        return 0;
    }

    double slope = ((sw * sxy) - (sx * sy)) / denom;
    double intercept = (sy - slope * sx) / sw;
    double weighted_error = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double residual = ((slope * xs[i]) + intercept) - ys[i];
        weighted_error += ((weights[i] > 0.0f) ? (double)weights[i] : 0.0) * residual * residual;
    }
    double rms_error = sqrt(weighted_error / sw);
    double corr_denom = sqrt(fmax(0.0, ((sw * sxx) - (sx * sx)) * ((sw * syy) - (sy * sy))));
    double correlation = (corr_denom > 1.0e-12) ? (((sw * sxy) - (sx * sy)) / corr_denom) : 0.0;

    out_fit->slope = (float)slope;
    out_fit->intercept = (float)intercept;
    out_fit->rms_error = (float)rms_error;
    out_fit->correlation = (float)correlation;
    return 1;
}

float UpeMath_QuantileF32(const float *values, uint32_t count, float q) {
    if (!values || count == 0u) {
        return 0.0f;
    }

    float *sorted = (float*)malloc(count * sizeof(float));
    if (!sorted) {
        return 0.0f;
    }
    memcpy(sorted, values, count * sizeof(float));
    qsort(sorted, count, sizeof(float), UpeMath_CompareFloatAscending);
    float quantile = UpeMath_QuantileSorted(sorted, count, q);
    free(sorted);
    return quantile;
}

int UpeMath_HistogramF32(const float *values, uint32_t count,
                         float min_value, float max_value,
                         uint32_t bin_count, uint32_t *out_bins) {
    if (!values || !out_bins || count == 0u || bin_count == 0u || max_value <= min_value) {
        return 0;
    }

    memset(out_bins, 0, bin_count * sizeof(uint32_t));
    float scale = (float)bin_count / (max_value - min_value);
    for (uint32_t i = 0; i < count; ++i) {
        float normalized = (values[i] - min_value) * scale;
        int bin = (int)floorf(normalized);
        if (bin < 0) {
            bin = 0;
        }
        if ((uint32_t)bin >= bin_count) {
            bin = (int)bin_count - 1;
        }
        out_bins[bin] += 1u;
    }
    return 1;
}

float UpeMath_ShannonEntropyFromHistogram(const uint32_t *bins, uint32_t bin_count) {
    if (!bins || bin_count == 0u) {
        return 0.0f;
    }

    double total = 0.0;
    for (uint32_t i = 0; i < bin_count; ++i) {
        total += (double)bins[i];
    }
    if (total <= 0.0) {
        return 0.0f;
    }

    double entropy = 0.0;
    for (uint32_t i = 0; i < bin_count; ++i) {
        if (bins[i] == 0u) {
            continue;
        }
        double p = (double)bins[i] / total;
        entropy -= p * (log(p) / log(2.0));
    }
    return (float)entropy;
}

float UpeMath_CovarianceF32(const float *xs, const float *ys, uint32_t count) {
    if (!xs || !ys || count == 0u) {
        return 0.0f;
    }

    double mean_x = 0.0;
    double mean_y = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        mean_x += (double)xs[i];
        mean_y += (double)ys[i];
    }
    mean_x /= (double)count;
    mean_y /= (double)count;

    double covariance = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        covariance += ((double)xs[i] - mean_x) * ((double)ys[i] - mean_y);
    }
    covariance /= (double)count;
    return (float)covariance;
}

float UpeMath_CorrelationCoefficientF32(const float *xs, const float *ys, uint32_t count) {
    if (!xs || !ys || count < 2u) {
        return 0.0f;
    }

    double cov = (double)UpeMath_CovarianceF32(xs, ys, count);
    double var_x = 0.0;
    double var_y = 0.0;
    double mean_x = 0.0;
    double mean_y = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        mean_x += (double)xs[i];
        mean_y += (double)ys[i];
    }
    mean_x /= (double)count;
    mean_y /= (double)count;
    for (uint32_t i = 0; i < count; ++i) {
        double dx = (double)xs[i] - mean_x;
        double dy = (double)ys[i] - mean_y;
        var_x += dx * dx;
        var_y += dy * dy;
    }
    var_x /= (double)count;
    var_y /= (double)count;

    double denom = sqrt(var_x * var_y);
    if (denom <= 1.0e-12) {
        return 0.0f;
    }
    return (float)(cov / denom);
}

float UpeMath_MedianAbsoluteDeviationF32(const float *values, uint32_t count) {
    if (!values || count == 0u) {
        return 0.0f;
    }

    float *sorted = (float*)malloc(count * sizeof(float));
    float *deviations = (float*)malloc(count * sizeof(float));
    if (!sorted || !deviations) {
        free(sorted);
        free(deviations);
        return 0.0f;
    }

    memcpy(sorted, values, count * sizeof(float));
    qsort(sorted, count, sizeof(float), UpeMath_CompareFloatAscending);
    float median = UpeMath_QuantileSorted(sorted, count, 0.5f);
    for (uint32_t i = 0; i < count; ++i) {
        deviations[i] = fabsf(values[i] - median);
    }
    qsort(deviations, count, sizeof(float), UpeMath_CompareFloatAscending);
    float mad = UpeMath_QuantileSorted(deviations, count, 0.5f);

    free(sorted);
    free(deviations);
    return mad;
}

float UpeMath_RootMeanSquareErrorF32(const float *predicted, const float *observed, uint32_t count) {
    if (!predicted || !observed || count == 0u) {
        return 0.0f;
    }

    double mse = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        double delta = (double)predicted[i] - (double)observed[i];
        mse += delta * delta;
    }
    mse /= (double)count;
    return (float)sqrt(mse);
}

float UpeMath_NormalizedRMSEF32(const float *predicted, const float *observed, uint32_t count) {
    if (!predicted || !observed || count == 0u) {
        return 0.0f;
    }

    float rmse = UpeMath_RootMeanSquareErrorF32(predicted, observed, count);
    float min_value = observed[0];
    float max_value = observed[0];
    for (uint32_t i = 1; i < count; ++i) {
        if (observed[i] < min_value) {
            min_value = observed[i];
        }
        if (observed[i] > max_value) {
            max_value = observed[i];
        }
    }
    float range = max_value - min_value;
    if (fabsf(range) <= 1.0e-6f) {
        return rmse;
    }
    return rmse / range;
}

float UpeMath_KolmogorovSmirnovDistanceF32(const float *sample_a, uint32_t count_a,
                                           const float *sample_b, uint32_t count_b) {
    if (!sample_a || !sample_b || count_a == 0u || count_b == 0u) {
        return 0.0f;
    }

    float *a = (float*)malloc(count_a * sizeof(float));
    float *b = (float*)malloc(count_b * sizeof(float));
    if (!a || !b) {
        free(a);
        free(b);
        return 0.0f;
    }
    memcpy(a, sample_a, count_a * sizeof(float));
    memcpy(b, sample_b, count_b * sizeof(float));
    qsort(a, count_a, sizeof(float), UpeMath_CompareFloatAscending);
    qsort(b, count_b, sizeof(float), UpeMath_CompareFloatAscending);

    uint32_t ia = 0u;
    uint32_t ib = 0u;
    float max_distance = 0.0f;
    while (ia < count_a && ib < count_b) {
        float x = (a[ia] < b[ib]) ? a[ia] : b[ib];
        while (ia < count_a && a[ia] <= x) {
            ++ia;
        }
        while (ib < count_b && b[ib] <= x) {
            ++ib;
        }
        float cdf_a = (float)ia / (float)count_a;
        float cdf_b = (float)ib / (float)count_b;
        float distance = fabsf(cdf_a - cdf_b);
        if (distance > max_distance) {
            max_distance = distance;
        }
    }

    free(a);
    free(b);
    return max_distance;
}

float UpeMath_DominantFrequencyEstimate(const float *samples, uint32_t count, float sample_rate_hz) {
    if (!samples || count < 2 || sample_rate_hz <= 0.0f) {
        return 0.0f;
    }

    uint32_t half = count / 2u;
    float best_freq = 0.0f;
    double best_mag_sq = -1.0;
    for (uint32_t k = 1; k <= half; ++k) {
        double real = 0.0;
        double imag = 0.0;
        for (uint32_t n = 0; n < count; ++n) {
            double phase = (2.0 * 3.14159265358979323846 * (double)k * (double)n) / (double)count;
            real += (double)samples[n] * cos(phase);
            imag -= (double)samples[n] * sin(phase);
        }
        double mag_sq = (real * real) + (imag * imag);
        if (mag_sq > best_mag_sq) {
            best_mag_sq = mag_sq;
            best_freq = ((float)k * sample_rate_hz) / (float)count;
        }
    }
    return best_freq;
}

float UpeMath_ConditionNumber2x2(float a00, float a01, float a10, float a11) {
    double frob = sqrt((double)a00 * a00 + (double)a01 * a01 + (double)a10 * a10 + (double)a11 * a11);
    double det = ((double)a00 * (double)a11) - ((double)a01 * (double)a10);
    if (fabs(det) <= 1.0e-12) {
        return INFINITY;
    }
    double inv00 =  a11 / det;
    double inv01 = -a01 / det;
    double inv10 = -a10 / det;
    double inv11 =  a00 / det;
    double inv_frob = sqrt(inv00 * inv00 + inv01 * inv01 + inv10 * inv10 + inv11 * inv11);
    return (float)(frob * inv_frob);
}

int UpeMath_Laplacian1D(const float *values, uint32_t count, float dx, float *out_laplacian) {
    if (!values || !out_laplacian || count == 0u || dx <= 0.0f) {
        return 0;
    }

    float inv_dx2 = 1.0f / (dx * dx);
    if (count == 1u) {
        out_laplacian[0] = 0.0f;
        return 1;
    }

    out_laplacian[0] = 0.0f;
    for (uint32_t i = 1u; i + 1u < count; ++i) {
        out_laplacian[i] = (values[i - 1u] - 2.0f * values[i] + values[i + 1u]) * inv_dx2;
    }
    out_laplacian[count - 1u] = 0.0f;
    return 1;
}

int UpeMath_DiffusionStep1D(const float *values, uint32_t count,
                            float diffusivity, float dt, float dx,
                            float *out_values) {
    if (!values || !out_values || count == 0u || diffusivity < 0.0f || dt < 0.0f || dx <= 0.0f) {
        return 0;
    }

    float *laplacian = (float*)malloc(count * sizeof(float));
    if (!laplacian) {
        return 0;
    }
    if (!UpeMath_Laplacian1D(values, count, dx, laplacian)) {
        free(laplacian);
        return 0;
    }

    float scale = diffusivity * dt;
    for (uint32_t i = 0u; i < count; ++i) {
        out_values[i] = values[i] + scale * laplacian[i];
    }

    free(laplacian);
    return 1;
}

int UpeMath_PoissonJacobi1D(const float *rhs, uint32_t count, float dx,
                            uint32_t iterations, float *out_solution) {
    if (!rhs || !out_solution || count < 3u || dx <= 0.0f || iterations == 0u) {
        return 0;
    }

    float *current = (float*)calloc(count, sizeof(float));
    float *next = (float*)calloc(count, sizeof(float));
    if (!current || !next) {
        free(current);
        free(next);
        return 0;
    }

    float dx2 = dx * dx;
    for (uint32_t iter = 0u; iter < iterations; ++iter) {
        next[0] = 0.0f;
        next[count - 1u] = 0.0f;
        for (uint32_t i = 1u; i + 1u < count; ++i) {
            next[i] = 0.5f * (current[i - 1u] + current[i + 1u] + rhs[i] * dx2);
        }
        float *swap = current;
        current = next;
        next = swap;
    }

    memcpy(out_solution, current, count * sizeof(float));
    free(current);
    free(next);
    return 1;
}

float UpeMath_CourantNumber1D(float velocity, float dt, float dx) {
    if (dx <= 0.0f) {
        return 0.0f;
    }
    return fabsf(velocity) * dt / dx;
}

float UpeMath_DiffusionNumber1D(float diffusivity, float dt, float dx) {
    if (dx <= 0.0f) {
        return 0.0f;
    }
    return diffusivity * dt / (dx * dx);
}

int UpeMath_AdvectionStep1D(const float *values, uint32_t count,
                            float velocity, float dt, float dx,
                            float *out_values) {
    if (!values || !out_values || count == 0u || dt < 0.0f || dx <= 0.0f) {
        return 0;
    }
    if (count == 1u) {
        out_values[0] = values[0];
        return 1;
    }

    float courant = velocity * dt / dx;
    if (velocity >= 0.0f) {
        out_values[0] = values[0];
        for (uint32_t i = 1u; i < count; ++i) {
            out_values[i] = values[i] - courant * (values[i] - values[i - 1u]);
        }
    } else {
        for (uint32_t i = 0u; i + 1u < count; ++i) {
            out_values[i] = values[i] - courant * (values[i + 1u] - values[i]);
        }
        out_values[count - 1u] = values[count - 1u];
    }
    return 1;
}

int UpeMath_WaveStep1D(const float *current, const float *previous,
                       uint32_t count, float wave_speed,
                       float dt, float dx, float damping,
                       float *out_next) {
    if (!current || !previous || !out_next || count == 0u || wave_speed < 0.0f || dt < 0.0f || dx <= 0.0f || damping < 0.0f) {
        return 0;
    }
    if (count == 1u) {
        out_next[0] = current[0];
        return 1;
    }

    float coeff = (wave_speed * dt / dx);
    coeff *= coeff;
    out_next[0] = current[0];
    for (uint32_t i = 1u; i + 1u < count; ++i) {
        float laplacian = current[i - 1u] - 2.0f * current[i] + current[i + 1u];
        float velocity_term = current[i] - previous[i];
        out_next[i] = 2.0f * current[i] - previous[i] + coeff * laplacian - damping * dt * velocity_term;
    }
    out_next[count - 1u] = current[count - 1u];
    return 1;
}

int UpeMath_TridiagonalSolve(const float *lower, const float *diag,
                             const float *upper, const float *rhs,
                             uint32_t count, float *out_solution) {
    if (!lower || !diag || !upper || !rhs || !out_solution || count == 0u) {
        return 0;
    }

    float *cprime = (float*)malloc(count * sizeof(float));
    float *dprime = (float*)malloc(count * sizeof(float));
    if (!cprime || !dprime) {
        free(cprime);
        free(dprime);
        return 0;
    }

    if (fabsf(diag[0]) <= 1.0e-6f) {
        free(cprime);
        free(dprime);
        return 0;
    }

    cprime[0] = (count > 1u) ? (upper[0] / diag[0]) : 0.0f;
    dprime[0] = rhs[0] / diag[0];

    for (uint32_t i = 1u; i < count; ++i) {
        float denom = diag[i] - lower[i] * cprime[i - 1u];
        if (fabsf(denom) <= 1.0e-6f) {
            free(cprime);
            free(dprime);
            return 0;
        }
        cprime[i] = (i + 1u < count) ? (upper[i] / denom) : 0.0f;
        dprime[i] = (rhs[i] - lower[i] * dprime[i - 1u]) / denom;
    }

    out_solution[count - 1u] = dprime[count - 1u];
    for (uint32_t i = count - 1u; i-- > 0u;) {
        out_solution[i] = dprime[i] - cprime[i] * out_solution[i + 1u];
    }

    free(cprime);
    free(dprime);
    return 1;
}

int UpeMath_RestrictHalfWeighting1D(const float *fine_values, uint32_t fine_count,
                                    float *out_coarse_values, uint32_t coarse_count) {
    if (!fine_values || !out_coarse_values || fine_count == 0u || coarse_count == 0u || coarse_count > fine_count) {
        return 0;
    }
    if (coarse_count == 1u) {
        out_coarse_values[0] = fine_values[0];
        return 1;
    }

    for (uint32_t j = 0u; j < coarse_count; ++j) {
        uint32_t mapped = (uint32_t)((uint64_t)j * (uint64_t)(fine_count - 1u) / (uint64_t)(coarse_count - 1u));
        if (j == 0u || j + 1u == coarse_count || mapped == 0u || mapped + 1u >= fine_count) {
            out_coarse_values[j] = fine_values[mapped];
        } else {
            out_coarse_values[j] = 0.25f * fine_values[mapped - 1u] +
                                   0.5f * fine_values[mapped] +
                                   0.25f * fine_values[mapped + 1u];
        }
    }
    return 1;
}

int UpeMath_ProlongateLinear1D(const float *coarse_values, uint32_t coarse_count,
                               float *out_fine_values, uint32_t fine_count) {
    if (!coarse_values || !out_fine_values || coarse_count == 0u || fine_count == 0u) {
        return 0;
    }
    if (coarse_count == 1u) {
        for (uint32_t i = 0u; i < fine_count; ++i) {
            out_fine_values[i] = coarse_values[0];
        }
        return 1;
    }
    if (fine_count == 1u) {
        out_fine_values[0] = coarse_values[0];
        return 1;
    }

    for (uint32_t i = 0u; i < fine_count; ++i) {
        float coord = ((float)i * (float)(coarse_count - 1u)) / (float)(fine_count - 1u);
        uint32_t lower_index = (uint32_t)floorf(coord);
        uint32_t upper_index = (lower_index + 1u < coarse_count) ? (lower_index + 1u) : lower_index;
        float fraction = coord - (float)lower_index;
        out_fine_values[i] = coarse_values[lower_index] +
                             (coarse_values[upper_index] - coarse_values[lower_index]) * fraction;
    }
    return 1;
}

float UpeMath_LinearElasticStress1D(float strain, float youngs_modulus) {
    return strain * youngs_modulus;
}

float UpeMath_ShearModulus(float youngs_modulus, float poisson_ratio) {
    float denom = 2.0f * (1.0f + poisson_ratio);
    if (fabsf(denom) <= 1.0e-6f) {
        return 0.0f;
    }
    return youngs_modulus / denom;
}

float UpeMath_BulkModulus(float youngs_modulus, float poisson_ratio) {
    float denom = 3.0f * (1.0f - 2.0f * poisson_ratio);
    if (fabsf(denom) <= 1.0e-6f) {
        return 0.0f;
    }
    return youngs_modulus / denom;
}

float UpeMath_VonMisesPlaneStress(float sigma_xx, float sigma_yy, float tau_xy) {
    float invariant = sigma_xx * sigma_xx - sigma_xx * sigma_yy + sigma_yy * sigma_yy + 3.0f * tau_xy * tau_xy;
    return (invariant > 0.0f) ? sqrtf(invariant) : 0.0f;
}

float UpeMath_MaxwellStressStep(float stress, float strain_rate,
                                float youngs_modulus, float viscosity, float dt) {
    if (viscosity <= 0.0f || dt < 0.0f) {
        return stress;
    }

    float relaxation_rate = youngs_modulus / viscosity;
    return stress + dt * (youngs_modulus * strain_rate - relaxation_rate * stress);
}

float UpeMath_ElasticEnergyDensity1D(float strain, float youngs_modulus) {
    return 0.5f * youngs_modulus * strain * strain;
}

float UpeMath_YieldUtilizationVonMises(float sigma_xx, float sigma_yy,
                                       float tau_xy, float yield_strength) {
    if (yield_strength <= 1.0e-6f) {
        return 0.0f;
    }
    return UpeMath_VonMisesPlaneStress(sigma_xx, sigma_yy, tau_xy) / yield_strength;
}

float UpeMath_UpwindFlux1D(float left_value, float right_value, float velocity) {
    return (velocity >= 0.0f) ? (velocity * left_value) : (velocity * right_value);
}

float UpeMath_RusanovFlux1D(float left_value, float right_value, float wave_speed) {
    float flux_left = left_value;
    float flux_right = right_value;
    float lambda = fabsf(wave_speed);
    return 0.5f * (flux_left + flux_right) - 0.5f * lambda * (right_value - left_value);
}

int UpeMath_FiniteVolumeDivergence1D(const float *interface_flux, uint32_t cell_count,
                                     float dx, float *out_divergence) {
    if (!interface_flux || !out_divergence || cell_count == 0u || dx <= 0.0f) {
        return 0;
    }

    float inv_dx = 1.0f / dx;
    for (uint32_t i = 0u; i < cell_count; ++i) {
        out_divergence[i] = -(interface_flux[i + 1u] - interface_flux[i]) * inv_dx;
    }
    return 1;
}

int UpeMath_FiniteVolumeAdvectionStep1D(const float *values, uint32_t count,
                                        float velocity, float dt, float dx,
                                        float *out_values) {
    if (!values || !out_values || count == 0u || dt < 0.0f || dx <= 0.0f) {
        return 0;
    }

    float *interface_flux = (float*)malloc((count + 1u) * sizeof(float));
    float *divergence = (float*)malloc(count * sizeof(float));
    if (!interface_flux || !divergence) {
        free(interface_flux);
        free(divergence);
        return 0;
    }

    interface_flux[0] = velocity * values[0];
    for (uint32_t i = 1u; i < count; ++i) {
        interface_flux[i] = UpeMath_UpwindFlux1D(values[i - 1u], values[i], velocity);
    }
    interface_flux[count] = velocity * values[count - 1u];

    if (!UpeMath_FiniteVolumeDivergence1D(interface_flux, count, dx, divergence)) {
        free(interface_flux);
        free(divergence);
        return 0;
    }

    for (uint32_t i = 0u; i < count; ++i) {
        out_values[i] = values[i] + dt * divergence[i];
    }

    free(interface_flux);
    free(divergence);
    return 1;
}

int UpeMath_CrankNicolsonDiffusion1D(const float *values, uint32_t count,
                                     float diffusivity, float dt, float dx,
                                     float *out_values) {
    if (!values || !out_values || count < 3u || diffusivity < 0.0f || dt < 0.0f || dx <= 0.0f) {
        return 0;
    }

    float r = diffusivity * dt / (dx * dx);
    out_values[0] = values[0];
    out_values[count - 1u] = values[count - 1u];

    uint32_t n = count - 2u;
    float *lower = (float*)malloc(n * sizeof(float));
    float *diag = (float*)malloc(n * sizeof(float));
    float *upper = (float*)malloc(n * sizeof(float));
    float *rhs = (float*)malloc(n * sizeof(float));
    float *solution = (float*)malloc(n * sizeof(float));
    if (!lower || !diag || !upper || !rhs || !solution) {
        free(lower);
        free(diag);
        free(upper);
        free(rhs);
        free(solution);
        return 0;
    }

    for (uint32_t i = 0u; i < n; ++i) {
        lower[i] = (i == 0u) ? 0.0f : (-0.5f * r);
        diag[i] = 1.0f + r;
        upper[i] = (i + 1u == n) ? 0.0f : (-0.5f * r);

        uint32_t center = i + 1u;
        rhs[i] = (1.0f - r) * values[center] + 0.5f * r * (values[center - 1u] + values[center + 1u]);
    }

    if (!UpeMath_TridiagonalSolve(lower, diag, upper, rhs, n, solution)) {
        free(lower);
        free(diag);
        free(upper);
        free(rhs);
        free(solution);
        return 0;
    }

    for (uint32_t i = 0u; i < n; ++i) {
        out_values[i + 1u] = solution[i];
    }

    free(lower);
    free(diag);
    free(upper);
    free(rhs);
    free(solution);
    return 1;
}

int UpeMath_BarElementStiffness1D(float youngs_modulus, float area, float length,
                                  float *out_matrix4) {
    if (!out_matrix4 || length <= 0.0f) {
        return 0;
    }
    float k = youngs_modulus * area / length;
    out_matrix4[0] = k;
    out_matrix4[1] = -k;
    out_matrix4[2] = -k;
    out_matrix4[3] = k;
    return 1;
}

int UpeMath_BarElementMass1D(float density, float area, float length,
                             float *out_matrix4) {
    if (!out_matrix4 || length <= 0.0f) {
        return 0;
    }
    float factor = density * area * length / 6.0f;
    out_matrix4[0] = 2.0f * factor;
    out_matrix4[1] = factor;
    out_matrix4[2] = factor;
    out_matrix4[3] = 2.0f * factor;
    return 1;
}

float UpeMath_MinmodLimiter(float a, float b) {
    if (a * b <= 0.0f) {
        return 0.0f;
    }
    return (fabsf(a) < fabsf(b)) ? a : b;
}

float UpeMath_SuperbeeLimiter(float a, float b) {
    if (a * b <= 0.0f) {
        return 0.0f;
    }
    float sign = (a > 0.0f) ? 1.0f : -1.0f;
    float aa = fabsf(a);
    float bb = fabsf(b);
    float candidate1 = fminf(2.0f * aa, bb);
    float candidate2 = fminf(aa, 2.0f * bb);
    return sign * fmaxf(candidate1, candidate2);
}

int UpeMath_MusclReconstruct1D(const float *cell_values, uint32_t count,
                               float *out_left, float *out_right) {
    if (!cell_values || !out_left || !out_right || count == 0u) {
        return 0;
    }
    if (count == 1u) {
        out_left[0] = cell_values[0];
        out_right[0] = cell_values[0];
        return 1;
    }

    out_left[0] = cell_values[0];
    out_right[0] = cell_values[0];
    for (uint32_t i = 1u; i + 1u < count; ++i) {
        float slope_left = cell_values[i] - cell_values[i - 1u];
        float slope_right = cell_values[i + 1u] - cell_values[i];
        float limited = UpeMath_MinmodLimiter(slope_left, slope_right);
        out_left[i] = cell_values[i] - 0.5f * limited;
        out_right[i] = cell_values[i] + 0.5f * limited;
    }
    out_left[count - 1u] = cell_values[count - 1u];
    out_right[count - 1u] = cell_values[count - 1u];
    return 1;
}

float UpeMath_PerfectPlasticStress1D(float trial_stress, float yield_strength) {
    if (yield_strength < 0.0f) {
        yield_strength = -yield_strength;
    }
    return fmaxf(-yield_strength, fminf(yield_strength, trial_stress));
}

float UpeMath_DruckerPragerYieldFunction(float mean_stress, float deviatoric_q,
                                         float friction_coeff, float cohesion) {
    return deviatoric_q + friction_coeff * mean_stress - cohesion;
}

int UpeMath_BarElementInternalForce1D(float youngs_modulus, float area, float length,
                                      float displacement_a, float displacement_b,
                                      float *out_force_a, float *out_force_b) {
    if (!out_force_a || !out_force_b || length <= 0.0f) {
        return 0;
    }
    float k = youngs_modulus * area / length;
    float delta = displacement_b - displacement_a;
    *out_force_a = -k * delta;
    *out_force_b = k * delta;
    return 1;
}

int UpeMath_NewmarkBetaStep1D(float mass, float damping, float stiffness,
                              float external_force, float dt,
                              float beta, float gamma,
                              float *io_displacement,
                              float *io_velocity,
                              float *io_acceleration) {
    if (!io_displacement || !io_velocity || !io_acceleration || mass <= 0.0f || dt <= 0.0f || beta <= 0.0f || gamma < 0.0f) {
        return 0;
    }

    float d = *io_displacement;
    float v = *io_velocity;
    float a = *io_acceleration;

    float d_pred = d + dt * v + dt * dt * (0.5f - beta) * a;
    float v_pred = v + dt * (1.0f - gamma) * a;
    float effective_mass = mass + gamma * dt * damping + beta * dt * dt * stiffness;
    if (fabsf(effective_mass) <= 1.0e-6f) {
        return 0;
    }

    float a_new = (external_force - damping * v_pred - stiffness * d_pred) / effective_mass;
    float d_new = d_pred + beta * dt * dt * a_new;
    float v_new = v_pred + gamma * dt * a_new;

    *io_displacement = d_new;
    *io_velocity = v_new;
    *io_acceleration = a_new;
    return 1;
}

int UpeMath_PoissonGaussSeidel1D(const float *rhs, uint32_t count, float dx,
                                 uint32_t iterations, float *out_solution) {
    if (!rhs || !out_solution || count < 3u || dx <= 0.0f || iterations == 0u) {
        return 0;
    }

    memset(out_solution, 0, count * sizeof(float));
    float dx2 = dx * dx;
    for (uint32_t iter = 0u; iter < iterations; ++iter) {
        out_solution[0] = 0.0f;
        out_solution[count - 1u] = 0.0f;
        for (uint32_t i = 1u; i + 1u < count; ++i) {
            out_solution[i] = 0.5f * (out_solution[i - 1u] + out_solution[i + 1u] + rhs[i] * dx2);
        }
    }
    return 1;
}

int UpeMath_PoissonResidual1D(const float *solution, const float *rhs, uint32_t count,
                              float dx, float *out_residual) {
    if (!solution || !rhs || !out_residual || count < 3u || dx <= 0.0f) {
        return 0;
    }

    float inv_dx2 = 1.0f / (dx * dx);
    out_residual[0] = 0.0f;
    for (uint32_t i = 1u; i + 1u < count; ++i) {
        float laplacian = (solution[i - 1u] - 2.0f * solution[i] + solution[i + 1u]) * inv_dx2;
        out_residual[i] = rhs[i] + laplacian;
    }
    out_residual[count - 1u] = 0.0f;
    return 1;
}

float UpeMath_NaturalFrequency1D(float stiffness, float mass) {
    if (mass <= 0.0f || stiffness < 0.0f) {
        return 0.0f;
    }
    return sqrtf(stiffness / mass);
}

float UpeMath_DampingRatio1D(float mass, float damping, float stiffness) {
    if (mass <= 0.0f || stiffness <= 0.0f) {
        return 0.0f;
    }
    return damping / (2.0f * sqrtf(mass * stiffness));
}

int UpeMath_TwoGridPoissonCorrection1D(const float *rhs, uint32_t count, float dx,
                                       uint32_t pre_smooth_iters,
                                       uint32_t post_smooth_iters,
                                       float *out_solution) {
    if (!rhs || !out_solution || count < 3u || dx <= 0.0f) {
        return 0;
    }

    if (!UpeMath_PoissonGaussSeidel1D(rhs, count, dx, (pre_smooth_iters > 0u) ? pre_smooth_iters : 1u, out_solution)) {
        return 0;
    }

    float *residual = (float*)malloc(count * sizeof(float));
    if (!residual) {
        return 0;
    }
    if (!UpeMath_PoissonResidual1D(out_solution, rhs, count, dx, residual)) {
        free(residual);
        return 0;
    }

    uint32_t coarse_count = (count + 1u) / 2u;
    float *coarse_rhs = (float*)malloc(coarse_count * sizeof(float));
    float *coarse_solution = (float*)calloc(coarse_count, sizeof(float));
    float *correction = (float*)malloc(count * sizeof(float));
    if (!coarse_rhs || !coarse_solution || !correction) {
        free(residual);
        free(coarse_rhs);
        free(coarse_solution);
        free(correction);
        return 0;
    }

    if (!UpeMath_RestrictHalfWeighting1D(residual, count, coarse_rhs, coarse_count) ||
        !UpeMath_PoissonGaussSeidel1D(coarse_rhs, coarse_count, dx * 2.0f, (pre_smooth_iters + post_smooth_iters > 0u) ? (pre_smooth_iters + post_smooth_iters) : 1u, coarse_solution) ||
        !UpeMath_ProlongateLinear1D(coarse_solution, coarse_count, correction, count)) {
        free(residual);
        free(coarse_rhs);
        free(coarse_solution);
        free(correction);
        return 0;
    }

    for (uint32_t i = 0u; i < count; ++i) {
        out_solution[i] += correction[i];
    }

    if (!UpeMath_PoissonGaussSeidel1D(rhs, count, dx, (post_smooth_iters > 0u) ? post_smooth_iters : 1u, out_solution)) {
        free(residual);
        free(coarse_rhs);
        free(coarse_solution);
        free(correction);
        return 0;
    }

    free(residual);
    free(coarse_rhs);
    free(coarse_solution);
    free(correction);
    return 1;
}

int UpeMath_BarElementLoad1D(float body_force_density, float length,
                             float *out_vector2) {
    if (!out_vector2 || length <= 0.0f) {
        return 0;
    }
    float value = 0.5f * body_force_density * length;
    out_vector2[0] = value;
    out_vector2[1] = value;
    return 1;
}

float UpeMath_KelvinVoigtStress1D(float strain, float strain_rate,
                                  float youngs_modulus, float viscosity) {
    return youngs_modulus * strain + viscosity * strain_rate;
}

float UpeMath_CriticalTimeStepSpringMass(float mass, float stiffness) {
    if (mass <= 0.0f || stiffness <= 0.0f) {
        return 0.0f;
    }
    return 2.0f / sqrtf(stiffness / mass);
}

float UpeMath_CourantWaveStability1D(float wave_speed, float dt, float dx) {
    if (dx <= 0.0f) {
        return 0.0f;
    }
    return fabsf(wave_speed) * dt / dx;
}

float UpeMath_PhaseVelocityRatio1D(float courant_number, float nondim_wavenumber) {
    float cfl = fabsf(courant_number);
    float theta = fabsf(nondim_wavenumber);
    if (cfl <= 1.0e-6f || theta <= 1.0e-6f) {
        return 1.0f;
    }

    float dispersion_argument = cfl * sinf(0.5f * theta);
    dispersion_argument = fmaxf(-1.0f, fminf(1.0f, dispersion_argument));
    return (2.0f * asinf(dispersion_argument)) / (cfl * theta);
}

float UpeMath_NumericalDispersion1D(float courant_number, float nondim_wavenumber) {
    return UpeMath_PhaseVelocityRatio1D(courant_number, nondim_wavenumber) - 1.0f;
}

int UpeMath_AssembleBarChainStiffness1D(const float *element_stiffness, uint32_t element_count,
                                        float *out_matrix, uint32_t matrix_dim) {
    if (!element_stiffness || !out_matrix || matrix_dim == 0u || matrix_dim != element_count + 1u) {
        return 0;
    }

    memset(out_matrix, 0, matrix_dim * matrix_dim * sizeof(float));
    for (uint32_t e = 0u; e < element_count; ++e) {
        float k = element_stiffness[e];
        uint32_t i = e;
        uint32_t j = e + 1u;
        out_matrix[i * matrix_dim + i] += k;
        out_matrix[i * matrix_dim + j] -= k;
        out_matrix[j * matrix_dim + i] -= k;
        out_matrix[j * matrix_dim + j] += k;
    }
    return 1;
}

float UpeMath_GershgorinSpectralRadius(const float *matrix, uint32_t dim) {
    if (!matrix || dim == 0u) {
        return 0.0f;
    }

    float max_radius = 0.0f;
    for (uint32_t i = 0u; i < dim; ++i) {
        float row_sum = 0.0f;
        for (uint32_t j = 0u; j < dim; ++j) {
            if (i != j) {
                row_sum += fabsf(matrix[i * dim + j]);
            }
        }
        float disc_radius = fabsf(matrix[i * dim + i]) + row_sum;
        if (disc_radius > max_radius) {
            max_radius = disc_radius;
        }
    }
    return max_radius;
}

int UpeMath_PowerIterationSymmetric(const float *matrix, uint32_t dim, uint32_t iterations,
                                    float *out_eigenvalue, float *out_eigenvector) {
    if (!matrix || !out_eigenvalue || !out_eigenvector || dim == 0u) {
        return 0;
    }

    float *vector = (float*)malloc(dim * sizeof(float));
    float *next = (float*)malloc(dim * sizeof(float));
    if (!vector || !next) {
        free(vector);
        free(next);
        return 0;
    }

    float inv_norm0 = 1.0f / sqrtf((float)dim);
    for (uint32_t i = 0u; i < dim; ++i) {
        vector[i] = inv_norm0;
    }

    uint32_t iter_count = (iterations > 0u) ? iterations : 1u;
    for (uint32_t iter = 0u; iter < iter_count; ++iter) {
        for (uint32_t i = 0u; i < dim; ++i) {
            float sum = 0.0f;
            for (uint32_t j = 0u; j < dim; ++j) {
                sum += matrix[i * dim + j] * vector[j];
            }
            next[i] = sum;
        }

        float norm_sq = 0.0f;
        for (uint32_t i = 0u; i < dim; ++i) {
            norm_sq += next[i] * next[i];
        }
        if (norm_sq <= 1.0e-12f) {
            free(vector);
            free(next);
            return 0;
        }

        float inv_norm = 1.0f / sqrtf(norm_sq);
        for (uint32_t i = 0u; i < dim; ++i) {
            vector[i] = next[i] * inv_norm;
        }
    }

    float *Av = next;
    for (uint32_t i = 0u; i < dim; ++i) {
        float sum = 0.0f;
        for (uint32_t j = 0u; j < dim; ++j) {
            sum += matrix[i * dim + j] * vector[j];
        }
        Av[i] = sum;
    }

    float numerator = 0.0f;
    float denominator = 0.0f;
    for (uint32_t i = 0u; i < dim; ++i) {
        numerator += vector[i] * Av[i];
        denominator += vector[i] * vector[i];
    }
    *out_eigenvalue = (denominator > 1.0e-12f) ? (numerator / denominator) : 0.0f;

    for (uint32_t i = 0u; i < dim; ++i) {
        out_eigenvector[i] = vector[i];
    }

    free(vector);
    free(next);
    return 1;
}

float UpeMath_RayleighDamping1D(float mass_coeff, float stiffness_coeff, float mass, float stiffness) {
    if (mass < 0.0f || stiffness < 0.0f) {
        return 0.0f;
    }
    return mass_coeff * mass + stiffness_coeff * stiffness;
}

int UpeMath_BeamElementStiffnessEulerBernoulli(float flexural_rigidity, float length,
                                               float *out_matrix16) {
    if (!out_matrix16 || flexural_rigidity <= 0.0f || length <= 0.0f) {
        return 0;
    }

    float L = length;
    float L2 = L * L;
    float scale = flexural_rigidity / (L2 * L);
    const float base[16] = {
         12.0f,   6.0f * L, -12.0f,   6.0f * L,
          6.0f * L, 4.0f * L2,  -6.0f * L, 2.0f * L2,
        -12.0f,  -6.0f * L,  12.0f,  -6.0f * L,
          6.0f * L, 2.0f * L2,  -6.0f * L, 4.0f * L2
    };
    for (uint32_t i = 0u; i < 16u; ++i) {
        out_matrix16[i] = scale * base[i];
    }
    return 1;
}

int UpeMath_PowerIterationSymmetric3x3(const float *matrix9, uint32_t iterations,
                                       float *out_eigenvalue, float *out_vector3) {
    return UpeMath_PowerIterationSymmetric(matrix9, 3u, iterations, out_eigenvalue, out_vector3);
}

int UpeMath_RayleighDampingCoefficients(float damping_ratio_a, float omega_a,
                                        float damping_ratio_b, float omega_b,
                                        float *out_alpha, float *out_beta) {
    if (!out_alpha || !out_beta || omega_a <= 0.0f || omega_b <= 0.0f || fabsf(omega_a - omega_b) <= 1.0e-6f) {
        return 0;
    }

    float a11 = 1.0f / omega_a;
    float a12 = omega_a;
    float a21 = 1.0f / omega_b;
    float a22 = omega_b;
    float b1 = 2.0f * damping_ratio_a;
    float b2 = 2.0f * damping_ratio_b;
    float det = a11 * a22 - a12 * a21;
    if (fabsf(det) <= 1.0e-9f) {
        return 0;
    }

    *out_alpha = (b1 * a22 - a12 * b2) / det;
    *out_beta = (a11 * b2 - b1 * a21) / det;
    return 1;
}

float UpeMath_EulerBucklingLoadPinnedPinned(float youngs_modulus, float area_moment,
                                            float length) {
    if (youngs_modulus <= 0.0f || area_moment <= 0.0f || length <= 0.0f) {
        return 0.0f;
    }

    const float pi = 3.14159265358979323846f;
    return (pi * pi * youngs_modulus * area_moment) / (length * length);
}

static int UpeMeshTopology_FindEdge(const uint32_t *edge_a,
                                    const uint32_t *edge_b,
                                    uint32_t edge_count,
                                    uint32_t a,
                                    uint32_t b) {
    for (uint32_t i = 0u; i < edge_count; ++i) {
        if (edge_a[i] == a && edge_b[i] == b) {
            return (int)i;
        }
    }
    return -1;
}

static uint32_t UpeMeshTopology_FindRoot(uint32_t *parent, uint32_t node) {
    while (parent[node] != node) {
        parent[node] = parent[parent[node]];
        node = parent[node];
    }
    return node;
}

static void UpeMeshTopology_Union(uint32_t *parent, uint32_t a, uint32_t b) {
    uint32_t root_a = UpeMeshTopology_FindRoot(parent, a);
    uint32_t root_b = UpeMeshTopology_FindRoot(parent, b);
    if (root_a != root_b) {
        parent[root_b] = root_a;
    }
}

int UpeMesh_AnalyzeTopology(const UpeMesh *mesh, UpeMeshTopologySummary *out_summary) {
    if (!mesh || !mesh->vertices || !mesh->indices || !out_summary) {
        return 0;
    }

    memset(out_summary, 0, sizeof(*out_summary));
    out_summary->vertex_count = mesh->vertex_count;
    out_summary->triangle_count = mesh->triangle_count;
    if (mesh->triangle_count == 0u) {
        out_summary->connected_components = (mesh->vertex_count > 0u) ? mesh->vertex_count : 0u;
        out_summary->is_watertight = 0;
        out_summary->is_orientable = 1;
        out_summary->euler_characteristic = (int)mesh->vertex_count;
        return 1;
    }

    uint32_t max_edges = mesh->triangle_count * 3u;
    uint32_t *edge_a = (uint32_t*)malloc(max_edges * sizeof(uint32_t));
    uint32_t *edge_b = (uint32_t*)malloc(max_edges * sizeof(uint32_t));
    uint32_t *edge_counts = (uint32_t*)calloc(max_edges, sizeof(uint32_t));
    int *edge_orientation = (int*)calloc(max_edges, sizeof(int));
    uint32_t *parent = (uint32_t*)malloc(mesh->vertex_count * sizeof(uint32_t));
    uint8_t *used_vertex = (uint8_t*)calloc(mesh->vertex_count, sizeof(uint8_t));
    if (!edge_a || !edge_b || !edge_counts || !edge_orientation || !parent || !used_vertex) {
        free(edge_a); free(edge_b); free(edge_counts); free(edge_orientation); free(parent); free(used_vertex);
        return 0;
    }

    for (uint32_t i = 0u; i < mesh->vertex_count; ++i) {
        parent[i] = i;
    }

    uint32_t edge_count = 0u;
    int orientation_issues = 0;
    for (uint32_t tri = 0u; tri < mesh->triangle_count; ++tri) {
        uint32_t i0 = mesh->indices[tri * 3u + 0u];
        uint32_t i1 = mesh->indices[tri * 3u + 1u];
        uint32_t i2 = mesh->indices[tri * 3u + 2u];
        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) {
            free(edge_a); free(edge_b); free(edge_counts); free(edge_orientation); free(parent); free(used_vertex);
            return 0;
        }

        used_vertex[i0] = used_vertex[i1] = used_vertex[i2] = 1u;
        UpeMeshTopology_Union(parent, i0, i1);
        UpeMeshTopology_Union(parent, i1, i2);

        if (i0 == i1 || i1 == i2 || i2 == i0) {
            out_summary->has_degenerate_faces = 1;
        }

        const float *v0 = &mesh->vertices[i0 * 3u];
        const float *v1 = &mesh->vertices[i1 * 3u];
        const float *v2 = &mesh->vertices[i2 * 3u];
        float ax = v1[0] - v0[0];
        float ay = v1[1] - v0[1];
        float az = v1[2] - v0[2];
        float bx = v2[0] - v0[0];
        float by = v2[1] - v0[1];
        float bz = v2[2] - v0[2];
        float cx = ay * bz - az * by;
        float cy = az * bx - ax * bz;
        float cz = ax * by - ay * bx;
        float area2 = cx * cx + cy * cy + cz * cz;
        if (area2 <= 1.0e-12f) {
            out_summary->has_degenerate_faces = 1;
        }

        uint32_t tri_edges[3][2] = {{i0, i1}, {i1, i2}, {i2, i0}};
        for (uint32_t e = 0u; e < 3u; ++e) {
            uint32_t a = tri_edges[e][0];
            uint32_t b = tri_edges[e][1];
            uint32_t lo = (a < b) ? a : b;
            uint32_t hi = (a < b) ? b : a;
            int orientation = (a == lo) ? 1 : -1;
            int edge_index = UpeMeshTopology_FindEdge(edge_a, edge_b, edge_count, lo, hi);
            if (edge_index < 0) {
                edge_a[edge_count] = lo;
                edge_b[edge_count] = hi;
                edge_counts[edge_count] = 1u;
                edge_orientation[edge_count] = orientation;
                edge_count++;
            } else {
                edge_counts[edge_index] += 1u;
                edge_orientation[edge_index] += orientation;
            }
        }
    }

    uint32_t boundary_edges = 0u;
    uint32_t nonmanifold_edges = 0u;
    for (uint32_t i = 0u; i < edge_count; ++i) {
        if (edge_counts[i] == 1u) {
            boundary_edges++;
        } else if (edge_counts[i] > 2u) {
            nonmanifold_edges++;
        }
        if (edge_counts[i] == 2u && edge_orientation[i] != 0) {
            orientation_issues++;
        }
    }

    uint32_t connected_components = 0u;
    for (uint32_t i = 0u; i < mesh->vertex_count; ++i) {
        if (used_vertex[i] && UpeMeshTopology_FindRoot(parent, i) == i) {
            connected_components++;
        }
    }

    out_summary->edge_count = edge_count;
    out_summary->boundary_edge_count = boundary_edges;
    out_summary->nonmanifold_edge_count = nonmanifold_edges;
    out_summary->connected_components = connected_components;
    out_summary->is_watertight = (boundary_edges == 0u && nonmanifold_edges == 0u && !out_summary->has_degenerate_faces) ? 1 : 0;
    out_summary->is_orientable = (orientation_issues == 0 && nonmanifold_edges == 0u && !out_summary->has_degenerate_faces) ? 1 : 0;
    out_summary->euler_characteristic = (int)mesh->vertex_count - (int)edge_count + (int)mesh->triangle_count;

    free(edge_a);
    free(edge_b);
    free(edge_counts);
    free(edge_orientation);
    free(parent);
    free(used_vertex);
    return 1;
}

static double UpeMath_DotF32(const float *a, const float *b, uint32_t n) {
    double dot = 0.0;
    for (uint32_t i = 0; i < n; ++i) {
        dot += (double)a[i] * (double)b[i];
    }
    return dot;
}

static float UpeMath_NormF32(const float *v, uint32_t n) {
    return (float)sqrt(UpeMath_DotF32(v, v, n));
}

void UpeSparse_Init(UpeSparseMatrix *mat, uint32_t rows, uint32_t cols, uint32_t nnz) {
    if (!mat) {
        return;
    }

    mat->values = nullptr;
    mat->col_indices = nullptr;
    mat->row_ptr = nullptr;
    mat->rows = rows;
    mat->cols = cols;
    mat->nnz = (nnz > 0u) ? nnz : 1u;

    mat->values = (float*)calloc(mat->nnz, sizeof(float));
    mat->col_indices = (uint32_t*)malloc(mat->nnz * sizeof(uint32_t));
    mat->row_ptr = (uint32_t*)calloc(rows + 1u, sizeof(uint32_t));
    if (!mat->values || !mat->col_indices || !mat->row_ptr) {
        UpeSparse_Destroy(mat);
        return;
    }
    for (uint32_t i = 0; i < mat->nnz; ++i) {
        mat->col_indices[i] = 0xFFFFFFFFu;
    }
}

void UpeSparse_Destroy(UpeSparseMatrix *mat) {
    if (!mat) {
        return;
    }
    free(mat->values);
    free(mat->col_indices);
    free(mat->row_ptr);
    mat->values = nullptr;
    mat->col_indices = nullptr;
    mat->row_ptr = nullptr;
    mat->rows = 0;
    mat->cols = 0;
    mat->nnz = 0;
}

void UpeSparse_SetValue(UpeSparseMatrix *mat, uint32_t row, uint32_t col, float value) {
    if (!mat || !mat->values || !mat->col_indices || !mat->row_ptr || row >= mat->rows || col >= mat->cols) {
        return;
    }

    uint32_t row_begin = mat->row_ptr[row];
    uint32_t row_end = mat->row_ptr[row + 1u];
    for (uint32_t i = row_begin; i < row_end; ++i) {
        if (mat->col_indices[i] == col) {
            mat->values[i] = value;
            return;
        }
    }

    uint32_t used = mat->row_ptr[mat->rows];
    if (used >= mat->nnz) {
        return;
    }

    for (uint32_t i = used; i > row_end; --i) {
        mat->values[i] = mat->values[i - 1u];
        mat->col_indices[i] = mat->col_indices[i - 1u];
    }
    mat->values[row_end] = value;
    mat->col_indices[row_end] = col;
    for (uint32_t r = row + 1u; r <= mat->rows; ++r) {
        mat->row_ptr[r] += 1u;
    }
}

void UpeSparse_MatVecMul(const UpeSparseMatrix *mat, const float *x, float *result) {
    if (!mat || !x || !result || !mat->row_ptr) {
        return;
    }

    for (uint32_t row = 0; row < mat->rows; ++row) {
        double sum = 0.0;
        uint32_t row_begin = mat->row_ptr[row];
        uint32_t row_end = mat->row_ptr[row + 1u];
        for (uint32_t i = row_begin; i < row_end; ++i) {
            uint32_t col = mat->col_indices[i];
            if (col == 0xFFFFFFFFu || col >= mat->cols) {
                continue;
            }
            sum += (double)mat->values[i] * (double)x[col];
        }
        result[row] = (float)sum;
    }
}

int UpeMath_ConjugateGradient(const UpeSparseMatrix *A, const float *b, float *x,
                              uint32_t max_iters, float tolerance) {
    if (!A || !b || !x || A->rows == 0 || A->rows != A->cols) {
        return 0;
    }

    uint32_t n = A->rows;
    float *r = (float*)malloc(n * sizeof(float));
    float *p = (float*)malloc(n * sizeof(float));
    float *Ap = (float*)malloc(n * sizeof(float));
    if (!r || !p || !Ap) {
        free(r);
        free(p);
        free(Ap);
        return 0;
    }

    UpeSparse_MatVecMul(A, x, Ap);
    for (uint32_t i = 0; i < n; ++i) {
        r[i] = b[i] - Ap[i];
        p[i] = r[i];
    }

    double rsold = UpeMath_DotF32(r, r, n);
    if (sqrt(rsold) <= (double)tolerance) {
        free(r); free(p); free(Ap);
        return 1;
    }

    for (uint32_t iter = 0; iter < max_iters; ++iter) {
        UpeSparse_MatVecMul(A, p, Ap);
        double denom = UpeMath_DotF32(p, Ap, n);
        if (fabs(denom) <= 1.0e-12) {
            break;
        }

        double alpha = rsold / denom;
        for (uint32_t i = 0; i < n; ++i) {
            x[i] = (float)(x[i] + alpha * p[i]);
            r[i] = (float)(r[i] - alpha * Ap[i]);
        }

        double rsnew = UpeMath_DotF32(r, r, n);
        if (sqrt(rsnew) <= (double)tolerance) {
            free(r); free(p); free(Ap);
            return (int)(iter + 1u);
        }

        double beta = rsnew / rsold;
        for (uint32_t i = 0; i < n; ++i) {
            p[i] = (float)(r[i] + beta * p[i]);
        }
        rsold = rsnew;
    }

    free(r); free(p); free(Ap);
    return (int)max_iters;
}

int UpeMath_GMRES(const UpeSparseMatrix *A, const float *b, float *x,
                  uint32_t max_iters, float tolerance, uint32_t restart) {
    (void)restart;
    if (!A || !b || !x || A->rows == 0) {
        return 0;
    }

    uint32_t n = A->rows;
    float *r = (float*)malloc(n * sizeof(float));
    float *Ar = (float*)malloc(n * sizeof(float));
    if (!r || !Ar) {
        free(r);
        free(Ar);
        return 0;
    }

    for (uint32_t iter = 0; iter < max_iters; ++iter) {
        UpeSparse_MatVecMul(A, x, Ar);
        for (uint32_t i = 0; i < n; ++i) {
            r[i] = b[i] - Ar[i];
        }

        float residual_norm = UpeMath_NormF32(r, n);
        if (residual_norm <= tolerance) {
            free(r);
            free(Ar);
            return (int)(iter + 1u);
        }

        UpeSparse_MatVecMul(A, r, Ar);
        double denom = UpeMath_DotF32(Ar, Ar, n);
        if (fabs(denom) <= 1.0e-12) {
            break;
        }
        double alpha = UpeMath_DotF32(Ar, r, n) / denom;
        for (uint32_t i = 0; i < n; ++i) {
            x[i] = (float)(x[i] + alpha * r[i]);
        }
    }

    free(r);
    free(Ar);
    return (int)max_iters;
}

void UpeMath_IncompleteCholesky(const UpeSparseMatrix *A, UpeSparseMatrix *L) {
    if (!A || !L || A->rows == 0) {
        return;
    }

    UpeSparse_Init(L, A->rows, A->cols, A->rows);
    for (uint32_t row = 0; row < A->rows; ++row) {
        float diag = 1.0f;
        if (A->row_ptr) {
            uint32_t begin = A->row_ptr[row];
            uint32_t end = A->row_ptr[row + 1u];
            for (uint32_t i = begin; i < end; ++i) {
                if (A->col_indices[i] == row) {
                    diag = A->values[i];
                    break;
                }
            }
        }
        if (diag < 1.0e-8f) {
            diag = 1.0e-8f;
        }
        UpeSparse_SetValue(L, row, row, sqrtf(diag));
    }
}

void UpeMath_EulerStep(UpeOdeRhsFunc rhs, float t, float *y, uint32_t n, float dt, void *user_data) {
    if (!rhs || !y || n == 0u) {
        return;
    }
    float *k1 = (float*)malloc(n * sizeof(float));
    if (!k1) {
        return;
    }
    rhs(t, y, k1, user_data);
    for (uint32_t i = 0; i < n; ++i) {
        y[i] += dt * k1[i];
    }
    free(k1);
}

void UpeMath_RK4Step(UpeOdeRhsFunc rhs, float t, float *y, uint32_t n, float dt, void *user_data) {
    if (!rhs || !y || n == 0u) {
        return;
    }

    float *k1 = (float*)malloc(n * sizeof(float));
    float *k2 = (float*)malloc(n * sizeof(float));
    float *k3 = (float*)malloc(n * sizeof(float));
    float *k4 = (float*)malloc(n * sizeof(float));
    float *tmp = (float*)malloc(n * sizeof(float));
    if (!k1 || !k2 || !k3 || !k4 || !tmp) {
        free(k1); free(k2); free(k3); free(k4); free(tmp);
        return;
    }

    rhs(t, y, k1, user_data);
    for (uint32_t i = 0; i < n; ++i) {
        tmp[i] = y[i] + 0.5f * dt * k1[i];
    }
    rhs(t + 0.5f * dt, tmp, k2, user_data);
    for (uint32_t i = 0; i < n; ++i) {
        tmp[i] = y[i] + 0.5f * dt * k2[i];
    }
    rhs(t + 0.5f * dt, tmp, k3, user_data);
    for (uint32_t i = 0; i < n; ++i) {
        tmp[i] = y[i] + dt * k3[i];
    }
    rhs(t + dt, tmp, k4, user_data);

    for (uint32_t i = 0; i < n; ++i) {
        y[i] += (dt / 6.0f) * (k1[i] + 2.0f * k2[i] + 2.0f * k3[i] + k4[i]);
    }

    free(k1); free(k2); free(k3); free(k4); free(tmp);
}

int UpeMath_RK45AdaptiveStep(UpeOdeRhsFunc rhs, float *t, float *y, uint32_t n,
                             float *dt, float tolerance, void *user_data) {
    if (!rhs || !t || !y || !dt || n == 0u || *dt <= 0.0f) {
        return 0;
    }

    float local_dt = *dt;
    float *y_full = (float*)malloc(n * sizeof(float));
    float *y_half = (float*)malloc(n * sizeof(float));
    if (!y_full || !y_half) {
        free(y_full);
        free(y_half);
        return 0;
    }

    for (uint32_t attempt = 0; attempt < 8u; ++attempt) {
        memcpy(y_full, y, n * sizeof(float));
        memcpy(y_half, y, n * sizeof(float));

        UpeMath_RK4Step(rhs, *t, y_full, n, local_dt, user_data);
        UpeMath_RK4Step(rhs, *t, y_half, n, 0.5f * local_dt, user_data);
        UpeMath_RK4Step(rhs, *t + 0.5f * local_dt, y_half, n, 0.5f * local_dt, user_data);

        float max_error = 0.0f;
        for (uint32_t i = 0; i < n; ++i) {
            float err = fabsf(y_half[i] - y_full[i]);
            if (err > max_error) {
                max_error = err;
            }
        }

        if (max_error <= tolerance || local_dt <= 1.0e-7f) {
            memcpy(y, y_half, n * sizeof(float));
            *t += local_dt;
            if (max_error < tolerance * 0.1f) {
                *dt = local_dt * 1.5f;
            } else {
                *dt = local_dt;
            }
            free(y_full);
            free(y_half);
            return 1;
        }

        local_dt *= 0.5f;
    }

    free(y_full);
    free(y_half);
    return 0;
}

int UpeMath_GradientDescent(UpeObjectiveFunc func, float *x, uint32_t n,
                            float learning_rate, uint32_t max_iters, float tolerance, void *user_data) {
    if (!func || !x || n == 0u || learning_rate <= 0.0f) {
        return 0;
    }

    float *gradient = (float*)malloc(n * sizeof(float));
    float *trial = (float*)malloc(n * sizeof(float));
    if (!gradient || !trial) {
        free(gradient);
        free(trial);
        return 0;
    }

    for (uint32_t iter = 0; iter < max_iters; ++iter) {
        float value = func(x, gradient, n, user_data);
        float grad_norm = UpeMath_NormF32(gradient, n);
        if (grad_norm <= tolerance) {
            free(gradient);
            free(trial);
            return (int)(iter + 1u);
        }

        float step = learning_rate;
        int accepted = 0;
        for (uint32_t backtrack = 0; backtrack < 8u; ++backtrack) {
            for (uint32_t i = 0; i < n; ++i) {
                trial[i] = x[i] - step * gradient[i];
            }
            float trial_value = func(trial, nullptr, n, user_data);
            if (trial_value < value) {
                memcpy(x, trial, n * sizeof(float));
                accepted = 1;
                break;
            }
            step *= 0.5f;
        }

        if (!accepted) {
            for (uint32_t i = 0; i < n; ++i) {
                x[i] -= 0.25f * learning_rate * gradient[i];
            }
        }
    }

    free(gradient);
    free(trial);
    return (int)max_iters;
}

int UpeMath_Newton(UpeObjectiveFunc func, float *x, uint32_t n,
                   uint32_t max_iters, float tolerance, void *user_data) {
    if (!func || !x || n == 0u) {
        return 0;
    }

    float *gradient = (float*)malloc(n * sizeof(float));
    float *gradient_shifted = (float*)malloc(n * sizeof(float));
    float *trial = (float*)malloc(n * sizeof(float));
    if (!gradient || !gradient_shifted || !trial) {
        free(gradient);
        free(gradient_shifted);
        free(trial);
        return 0;
    }

    for (uint32_t iter = 0; iter < max_iters; ++iter) {
        float value = func(x, gradient, n, user_data);
        float grad_norm = UpeMath_NormF32(gradient, n);
        if (grad_norm <= tolerance) {
            free(gradient); free(gradient_shifted); free(trial);
            return (int)(iter + 1u);
        }

        memcpy(trial, x, n * sizeof(float));
        for (uint32_t i = 0; i < n; ++i) {
            float eps = 1.0e-3f;
            trial[i] = x[i] + eps;
            func(trial, gradient_shifted, n, user_data);
            trial[i] = x[i];
            float diag_h = (gradient_shifted[i] - gradient[i]) / eps;
            if (fabsf(diag_h) < 1.0e-4f) {
                diag_h = (diag_h < 0.0f) ? -1.0f : 1.0f;
            }
            trial[i] = x[i] - (gradient[i] / diag_h);
        }

        float trial_value = func(trial, nullptr, n, user_data);
        if (trial_value < value) {
            memcpy(x, trial, n * sizeof(float));
        } else {
            for (uint32_t i = 0; i < n; ++i) {
                x[i] -= 0.25f * gradient[i];
            }
        }
    }

    free(gradient); free(gradient_shifted); free(trial);
    return (int)max_iters;
}

int UpeMath_LBFGS(UpeObjectiveFunc func, float *x, uint32_t n,
                  uint32_t max_iters, float tolerance, uint32_t history_size, void *user_data) {
    float base_lr = 0.35f;
    if (history_size > 0u) {
        base_lr = 0.5f / (float)history_size;
        if (base_lr < 0.05f) {
            base_lr = 0.05f;
        }
    }
    return UpeMath_GradientDescent(func, x, n, base_lr, max_iters, tolerance, user_data);
}

void UpeMath_FFT(const float *real, const float *imag, float *out_real, float *out_imag, uint32_t n) {
    if (!real || !out_real || !out_imag || n == 0u) {
        return;
    }
    for (uint32_t k = 0; k < n; ++k) {
        double sum_r = 0.0;
        double sum_i = 0.0;
        for (uint32_t t = 0; t < n; ++t) {
            double phase = (2.0 * 3.14159265358979323846 * (double)k * (double)t) / (double)n;
            double in_r = real[t];
            double in_i = imag ? imag[t] : 0.0;
            sum_r += in_r * cos(phase) + in_i * sin(phase);
            sum_i += -in_r * sin(phase) + in_i * cos(phase);
        }
        out_real[k] = (float)sum_r;
        out_imag[k] = (float)sum_i;
    }
}

void UpeMath_IFFT(const float *real, const float *imag, float *out_real, float *out_imag, uint32_t n) {
    if (!real || !out_real || !out_imag || n == 0u) {
        return;
    }
    for (uint32_t k = 0; k < n; ++k) {
        double sum_r = 0.0;
        double sum_i = 0.0;
        for (uint32_t t = 0; t < n; ++t) {
            double phase = (2.0 * 3.14159265358979323846 * (double)k * (double)t) / (double)n;
            double in_r = real[t];
            double in_i = imag ? imag[t] : 0.0;
            sum_r += in_r * cos(phase) - in_i * sin(phase);
            sum_i += in_r * sin(phase) + in_i * cos(phase);
        }
        out_real[k] = (float)(sum_r / (double)n);
        out_imag[k] = (float)(sum_i / (double)n);
    }
}

UpeVec3 UpeMath_BezierCubic(UpeVec3 p0, UpeVec3 p1, UpeVec3 p2, UpeVec3 p3, float t) {
    float u = 1.0f - t;
    float b0 = u*u*u;
    float b1 = 3.0f*u*u*t;
    float b2 = 3.0f*u*t*t;
    float b3 = t*t*t;
    return {
        b0*p0.x + b1*p1.x + b2*p2.x + b3*p3.x,
        b0*p0.y + b1*p1.y + b2*p2.y + b3*p3.y,
        b0*p0.z + b1*p1.z + b2*p2.z + b3*p3.z
    };
}

UpeVec3 UpeMath_BezierCubicTangent(UpeVec3 p0, UpeVec3 p1, UpeVec3 p2, UpeVec3 p3, float t) {
    float u = 1.0f - t;
    return {
        3.0f*u*u*(p1.x-p0.x) + 6.0f*u*t*(p2.x-p1.x) + 3.0f*t*t*(p3.x-p2.x),
        3.0f*u*u*(p1.y-p0.y) + 6.0f*u*t*(p2.y-p1.y) + 3.0f*t*t*(p3.y-p2.y),
        3.0f*u*u*(p1.z-p0.z) + 6.0f*u*t*(p2.z-p1.z) + 3.0f*t*t*(p3.z-p2.z)
    };
}

float UpeMath_FractalBoxDimension(float count_small, float count_large, float scale_ratio) {
    if (count_small <= 0.0f || count_large <= 0.0f || scale_ratio <= 0.0f || scale_ratio == 1.0f) return 0.0f;
    return logf(count_large / count_small) / logf(scale_ratio);
}

static float wrap_pi2(float a) {
    const float PI2 = 6.28318530718f;
    while (a < 0.0f) a += PI2;
    while (a >= PI2) a -= PI2;
    return a;
}

float UpeMath_LUTSin(float angle_rad) {
    const int N = 256;
    static int init = 0;
    static float lut[N];
    if (!init) {
        for (int i = 0; i < N; i++) lut[i] = sinf((6.28318530718f * i) / (float)N);
        init = 1;
    }
    float a = wrap_pi2(angle_rad);
    float f = a * (N / 6.28318530718f);
    int i0 = (int)f & (N - 1);
    int i1 = (i0 + 1) & (N - 1);
    float t = f - (float)((int)f);
    return lut[i0] * (1.0f - t) + lut[i1] * t;
}

float UpeMath_LUTCos(float angle_rad) {
    return UpeMath_LUTSin(angle_rad + 1.57079632679f);
}

float UpeMath_SquareCubeLaw(float linear_scale) {
    if (linear_scale <= 0.0f) return 0.0f;
    /* (s^2)/(s^3) relative to baseline => 1/s */
    return 1.0f / linear_scale;
}
