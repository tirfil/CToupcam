# CToupcam
## CAPTURE
Need Touptek cam SDK : libtoupcam.so to install under /usr/local/lib. Export it in LD_LIBRARY_PATH.

RGB and MONO mode use pull mode. But RAW mode uses push mode because pull mode doesn't work correctly.

RGB and MONO captures are in 8 bit. RAW mode in 12 bit.Use bayer2rgb to debayer RAW mode.

Outputs are FITS files.

Should be also working for Altair, RisingTech, Prostar camera.
