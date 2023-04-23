
extern "C"{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define STREAM_DURATION     4.0
#define STREAM_FRAME_RATE   25 // 25 images/s
#define STREAM_PIX_FMT      AV_PIX_FMT_YUV420P // default pix_fmt
// #define STREAM_BITRATE      40000000
#define STREAM_WIDTH        1920/2 // multiple of 2
#define STREAM_HEIGHT       1080/2 // multiple of 2
#define STREAM_GOP_SIZE     15


AVStream *stream;           // Stream
AVCodecContext *encoder;    // Encoder
int64_t next_pts;           // Pts of the next frame that will be generated.
AVFrame *frame;             // Frame
AVPacket *pkt;              // Packet
const AVOutputFormat *fmt;
AVCodecID codec_id;
const char *filename = "out.mp4";
AVFormatContext *out_ctx;
const AVCodec *video_codec;
int ret, i;
int encode_video = 0;
AVDictionary *opt = NULL;
AVFrame *rgbframe;

// Test function to make dummy images.
static void fill_rgb_image(AVFrame *rgbframe, int frame_index,
                           int width, int height)
{
    i = frame_index;

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            // rgbpic->linesize[0] is equal to width.
            rgbframe->data[0][y * rgbframe->linesize[0] + 3*x + 0] = (x   + y + i * 3) % 255 ; // red
            rgbframe->data[0][y * rgbframe->linesize[0] + 3*x + 1] = (128 + y + i * 2) % 255 ; // green
            rgbframe->data[0][y * rgbframe->linesize[0] + 3*x + 2] = (64  + x + i * 5) % 255 ; // blue
        }
    }
}

// Converts a RGB24 AVFrame to YUV420P AVFrame.
static void convert_rgb_to_yuv(AVFrame *pict, AVFrame *rgbframe,
                               int width, int height){
    // Preparing the conversion context.
    struct SwsContext* convertCtx = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                                                   width, height, AV_PIX_FMT_YUV420P,
                                                   SWS_FAST_BILINEAR, NULL, NULL, NULL);

    // Not actually scaling anything, but just converting the RGB data to YUV and store it in pict.
    sws_scale(convertCtx, rgbframe->data, rgbframe->linesize, 0, height, pict->data, pict->linesize);

}

// Write a frame from a pixel RGB24 array.
static void write_frame_mp4(unsigned char* pixel_data, int height, int width) {
    // When we pass a frame to the encoder, it may keep a reference to it
    // internally; make sure we do not overwrite it here.
    if (av_frame_make_writable(frame) < 0)
        exit(1);

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            rgbframe->data[0][y * rgbframe->linesize[0] + 3*x + 0] = pixel_data[(y * width + x) * 3 + 0] ; // red
            rgbframe->data[0][y * rgbframe->linesize[0] + 3*x + 1] = pixel_data[(y * width + x) * 3 + 1] ; // green
            rgbframe->data[0][y * rgbframe->linesize[0] + 3*x + 2] = pixel_data[(y * width + x) * 3 + 2] ; // blue
        }
    }

    convert_rgb_to_yuv(frame, rgbframe, encoder->width, encoder->height);
    frame->pts = next_pts++; // Move to next pts.
    ret = avcodec_send_frame(encoder, frame); // Send the frame to the encoder.

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;

        // Rescale output packet timestamp values from codec to stream timebase.
        av_packet_rescale_ts(pkt, encoder->time_base, stream->time_base);
        pkt->stream_index = stream->index;

        // Write the compressed frame to the media file.
        ret = av_interleaved_write_frame(out_ctx, pkt);
        // pkt is now blank (av_interleaved_write_frame() takes ownership of
        // its contents and resets pkt), so that no unreferencing is necessary.
    }
}

// Prepares the ffmpeg environment to receive the pictures composing the video.
static void prepare_mp4(int width, int height, int bitrate) {

    // Allocate the output media context.
    avformat_alloc_output_context2(&out_ctx, NULL, NULL, filename);
    if (!out_ctx)
        exit(1);

    fmt = out_ctx->oformat;
    codec_id = fmt->video_codec;

    // Add the video stream using the default format codecs
    // and initialize the codecs.

    if (codec_id != AV_CODEC_ID_NONE) {
        video_codec = avcodec_find_encoder(codec_id);
        stream      = avformat_new_stream(out_ctx, NULL);
        stream->id  = out_ctx->nb_streams-1;
        encoder     = avcodec_alloc_context3(video_codec);

        encoder->codec_id  = codec_id;

        encoder->bit_rate  = bitrate;
        encoder->width     = width;
        encoder->height    = height;
        stream->time_base  = (AVRational){ 1, STREAM_FRAME_RATE }; // time_base = 1/frame_rate
        encoder->time_base = stream->time_base;

        encoder->gop_size  = STREAM_GOP_SIZE; // Emit one intra frame every gop_size frames at most.
        encoder->pix_fmt   = STREAM_PIX_FMT;

        // Some formats want stream headers to be separate.
        if (fmt->flags & AVFMT_GLOBALHEADER)
            encoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        encode_video = 1;
    }

    // Now that all the parameters are set, we can open the
    // video codec and allocate the necessary encode buffer.

    ret = avcodec_open2(encoder, video_codec, &opt);   // Open the codec.

    frame = av_frame_alloc();               // Allocate and init a re-usable frame.
    if (!frame)
        exit(1);
    frame->format = encoder->pix_fmt;
    frame->width  = encoder->width;
    frame->height = encoder->height;
    ret = av_frame_get_buffer(frame, 0);    // Allocate the buffers for the frame data.

    rgbframe = av_frame_alloc();            // Allocate and init a RGB frame.
    if (!rgbframe)
        exit(1);
    rgbframe->format = AV_PIX_FMT_RGB24;
    rgbframe->width  = encoder->width;
    rgbframe->height = encoder->height;
    ret = av_frame_get_buffer(rgbframe, 0); // Allocate the buffers for the frame data.

    ret = avcodec_parameters_from_context(stream->codecpar, encoder); // Copy the stream parameters to the muxer.

    av_dump_format(out_ctx, 0, filename, 1);                     // Print info about output.
    ret = avio_open(&out_ctx->pb, filename, AVIO_FLAG_WRITE);    // Open the output file.
    ret = avformat_write_header(out_ctx, &opt);                  // Write the stream header.
        
    pkt = av_packet_alloc(); // Allocate the packet.
}

// Closes the mp4 file and all dependencies.
static void end_mp4() {
    av_write_trailer(out_ctx);      // End of file.
    avcodec_free_context(&encoder); // Close video codec.
    av_frame_free(&frame);          // Free the yuv frame.
    av_frame_free(&rgbframe);       // Free the rgb frame.
    av_packet_free(&pkt);           // Free the packet.
    avio_closep(&out_ctx->pb);      // Close the output file.
    avformat_free_context(out_ctx); // Free the stream.
    av_dict_free(&opt);             // Free dict.
}