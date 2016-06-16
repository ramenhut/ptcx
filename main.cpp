
#include "base.h"
#include "ptcx.h"
#include "bitmap.h"

using namespace base;

uint32 _get_file_size(FILE *f)
{
    uint32 file_size = 0;
    uint32 seek_pos = ftell(f);

    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, seek_pos, SEEK_SET);

    return file_size;
}

void _read_file_into_stream(FILE *input, memory_stream *output)
{
    uint32 input_file_size = _get_file_size(input);

    if (0 == input_file_size)
    {
        base_msg("Input file is empty!");
        return;
    }

    if (input_file_size != output->resize_capacity(input_file_size))
    {
        base_msg("failed to resize input memory stream.");
        return;
    }

    fread(output->query_write_pointer(), 1, input_file_size, input);

    output->advance_write_pointer(input_file_size);
}

void _write_stream_to_file(memory_stream *input, FILE *output)
{
    if (input->is_empty())
    {
        base_msg("Input stream is empty!");
        return;
    }

    fwrite(input->query_read_pointer(), 1, input->query_occupancy(), output);

    input->advance_read_pointer(input->query_occupancy());
}

void _read_bitmap_from_file(char *filename, image *output)
{
    FILE *input = fopen(filename, "rb");
    memory_stream input_stream;

    if (!input)
    {
        base_msg("Error reading file %s.", filename);
        return;
    }

    _read_file_into_stream(input, &input_stream);
    load_bitmap(&input_stream, output);
    fclose(input);
}

void _write_bitmap_to_file(const image &input, char *filename)
{
    FILE *output = fopen(filename, "wb");
    memory_stream output_stream;

    if (!output)
    {
        base_msg("Error writing file %s.", filename);
        return;
    }

    // write image to stream as bitmap
    output_stream.resize_capacity(1*BASE_MB);
    save_bitmap(&output_stream, input);
    _write_stream_to_file(&output_stream, output);
    fclose(output);
}

void _stream_test(char *input_filename, char *output_filename)
{
    FILE *input = fopen(input_filename, "rb");
    FILE *output = fopen(output_filename, "wb");
    memory_stream input_stream;

    _read_file_into_stream(input, &input_stream);
    _write_stream_to_file(&input_stream, output);

    fclose(input);
    fclose(output);
}

int main(int argc, char **argv)
{
    memory_stream ptcx_stream;
    image bitmap_image;

    if (4 != argc)
    {
        base_msg("Required syntax: ptcx_test input_filename quality output_filename");
        return 0;
    }

    _read_bitmap_from_file(argv[1], &bitmap_image);

    // convert bitmap to ptcx and back.
    ptcx_stream.resize_capacity(1*BASE_MB);
    save_ptcx(bitmap_image, atoi(argv[2]), &ptcx_stream);
    
    printf("Size of PTCX: %i bytes\n", ptcx_stream.query_occupancy());

    load_ptcx(&ptcx_stream, &bitmap_image);
    _write_bitmap_to_file(bitmap_image, argv[3]);

    return 0;
}