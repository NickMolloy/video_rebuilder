#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <inttypes.h>

#define MAX(a, b) ( ((a) > (b)) ? (a) : (b) )

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Incorrect number of arguments\n");
        printf("Usage: reconstruct <FilePart1> <FilePart2> <OutputFile>\n");
        return -1;
    }

    AVFormatContext *inputContext = NULL, *outputContext = NULL;
    AVOutputFormat *outputFormat = NULL;
    AVPacket packet;

    int video_stream_index = -1;
    int audio_stream_index = -1;
    char err_msg[100];
    const char *input_filename1, *input_filename2, *output_filename;
    int ret, i;
    int64_t max_video_pts = -1;
    int64_t max_video_dts = -1;
    int64_t max_audio_pts = -1;
    int64_t max_audio_dts = -1;

    input_filename1 = argv[1];
    input_filename2 = argv[2];
    output_filename = argv[3];

    av_register_all();

    // Allocate input context
    inputContext = avformat_alloc_context();
    if (!inputContext) {
        printf("Failed to allocate input format context\n");
        return -1;
    }

    // Allocate output context
    outputContext = avformat_alloc_context();
    if (!outputContext) {
        printf("Failed to allocate output format context\n");
        return -1;
    }

    // open input file 1
    ret = avformat_open_input(&inputContext, input_filename1, NULL, NULL);
    if (ret != 0) {
        av_strerror(ret, err_msg, sizeof(err_msg));
        printf("Failed to open file %s: %s\n", input_filename1, err_msg);
        return -1;
    }

    ret = avformat_find_stream_info(inputContext, NULL);
    if (ret !=0) {
        av_strerror(ret, err_msg, sizeof(err_msg));
        printf("Failed to get stream info: %s\n", err_msg);
        return -1;
    }

    // Open output file
    ret = avformat_alloc_output_context2(&outputContext, NULL, NULL, output_filename);
    if (ret < 0) {
        av_strerror(ret, err_msg, sizeof(err_msg));
        printf("Failed to create output context: %s\n", err_msg);
        return -1;
    }

    for (i = 0; i < inputContext->nb_streams; i++) {
        AVStream *output_stream = NULL;
        AVStream *input_stream = inputContext->streams[i];
        AVCodecParameters *input_codec_parameters = input_stream->codecpar;

        output_stream = avformat_new_stream(outputContext, NULL);
        if (!output_stream) {
            printf("Failed to allocate output stream\n");
            return -1;
        }

        ret = avcodec_parameters_copy(output_stream->codecpar, input_codec_parameters);
        if (ret < 0) {
            av_strerror(ret, err_msg, sizeof(err_msg));
            printf("Failed to copy codec parameters: %s\n", err_msg);
            return -1;
        }

        output_stream->codecpar->codec_tag = 0;

        if (input_codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        } else if (input_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
    }

    if (video_stream_index == -1) {
        printf("Failed to find video stream\n");
        return -1;
    } else if (audio_stream_index == -1) {
        printf("Failed to find audio stream\n");
        return -1;
    }

    outputFormat = outputContext->oformat;
    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outputContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("Could not open output file: %s", output_filename);
            return -1;
        }
    }

    ret = avformat_write_header(outputContext, NULL);
    if (ret < 0) {
        av_strerror(ret, err_msg, sizeof(err_msg));
        printf("Error occurred when opening output file: %s\n", err_msg);
        return -1;
    }

    while (av_read_frame(inputContext, &packet) >= 0) {
        AVStream *in_stream, *out_stream;
        in_stream = inputContext->streams[packet.stream_index];
        out_stream = outputContext->streams[packet.stream_index];

        if (packet.stream_index == video_stream_index) {
            max_video_pts = MAX(max_video_pts, packet.pts);
            max_video_dts = MAX(max_video_dts, packet.dts);
        } else if (packet.stream_index == audio_stream_index) {
            max_audio_pts = MAX(max_audio_pts, packet.pts);
            max_audio_dts = MAX(max_audio_dts, packet.dts);
        }

        packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
        packet.pos = -1;
        ret = av_interleaved_write_frame(outputContext, &packet);
        if (ret < 0) {
            av_strerror(ret, err_msg, sizeof(err_msg));
            printf("Error muxing packet: %s\n", err_msg);
            break;
        }
        av_packet_unref(&packet);
    }

    printf("Max video pts: %"PRId64"\n", max_video_pts);
    printf("Max video dts: %"PRId64"\n", max_video_dts);
    printf("Max audio pts: %"PRId64"\n", max_audio_pts);
    printf("Max audio dts: %"PRId64"\n", max_audio_dts);

    avformat_close_input(&inputContext);
    avformat_free_context(inputContext);

    // Process input file 2
    ret = avformat_open_input(&inputContext, input_filename2, NULL, NULL);
    if (ret != 0) {
        av_strerror(ret, err_msg, sizeof(err_msg));
        printf("Failed to open file %s: %s\n", input_filename2, err_msg);
        return -1;
    }

    ret = avformat_find_stream_info(inputContext, NULL);
    if (ret !=0) {
        av_strerror(ret, err_msg, sizeof(err_msg));
        printf("Failed to get stream info: %s\n", err_msg);
        return -1;
    }

    while (av_read_frame(inputContext, &packet) >= 0) {
        AVStream *in_stream, *out_stream;
        in_stream = inputContext->streams[packet.stream_index];
        out_stream = outputContext->streams[packet.stream_index];

        if ( ((packet.stream_index == video_stream_index) && (packet.pts > max_video_pts)) || ((packet.stream_index == audio_stream_index) && (packet.pts > max_audio_pts)) ) {
            packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
            packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
            packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
            packet.pos = -1;
            ret = av_interleaved_write_frame(outputContext, &packet);
            if (ret < 0) {
                av_strerror(ret, err_msg, sizeof(err_msg));
                printf("Error muxing packet: %s\n", err_msg);
                break;
            }
        }
        av_packet_unref(&packet);
    }

    av_write_trailer(outputContext);

    avformat_close_input(&inputContext);
    avformat_free_context(inputContext);

    return 0;

}
