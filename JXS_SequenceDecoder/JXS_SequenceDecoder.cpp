// JXS_SequenceDecoder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <string>
#include <vector>
#include <time.h>
#include "libjxs.h"
#include "PlanarToPackaged.h"
extern "C" {
#include "cmdline_options.h"
#include "file_io.h"
#include "file_sequence.h"
#include "image_open.h"
}
#ifdef _DEBUG
#pragma comment(lib, "jxsd.lib")
#pragma comment(lib, "jxs_utils_d.lib")
#else
#pragma comment(lib, "jxs.lib")
#pragma comment(lib, "jxs_utils.lib")
#endif // DEBUG

#define DO_NOT_WRITE_JXS_SEQUENCE_OUTPUT_FILES

int main(int argc, char** argv)
{
    cmdline_options_t options;
    int optind;
    char* input_seq_n = NULL;
    char* output_seq_n = NULL;
    char* input_fn = NULL;
    char* output_fn = NULL;
    xs_config_t xs_config;
    xs_image_t image = { 0 };
    uint8_t* bitstream_buf = NULL;
    size_t bitstream_buf_size, bitstream_buf_max_size;
    xs_dec_context_t* ctx = NULL;
    int ret = 0;
    int file_idx = 0;

    std::string decode_durations("");

    if ((optind = cmdline_options_parse(argc, argv, CMDLINE_OPT_DECODER, &options)) < 0)
    {
        ret = -1;
        return ret;
    }

    xs_config.verbose = options.verbose;

    if (argc - optind == 1)
    {
    }
    else if (argc - optind == 2)
    {
        output_seq_n = argv[optind + 1];
        output_fn = (char*) malloc(strlen(output_seq_n) + MAX_SEQ_NUMBER_DIGITS);
    }
    else
    {
        fprintf(stderr, "\nShow image XS configuration: %s -D [options] <input.jxc>\n", argv[0]);
        fprintf(stderr, "\nSingle image: %s [options] <input.jxc> <image_out.ppm>\n", argv[0]);
        fprintf(stderr, "\nFor sequences: %s [options] <sequence_in_%%06d.jxc> <sequence_out_%%06d.ppm>\n\n", argv[0]);
        fprintf(stderr, "Options:\n");
        cmdline_options_print_usage(CMDLINE_OPT_DECODER);
        ret = -1;
        return ret;
    }

    input_seq_n = argv[optind];
    input_fn = (char*) malloc(strlen(input_seq_n) + MAX_SEQ_NUMBER_DIGITS);

    sequence_get_filepath(input_seq_n, input_fn, file_idx + options.sequence_first);
    bitstream_buf_max_size = fileio_getsize(input_fn);
    if (bitstream_buf_max_size <= 0)
    {
        fprintf(stderr, "Unable to open file %s\n", input_seq_n);
        ret = -1;
        return ret;
    }

    bitstream_buf = (uint8_t*)malloc((bitstream_buf_max_size + 8) & (~0x7));
    if (!bitstream_buf)
    {
        fprintf(stderr, "Unable to allocate memory for codestream\n");
        ret = -1;
        return ret;
    }
    fileio_read(input_fn, bitstream_buf, &bitstream_buf_size, bitstream_buf_max_size);

    if (!xs_dec_probe(bitstream_buf, bitstream_buf_size, &xs_config, &image))
    {
        fprintf(stderr, "Unable to parse input codestream (%s)\n", input_fn);
        ret = -1;
        return ret;
    }

    if (options.verbose > 0 || options.dump_xs_cfg > 0)
    {
        fprintf(stdout, "Image: %dx%d %d-comp@%dbpp\n", image.width, image.height, image.ncomps, image.depth);
        fprintf(stdout, "Sampling: %dx%d", image.sx[0], image.sy[0]);
        for (int c = 1; c < image.ncomps; ++c)
        {
            fprintf(stdout, "/%dx%d", image.sx[c], image.sy[c]);
        }
        fprintf(stdout, "\n");
    }

    if (options.dump_xs_cfg > 0)
    {
        char dump_str[1024];
        memset(dump_str, 0, 1024);
        if (!xs_config_dump(&xs_config, &image, dump_str, 1024, options.dump_xs_cfg))
        {
            fprintf(stderr, "Unable to dump configuration for codestream (%s)\n", input_fn);
            ret = -1;
            return ret;
        }
        fprintf(stdout, "Configuration: \"%s\"\n", dump_str);
    }

    if (argc - optind == 1)
    {
        // No output file given.
        fprintf(stderr, "Not decoding, no output file given\n");
        return ret;
    }

    if (!xs_allocate_image(&image, false))
    {
        fprintf(stderr, "Image memory allocation error\n");
        ret = -1;
        return ret;
    }

    ctx = xs_dec_init(&xs_config, &image);
    if (!ctx)
    {
        fprintf(stderr, "Error parsing the codestream\n");
        ret = -1;
        return ret;
    }

    do
    {
        clock_t start = clock();
        if (!xs_dec_bitstream(ctx, file_idx, bitstream_buf, bitstream_buf_max_size, &image))
        {
            fprintf(stderr, "Error while decoding the codestream\n");
            ret = -1;
            break;
        }
        if (!xs_dec_postprocess_image(&xs_config, &image))
        {
            fprintf(stderr, "Error postprocessing the image\n");
            ret = -1;
            break;
        }
        decode_durations += std::to_string(clock() - start) + " ";

#ifndef DO_NOT_WRITE_JXS_SEQUENCE_OUTPUT_FILES
        sequence_get_filepath(output_seq_n, output_fn, file_idx + options.sequence_first);
        if (is_sequence_path(input_seq_n) || is_sequence_path(output_seq_n))
        {
            fprintf(stderr, "writing %s\n", output_fn);
        }
        if (image_save_auto(output_fn, &image) < 0)
        {
            ret = -1;
            break;
        }
#endif
        if (!(is_sequence_path(input_seq_n) || is_sequence_path(output_seq_n)))
        {
            break;
        }

        file_idx++;
        if ((options.sequence_n > 0) && (file_idx >= options.sequence_n))
        {
            break;
        }
        sequence_get_filepath(input_seq_n, input_fn, file_idx + options.sequence_first);
    } while (ret == 0 && fileio_read(input_fn, bitstream_buf, &bitstream_buf_size, bitstream_buf_max_size) >= 0);

    if (input_fn)
    {
        free(input_fn);
    }
    if (output_fn)
    {
        free(output_fn);
    }
    xs_free_image(&image);
    if (ctx)
    {
        xs_dec_close(ctx);
    }
    if (bitstream_buf)
    {
        free(bitstream_buf);
    }

    printf("%s\n", (char*)decode_durations.c_str());

    return ret;
}
