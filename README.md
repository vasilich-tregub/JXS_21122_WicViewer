# JXS 21122 WicViewer

The project is created to help the user visualize 
<a href="https://standards.iso.org/iso-iec/21122/-4/ed-3/en/part-4-refstreams.zip">JPEG XS files of ISO/IEC 21122-4, Part 4: Conformance testing</a> archive.

A Windows classic sample 
<a href="https://learn.microsoft.com/en-us/windows/win32/wic/-wic-sample-d2d-viewer">WIC image viewer using Direct2D sample</a> is extended with options enabling the user
1) to view JPEG XS files (app menu File->Open JPEG XS files)
2) to zoom the images (app menu items View->Actual Size, Zoom In, Zoom Out)

To decode an JPEG XS file, the ISO 21122 reference codec is used, 
build as a static library jxs(d).lib. Another compilation module of 
the ISO 21122 reference software package, file_io.h/file_io.c, is used 
to create a bitstream buffer. The public methods of jxs.lib library 
(declared in the libjxs.h header file in the project) decode the bitstream 
**bitstream\_buf** into a planar color raster image structure of **image\_xs\_t** type
(see the libjxs.h header file).
