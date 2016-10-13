# rtl8723bs
Realtek SDIO Wi-Fi driver

Tested on:
- Onda v975w
- Teclast 3G
- HP Stream 7
- Dell Venue 8 3000
- WinBook TW100 and TW700
- Acer Aspire Switch 10E
- Lenovo Miix 3-830 and 3-1030
- Medion Akoya S2217
- Chuwi Hi12
- Intel Compute Stick STCK1A32WFC
- Lark Ultimate 7i WIN

Do verify that the device is listed under ```/sys/bus/sdio/```

Note that on all those tablets, you will need to apply a few patches because
to the BayTrail SDIO drivers:
http://thread.gmane.org/gmane.linux.kernel.mmc/25081/focus=25087
http://thread.gmane.org/gmane.linux.kernel.mmc/31583
For older kernel than 4.3, you might also need this patch applied:
https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=d31911b9374a76560d2c8ea4aa6ce5781621e81d

You can find these patches in the following directories:
- `patches/` for Kernels < 4.5
  - Patch #4a is to be used for kernels < 4.4.0, #4b for kernels >= 4.4.0.
- `patches_4.5/` for kernels >= 4.5
