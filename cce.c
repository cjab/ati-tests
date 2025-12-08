#include "cce.h"

/* CCE microcode (from ATI) */
static uint32_t r128_cce_microcode[] = {
    0,  276838400,   0,  268449792,  2,  142,         2,  145,
    0,  1076765731,  0,  1617039951, 0,  774592877,   0,  1987540286,
    0,  2307490946U, 0,  599558925,  0,  589505315,   0,  596487092,
    0,  589505315,   1,  11544576,   1,  206848,      1,  311296,
    1,  198656,      2,  912273422,  11, 262144,      0,  0,
    1,  33559837,    1,  7438,       1,  14809,       1,  6615,
    12, 28,          1,  6614,       12, 28,          2,  23,
    11, 18874368,    0,  16790922,   1,  409600,      9,  30,
    1,  147854772,   16, 420483072,  3,  8192,        0,  10240,
    1,  198656,      1,  15630,      1,  51200,       10, 34858,
    9,  42,          1,  33559823,   2,  10276,       1,  15717,
    1,  15718,       2,  43,         1,  15936948,    1,  570480831,
    1,  14715071,    12, 322123831,  1,  33953125,    12, 55,
    1,  33559908,    1,  15718,      2,  46,          4,  2099258,
    1,  526336,      1,  442623,     4,  4194365,     1,  509952,
    1,  459007,      3,  0,          12, 92,          2,  46,
    12, 176,         1,  15734,      1,  206848,      1,  18432,
    1,  133120,      1,  100670734,  1,  149504,      1,  165888,
    1,  15975928,    1,  1048576,    6,  3145806,     1,  15715,
    16, 2150645232U, 2,  268449859,  2,  10307,       12, 176,
    1,  15734,       1,  15735,      1,  15630,       1,  15631,
    1,  5253120,     6,  3145810,    16, 2150645232U, 1,  15864,
    2,  82,          1,  343310,     1,  1064207,     2,  3145813,
    1,  15728,       1,  7817,       1,  15729,       3,  15730,
    12, 92,          2,  98,         1,  16168,       1,  16167,
    1,  16002,       1,  16008,      1,  15974,       1,  15975,
    1,  15990,       1,  15976,      1,  15977,       1,  15980,
    0,  15981,       1,  10240,      1,  5253120,     1,  15720,
    1,  198656,      6,  110,        1,  180224,      1,  103824738,
    2,  112,         2,  3145839,    0,  536885440,   1,  114880,
    14, 125,         12, 206975,     1,  33559995,    12, 198784,
    0,  33570236,    1,  15803,      0,  15804,       3,  294912,
    1,  294912,      3,  442370,     1,  11544576,    0,  811612160,
    1,  12593152,    1,  11536384,   1,  14024704,    7,  310382726,
    0,  10240,       1,  14796,      1,  14797,       1,  14793,
    1,  14794,       0,  14795,      1,  268679168,   1,  9437184,
    1,  268449792,   1,  198656,     1,  9452827,     1,  1075854602,
    1,  1075854603,  1,  557056,     1,  114880,      14, 159,
    12, 198784,      1,  1109409213, 12, 198783,      1,  1107312059,
    12, 198784,      1,  1109409212, 2,  162,         1,  1075854781,
    1,  1073757627,  1,  1075854780, 1,  540672,      1,  10485760,
    6,  3145894,     16, 274741248,  9,  168,         3,  4194304,
    3,  4209949,     0,  0,          0,  256,         14, 174,
    1,  114857,      1,  33560007,   12, 176,         0,  10240,
    1,  114858,      1,  33560018,   1,  114857,      3,  33560007,
    1,  16008,       1,  114874,     1,  33560360,    1,  114875,
    1,  33560154,    0,  15963,      0,  256,         0,  4096,
    1,  409611,      9,  188,        0,  10240,       0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0};

void
ati_init_cce_engine(ati_device_t *dev)
{
    ati_wait_for_idle(dev);

    // Load CCE microcode
    wr_pm4_microcode_addr(dev, 0);
    for (int i = 0; i < 256; i += 1) {
        wr_pm4_microcode_datah(dev, r128_cce_microcode[i * 2]);
        wr_pm4_microcode_datal(dev, r128_cce_microcode[i * 2 + 1]);
    }

    // Set to PIO-based CCE mode with 192 entries
    wr_pm4_buffer_cntl(dev, (PM4_192PIO << PM4_BUFFER_MODE_SHIFT) |
                                PM4_BUFFER_CNTL_NOUPDATE);

    // Read as per sample code (may be required for mode change to take effect)
    (void) rd_pm4_buffer_addr(dev);

    // Start the CCE engine
    wr_pm4_micro_cntl(dev, PM4_MICRO_FREERUN);
}

void
ati_stop_cce_engine(ati_device_t *dev)
{
    // Wait for CCE to finish processing
    ati_wait_for_idle(dev);
    // Stop the microengine (clear FREERUN bit)
    wr_pm4_micro_cntl(dev, 0);
    ati_engine_reset(dev);
    // Set back to non-PM4 (standard PIO) mode
    wr_pm4_buffer_cntl(dev, PM4_NONPM4 << PM4_BUFFER_MODE_SHIFT);
}

static void
ati_cce_wait_for_fifo(ati_device_t *dev, uint32_t entries)
{
    while ((rd_pm4_stat(dev) & PM4_FIFOCNT_MASK) < entries) {
        // spin
    }
}

void
ati_cce_pio_submit(ati_device_t *dev, uint32_t *packets, size_t dwords)
{
    for (size_t i = 0; i < dwords; i += 2) {
        ati_cce_wait_for_fifo(dev, 2);
        wr_pm4_fifo_data_even(dev, packets[i]);
        if (i + 1 < dwords) {
            wr_pm4_fifo_data_odd(dev, packets[i + 1]);
        } else {
            wr_pm4_fifo_data_odd(dev, CCE_PKT2());
        }
    }
}

void
ati_cce_load_microcode(ati_device_t *dev)
{
    // Load CCE microcode from static array
    // Note: Engine should be stopped before calling this
    wr_pm4_microcode_addr(dev, 0);
    for (int i = 0; i < 256; i++) {
        wr_pm4_microcode_datah(dev, r128_cce_microcode[i * 2]);
        wr_pm4_microcode_datal(dev, r128_cce_microcode[i * 2 + 1]);
    }
}
