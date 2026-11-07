# JXS 21122 WicViewer

The project is created to help the user visualize 
<a href="https://standards.iso.org/iso-iec/21122/-4/ed-3/en/part-4-refstreams.zip">JPEG XS files of ISO/IEC 21122-4, Part 4: Conformance testing</a> archive.

A Windows classic sample 
<a href="https://learn.microsoft.com/en-us/windows/win32/wic/-wic-sample-d2d-viewer">WIC image viewer using Direct2D sample</a> is extended with options enabling the user
1) to view JPEG XS files and sequencies (app menu File->Open JPEG XS files)
2) to zoom the images (app menu items View->Actual Size, Zoom In, Zoom Out, accelerators Backspace, numpad +, numpad -)
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

To decode a sequence of files with TDC markers, you select sequential files of 
your sequence in the dialog 'File/Open JPEG XS Sequence' that you open via 
the app main menu. A 65-frame sample sequence is in 308.zip of this repo, 
a 90-frame sample sequence is in sintel.zip of this repo,
other sequencies can be extracted from the original archive 
https://standards.iso.org/iso-iec/21122/-4/ed-3/en/tests.zip .

Once you open View JXS sequence in JXS_21122_WicViewer, you can walk through 
individual frames with Page Up/Page Down. Interesting insight gives viewing 
a sequence with a starting frame, like frame_00000000.jxs in sequence 308,
omitted. For sequence 'sintel' a starting frame is sintel0001.jxc; if you 
select the sequence starting from sintel0002.jxc to the last frame, you can 
see how the playback quality is gradually regained.

To compare the decoding performance on bitstreams with and without TDC markers, 
two sequences were generated:

in the folder 'sintel', with 
> \> jxs_encoder.exe -c p=tdc -c rate=4 -f 1 -n 90 sintelppm/sintel%04d.ppm sintel/sintel%04d.jxc

in the folder 'sintel-no-tdc', with
> \> jxs_encoder.exe -c rate=4 -f 1 -n 90 sintelppm/sintel%04d.ppm sintel-no-tdc/sintel%04d.jxc

Based on xs_dec_main.c from the reference codec source, I authored a console application, 
JXS_SequenceDecoder, which measures and shows decoding times per frame.

JXS_SequenceDecoder can be run like this:
> \> JXS_SequenceDecoder -f 1 -n 90 sintel/sintel%04d.jxc sintelppm/sintel%04d.ppm<br/>
Image: 720x306 3-comp@8bpp<br/>
Sampling: 1x1/1x1/1x1<br/>
24 24 24 24 24 23 24 23 23 24 25 24 23 23 23 24 23 23 23 24 23 23 23 24 23 24 21 21 22 21 21 21 21 22 21 23 22 24 22 23 22 27 24 25 26 25 23 24 29 23 21 21 21 21 21 21 21 21 22 21 22 21 21 21 21 22 21 21 22 22 23 23 22 23 23 23 22 23 23 23 23 23 23 26 28 26 26 25 24 24

or

> \> JXS_SequenceDecoder -f 1 -n 90 sintel-no-tdc/sintel%04d.jxc sintelppm/sintel%04d.ppm<br/>
Image: 720x306 3-comp@8bpp<br/>
Sampling: 1x1/1x1/1x1<br/>
18 16 16 16 16 16 17 17 16 16 16 16 16 17 16 16 17 17 17 16 17 16 16 16 16 16 16 16 16 16 17 17 16 16 16 17 17 16 16 16 17 16 16 16 16 16 16 16 17 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 18 17 17 16 16 17 16 16 16 16 17 16 16 17 16 17 16 16 17 17

To conform to jxs_decoder's options format, an output sequence is specified 
but file write operation is muted by 
> #define DO_NOT_WRITE_JXS_SEQUENCE_OUTPUT_FILES
