PTCX: Texture Compression
=======

This is a very simple compressed image format that was designed in 2003 as part of the Vision 1.0 project. PTCX features a basic adaptive quantization scheme that is reasonably effective for low frequency texture information (e.g. grass and gravel), and supports a wide variety of pixel formats. 

PTCX closely resembles the DirectX Texture Compression (DXT or S3TC) formats, but offers significantly greater flexibility. This format divides a source image into macroblock tiles and then independently quantizes each block using an adaptive vector quantizer.

What makes PTCX unique is that the block size, quantization step size, control value precision, and range evaluation functions are all fully configurable and adjust according to the characteristics of the input data.

For more information about PTCX and how it works, visit [bertolami.com](http://bertolami.com/index.php?engine=portfolio&content=research&detail=primitive-texture-compression).
