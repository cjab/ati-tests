#include "ati.h"
#include "cce.h"
#include "r128_cce.h"
#include "r100_cce.h"

bool
ati_init_cce_engine(ati_device_t *dev)
{
    switch (ati_get_chip_family(dev)) {
    case CHIP_R128:
        ati_r128_init_cce_engine(dev); break;
    case CHIP_R100:
        ati_r100_init_cce_engine(dev);
        break;
    case CHIP_UNKNOWN:
    default:
        return false;
        break;
    }
    return true;
}

bool
ati_start_cce_engine(ati_device_t *dev)
{
    switch (ati_get_chip_family(dev)) {
    case CHIP_R128:
        ati_r128_start_cce_engine(dev);
        break;
    case CHIP_R100:
        ati_r100_start_cce_engine(dev);
        break;
    case CHIP_UNKNOWN:
    default:
        return false;
        break;
    }
    return true;
}

bool
ati_stop_cce_engine(ati_device_t *dev)
{
    switch (ati_get_chip_family(dev)) {
    case CHIP_R128:
        ati_r128_stop_cce_engine(dev);
        break;
    case CHIP_R100:
        ati_r100_stop_cce_engine(dev);
        break;
    case CHIP_UNKNOWN:
    default:
        return false;
        break;
    }
    return true;
}

bool
ati_dump_microcode(ati_device_t *dev, uint32_t *out)
{
    switch (ati_get_chip_family(dev)) {
    case CHIP_R128:
        ati_r128_dump_microcode(dev, out);
        break;
    case CHIP_R100:
        ati_r100_dump_microcode(dev, out);
        break;
    case CHIP_UNKNOWN:
    default:
        return false;
        break;
    }
    return true;
}

bool
ati_read_microcode(ati_device_t *dev, uint8_t addr, uint64_t *out)
{
    switch (ati_get_chip_family(dev)) {
    case CHIP_R128:
        *out = ati_r128_read_microcode(dev, addr);
        break;
    case CHIP_R100:
        *out = ati_r100_read_microcode(dev, addr);
        break;
    case CHIP_UNKNOWN:
    default:
        return false;
        break;
    }
    return true;
}

bool
ati_write_microcode(ati_device_t *dev, uint8_t addr, uint64_t inst)
{
    switch (ati_get_chip_family(dev)) {
    case CHIP_R128:
        ati_r128_write_microcode(dev, addr, inst);
        break;
    case CHIP_R100:
        ati_r100_write_microcode(dev, addr, inst);
        break;
    case CHIP_UNKNOWN:
    default:
        return false;
        break;
    }
    return true;
}
