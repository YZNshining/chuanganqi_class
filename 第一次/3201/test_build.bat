@echo off
cd /d F:\chuanganqi\3201
"E:\keil5\ARM\ARMCC\bin\armcc.exe" -c --cpu Cortex-M3 --gnu -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -I.\LIB -I"E:\keil5\ARM\PACK\ARM\CMSIS\5.0.1\CMSIS\Include" -I"E:\keil5\ARM\PACK\Keil\STM32F1xx_DFP\2.4.1\Device\Include" -I"E:\keil5\ARM\PACK\Keil\STM32F1xx_DFP\2.4.1\Device\StdPeriph_Driver\inc" .\Listings\lcd1602.c 2>&1
