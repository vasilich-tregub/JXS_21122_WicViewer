# JXS 21122 WicViewer

The project is created to help the user visualize 
<a href="https://standards.iso.org/iso-iec/21122/-4/ed-3/en/part-4-refstreams.zip">JPEG XS files of ISO/IEC 21122-4, Part 4: Conformance testing</a> archive.

A Windows classic sample 
<a href="https://learn.microsoft.com/en-us/windows/win32/wic/-wic-sample-d2d-viewer">WIC image viewer using Direct2D sample</a> is extended with options enabling the user
1) to view JPEG XS files (app menu File->Open JPEG XS files)
2) to zoom the images (app menu items View->Actual Size, Zoom In, Zoom Out)
3) in 2026, JXS sequence processing added to help the user study 
new Temporal Differential Coding feature of ISO/IEC 21122 ed. 3,
see [JPEG XS 3rd Edition: The New Temporal Differential Coding (TDC) Profile](https://journal.smpte.org/periodicals/SMPTE%20Motion%20Imaging%20Journal/134/5/16/)

To decode an JPEG XS file, the ISO 21122 reference codec is used, 
built as a static library jxs(d).lib. Another compilation module of 
the ISO 21122 reference software package, file_io.h/file_io.c, is used 
to create a bitstream buffer. The public methods of jxs.lib library 
(declared in the libjxs.h header file in the project) decode the bitstream 
**bitstream\_buf** into a planar color raster image structure of **image\_xs\_t** type
(see the libjxs.h header file).

To decode a sequence of files with TDC markers, you select all files of 
your sequence in the dialog 'File/JPEG XS Files' that opens via the app 
main menu. The sequence files must be named as shown in 
ISO_IEC_21122-5_ED-3's README example of encoding/decoding JXS sequences.
The sample sequence can be found in my repo 
https://github.com/vasilich-tregub/JXS_21122_LAB/tree/master/JXS_TDC 
or extracted from the the original archive 
https://standards.iso.org/iso-iec/21122/-4/ed-3/en/tests.zip .

