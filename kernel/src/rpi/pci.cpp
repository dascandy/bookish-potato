#include "rpi/mailbox.h"
#include "debug.h"
#include "io.h"
/*
void pci_init(uintptr_t pcie_base) {
  // Reset pci root
  mmio_write(pcie_base + 0x9210, mmio_read<uint32_t>(pcie_base + 0x9210) | 2);
  delay(x);
  mmio_write(pcie_base + 0x9210, mmio_read<uint32_t>(pcie_base + 0x9210) & 0xFFFFFFFD);

  // stabilize serdes
  mmio_write(pcie_base + 0x4204, mmio_read<uint32_t>(pcie_base + 0x4204) & 0xF7FFFFFF);
  delay(x);
}
/
  u32p_replace_bits(&tmp, 1, PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN_MASK);
  u32p_replace_bits(&tmp, 1, PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE_MASK);
  u32p_replace_bits(&tmp, PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_128,
        PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK);
  writel(tmp, base + PCIE_MISC_MISC_CTRL);

  ret = brcm_pcie_get_rc_bar2_size_and_offset(pcie, &rc_bar2_size,
                &rc_bar2_offset);
  if (ret)
    return ret;

  tmp = lower_32_bits(rc_bar2_offset);
  u32p_replace_bits(&tmp, brcm_pcie_encode_ibar_size(rc_bar2_size),
        PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_MASK);
  writel(tmp, base + PCIE_MISC_RC_BAR2_CONFIG_LO);
  writel(upper_32_bits(rc_bar2_offset),
         base + PCIE_MISC_RC_BAR2_CONFIG_HI);

  scb_size_val = rc_bar2_size ?
           ilog2(rc_bar2_size) - 15 : 0xf;
  tmp = readl(base + PCIE_MISC_MISC_CTRL);
  u32p_replace_bits(&tmp, scb_size_val,
        PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK);
  writel(tmp, base + PCIE_MISC_MISC_CTRL);

  // *
  // * We ideally want the MSI target address to be located in the 32bit
  // * addressable memory area. Some devices might depend on it. This is
  // * possible either when the inbound window is located above the lower
  // * 4GB or when the inbound area is smaller than 4GB (taking into
  // * account the rounding-up we're forced to perform).
  // * /
  if (rc_bar2_offset >= SZ_4G || (rc_bar2_size + rc_bar2_offset) < SZ_4G)
    pcie->msi_target_addr = BRCM_MSI_TARGET_ADDR_LT_4GB;
  else
    pcie->msi_target_addr = BRCM_MSI_TARGET_ADDR_GT_4GB;

  /* disable the PCIe->GISB memory window (RC_BAR1) * /
  tmp = readl(base + PCIE_MISC_RC_BAR1_CONFIG_LO);
  tmp &= ~PCIE_MISC_RC_BAR1_CONFIG_LO_SIZE_MASK;
  writel(tmp, base + PCIE_MISC_RC_BAR1_CONFIG_LO);

  /* disable the PCIe->SCB memory window (RC_BAR3) * /
  tmp = readl(base + PCIE_MISC_RC_BAR3_CONFIG_LO);
  tmp &= ~PCIE_MISC_RC_BAR3_CONFIG_LO_SIZE_MASK;
  writel(tmp, base + PCIE_MISC_RC_BAR3_CONFIG_LO);

  /* Mask all interrupts since we are not handling any yet * /
  writel(0xffffffff, pcie->base + PCIE_MSI_INTR2_MASK_SET);

  /* clear any interrupts we find on boot * /
  writel(0xffffffff, pcie->base + PCIE_MSI_INTR2_CLR);




  if (pcie->gen)
    brcm_pcie_set_gen(pcie, pcie->gen);

  /* Unassert the fundamental reset * /
  brcm_pcie_perst_set(pcie, 0);


  for (i = 0; i < 100 && !brcm_pcie_link_up(pcie); i += 5)
    msleep(5);

  if (!brcm_pcie_link_up(pcie)) {
    dev_err(dev, "link down\n");
    return -ENODEV;
  }

*/

