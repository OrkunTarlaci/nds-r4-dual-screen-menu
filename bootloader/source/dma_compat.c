// BlocksDS uyumlulugu: libnds'in dma.h satir ici fonksiyonlari dmaSetParams() cagiriyor.
// Bootloader libnds kutuphanesine baglanmadigi icin burada basit bir karsiligi var.
#include <nds/ndstypes.h>
#include <nds/dma.h>

void dmaSetParams(uint8_t channel, const void *src, void *dest, uint32_t ctrl)
{
    REG_DMA_SRC(channel) = (u32)src;
    REG_DMA_DEST(channel) = (u32)dest;
    REG_DMA_CR(channel) = ctrl;
}
