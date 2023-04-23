#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <jpeglib.h>

// Function to write the JPEG file.
void write_jpeg_file(char* filename, unsigned char* pixel_data, int height, int width, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE* outfile;
    JSAMPROW row_pointer[1];

    // Open the output file.
    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "Can't open %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Initialize the JPEG compression object.
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    // Set the image width, height, and color space.
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    // Set the JPEG compression parameters.
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    // Start the compression process.
    jpeg_start_compress(&cinfo, TRUE);

    // Write each scanline of the image.
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &pixel_data[cinfo.next_scanline * width * 3];
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // Finish the compression process and close everything.
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);
}

// HSL function.
float f_HtR(int n, float H, float S, float L){
    float k;
    float a;

    k = fmod(H*360/30 + n, 12);
    a = S*std::min(L, 1-L);

    return L-a*std::max((float)-1,std::min(k-3,std::min(9-k, (float) 1)));
}

// Function that converts HSL float values to RGB24.
int* HSLtoRGB(float H, float S, float L){ // takes H, S, and L as floats between 0 and 1
    static float RGB[3];
    static int ZHQ[3]={0,8,4};
    static int RGB_int[3];

    for(int i=0; i<3; i++){
        RGB[i] = f_HtR(ZHQ[i], H, S, L);
        RGB_int[i] = int(RGB[i]*255);
    }

    return RGB_int;
}